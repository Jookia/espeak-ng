/*
 * Copyright (C) 2026 Jookia <contact@jookia.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, see: <http://www.gnu.org/licenses/>.
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <espeak-ng/espeak_ng.h>

#include "common.h"
#include "bundle.h"

static const uint8_t *Align16(const uint8_t *mem)
{
	// This pushes the mem address over the next multiple of 16 then
	// floors it to a multiple of 16, discarding the remainder.
	// As a special case multiples of 16 are not pushed up to the
	// next multiple. This keeps aligned memory aligned.
	uintptr_t addr = (uintptr_t)mem;
	addr += 0xF; // addr += 15
	addr &= ~0xF; // addr -= (addr % 16)
	return (const uint8_t*)addr;
}

static uint32_t ReadU32BE(const uint8_t* mem)
{
	return (mem[3] << 0) |
		(mem[2] << 8) |
		(mem[1] << 16) |
		(mem[0] << 24);
}

static uint32_t FNV1a(const uint8_t *checkpos, const uint8_t *end)
{
	uint32_t hash = 2166136261;
	do {
		hash ^= *checkpos;
		hash *= 16777619;
	} while(checkpos++ != end);
	return hash;
}

struct bundle_state {
	const uint8_t* start;
	const uint8_t* end;
	bool locked;
};

static struct bundle_state bundle_state;

void BundleLock(bool lock)
{
	bundle_state.locked = lock;
}

bool BundleInUse(void)
{
	bool bundle_loaded = (bundle_state.start != NULL);
	bool bundle_locked = (bundle_state.locked);
	return (bundle_loaded && bundle_locked);
}

ESPEAK_NG_API espeak_ng_STATUS espeak_ng_SetBundle(const void *bundle,
	size_t len)
{
	if (BundleInUse())
		return (espeak_ng_STATUS)EINVAL;
	
	// We assume there is at least a header and one entry
	if (len < 32)
		return (espeak_ng_STATUS)EINVAL;

	const uint8_t *header = (const uint8_t*)bundle;
	const uint8_t *header_aligned = Align16(header);

	if (header != header_aligned)
		return (espeak_ng_STATUS)EINVAL;

	if (memcmp(header, "ESPK1\x00\x00\x00", 8) != 0)
		return (espeak_ng_STATUS)EINVAL;

	uint32_t data_checksum = ReadU32BE(header + 12);
	uint32_t data_len = len - 16; // Skip bundle header

	const uint8_t *start = (header + 16); // Skip bundle header
	const uint8_t *end = (start + data_len) - 1; // Last valid byte

	if (FNV1a(start, end) != data_checksum)
		return (espeak_ng_STATUS)EINVAL;

	bundle_state.start = start;
	bundle_state.end = end;

	return ENS_OK;
}

bool BundleReadNext(struct bundle_entry *entry)
{
	if (!BundleInUse())
		return false;

	if (entry->next_header == NULL)
		entry->next_header = &bundle_state.start[0];

	if (entry->next_header > bundle_state.end)
		return false;

	// NOTE: The memory format of the bundle is packed and unaligned,
	// do NOT try to cast it to larger data types. This will crash on
	// machines that lack unaligned access.

	entry->name = entry->next_header;
	size_t name_len = strlen((const char*)entry->name);

	const uint8_t *size = entry->name + name_len + 1; // Skip NULL
	entry->data_len = ReadU32BE(size);
	entry->data = Align16(size + 4);
	entry->next_header = Align16(entry->data + entry->data_len);

	return true;
}

static bool BundleFindEntry(struct bundle_entry *entry, const char *name)
{
	while (BundleReadNext(entry)) {
		if (strcmp((const char*)entry->name, name) == 0)
			return true;
	}

	return false;
}

int BundleGetFileLength(const char *name)
{
	struct bundle_entry entry = {0};
	if (!BundleFindEntry(&entry, name))
		return -ENOENT;

	return entry.data_len;
}

FILE *BundleFopen(const char *path)
{
	struct bundle_entry entry = {0};
	if (!BundleFindEntry(&entry, path))
		return NULL;

	return fmemopen((void*)entry.data, entry.data_len, "rb");
}

espeak_ng_STATUS BundleMemoryMap(const char *name, size_t *size, const void **ptr)
{
	struct bundle_entry entry = {0};
	if (!BundleFindEntry(&entry, name))
		return (espeak_ng_STATUS)ENOENT;

	*ptr = (const void*)entry.data;
	*size = entry.data_len;

	return ENS_OK;
}
