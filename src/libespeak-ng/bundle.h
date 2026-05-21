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

#ifndef ESPEAK_NG_BUNDLE_H
#define ESPEAK_NG_BUNDLE_H

#include <stdint.h>
#include <stdio.h>

// Locks the ability to change the currently set bundle
// Used to prevent changing the bundle while the library initialized
void BundleLock(bool lock);

// Queries whether a bundle is currently in use
// Used to decide whether to query the filesystem or bundle
bool BundleInUse(void);

// Information about a bundle entry
struct bundle_entry
{
	const uint8_t *name; // Entry UTF-8 name, NULL terminated
	const uint8_t *data; // File data for the entrty
	uint32_t data_len; // Length of the file data
	const uint8_t *next_header; // Pointer to the next entry header
};

// Finds a bundle_entry struct based on entry->next_header
// If next_header is NULL then the first data bundle entry will be used
// Returns true if an entry was found
//
// This function is intended to be used to iterate through bundle entries
// in a loop like this:
//
//   struct bundle_entry entry = {0};
//   while (BundleReadNext(&entry)) {
//     // Do something with the entry
//   }
bool BundleReadNext(struct bundle_entry *entry);

// Gets the length of an entry's data in a bundle
int BundleGetFileLength(const char *name);

// Opens an entry's data from the bundle
FILE *BundleFopen(const char *name);

// Maps an entry's data from the bundle 
// This returns read-only memory that must not be modified
espeak_ng_STATUS BundleMemoryMap(const char *name, size_t *size, const void **ptr);

#endif
