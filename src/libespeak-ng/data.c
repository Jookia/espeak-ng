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

#include <string.h>
#include <stdlib.h>
#include <errno.h>

#include "common.h"
#include "data.h"
#include "bundle.h"
#include "speech.h" // for path_home

// Warning: This uses a static buffer, copy the result if needed
static const char *PathToBundleName(const char *path)
{
	static char buf[64];

	int home_len = strlen(path_home);
	if (strncmp(path, path_home, home_len) != 0)
		return NULL;

	const char *bundle_name = path + home_len;
	if (bundle_name[0] != PATHSEP)
		return NULL;

	bundle_name = bundle_name + 1; // Skip separator

	if (strlen(bundle_name) > sizeof(buf))
		return NULL;

	char *bufpos = buf;
	do {
		if (*bundle_name == PATHSEP)
			*bufpos++ = '/';
		else
			*bufpos++ = *bundle_name;
	} while (*bundle_name++ != '\0');
	*bufpos = '\0';

	return buf;
}

static espeak_ng_STATUS FileMemoryMap(const char *path, size_t *size, const void **ptr)
{
	int length = GetFileLength(path);
	if (length < 0)
		return (espeak_ng_STATUS)-length; // -length is error
	else if (length == 0)
		return ENS_UNEXPECTED_EOF;

	void *mem = malloc(length);
	if (mem == NULL)
		return (espeak_ng_STATUS)errno;

	FILE *file = fopen(path, "rb");
	if (file == NULL) {
		free(mem);
		return (espeak_ng_STATUS)errno;
	}

	size_t read = fread(mem, length, 1, file);
	fclose(file);

	if (read != 1) {
		free(mem);
		return ENS_UNEXPECTED_EOF;
	}

	// Only update target variables on success
	*ptr = mem;
	*size = length;

	return ENS_OK;
}

int DataGetFileLength(const char *path)
{
	const char *bundle_name = PathToBundleName(path);
	if (BundleInUse() && bundle_name)
		return BundleGetFileLength(bundle_name);
	else
		return GetFileLength(path);
}

FILE *DataFopen(const char *path)
{
	const char *bundle_name = PathToBundleName(path);
	if (BundleInUse() && bundle_name)
		return BundleFopen(bundle_name);
	else
		return fopen(path, "rb");
}

espeak_ng_STATUS DataMemoryMap(const char *path, size_t *size, const void **ptr)
{
	const char *bundle_name = PathToBundleName(path);
	if (BundleInUse() && bundle_name)
		return BundleMemoryMap(bundle_name, size, ptr);
	else
		return FileMemoryMap(path, size, ptr);
}

void DataMemoryFree(const void **ptr)
{
	if (BundleInUse())
		return;

	free((void*)*ptr);
	*ptr = NULL;
}
