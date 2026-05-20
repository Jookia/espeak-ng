#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
# Copyright 2026 Jookia <contact@jookia.org>

# This is a builder for a really simple bundle format specifically for
# espeak-ng-data files. This bundle is intended to be loaded in to read-only
# memory and read by espeak-ng directly without filesystem access.
#
# The bundle format looks like this:
# 
# - HEADER
# - One or more ENTRY structures
#
# The HEADER looks like this:
#
# - ASCII magic number: 'ESPK'
# - ASCII version number: '1'
# - 7 NULL bytes (reserved)
# - Unsigned 32-bit integer for FNV-1a checksum
# 
# An ENTRY looks like this:
# - NULL-terminated ASCII filename
# - Unsigned 32-bit integer for file length
# - NULL padding to 16 byte boundary
# - File data
# - NULL padding to 16 byte boundary
#
# All integers are stored big endian.
# Filenames use UNIX path slashes '/' not Windows slashes '\\'.
#
# The file data in the bundle must be aligned in memory at runtime. This is done
# by aligning the bundle itself in memory as well as the data within the bundle.
#
# espeak-ng expects the alignment to be at least what malloc() aligns to.
# The biggest alignment I know of is 16 bytes on most 64-bit systems, and as
# most developers are on 64-bit systems we shouldn't violate this assumption.
#
# Here is an example file hexdump:
#
#   45 53 50 4b 31 00 00 00  00 00 00 00 b8 09 d8 c5  |ESPK1...........|
#   66 69 6c 65 31 2e 74 78  74 00 00 00 00 0c 00 00  |file1.txt.......|
#   68 65 6c 6c 6f 20 77 6f  72 6c 64 0a 00 00 00 00  |hello world.....|
#   66 69 6c 65 32 2e 74 78  74 00 00 00 00 0d 00 00  |file2.txt.......|
#   68 6f 77 20 61 72 65 20  79 6f 75 3f 0a 00 00 00  |how are you?....|

import sys
import os

def find_bundle_files(data_dir):
    bundle_files = []
    for (root, dirs, files) in os.walk(data_dir):
        for file in files:
            filename = os.path.join(root, file)
            relative_name = filename[len(data_dir) + 1:]
            bundle_files.append(relative_name)

    # Sort files to aid in reproducibility
    bundle_files.sort()

    return bundle_files

def fnv1a(data):
	hash = 2166136261
	for b in data:
		hash = hash ^ b
		hash = hash * 16777619
		hash = hash & 0xFFFFFFFF # Clamp to 32 bits
	return hash

def make_bundle_header(bundle_data):
    bundle_fnv1a = fnv1a(bundle_data)

    bundle_header = bytearray()
    bundle_header += b'ESPK1'
    bundle_header += (b'\x00' * 7)
    bundle_header += int(bundle_fnv1a).to_bytes(4)

    return bundle_header

def pad_bytearray(array, pad):
    while len(array) % pad != 0:
        array += b'\x00'

def make_bundle_data(data_dir, bundle_files):
    bundle_data = bytearray()

    for filename in bundle_files:
        full_path = os.path.join(data_dir, filename)
        with open(full_path, 'rb') as f:
            file_data = f.read()
            file_data_len = len(file_data)

            # Write the file header
            bundle_data += filename.encode('utf-8')
            bundle_data += b'\x00' # NULL terminator
            bundle_data += int(file_data_len).to_bytes(4)
            pad_bytearray(bundle_data, 16)

            # Write the file data
            bundle_data += file_data
            pad_bytearray(bundle_data, 16)

    return bundle_data

def main(args):
    if len(args) != 3:
        print('Usage: bundle.py /path/to/espeak-ng-data data.bundle')
        return 1

    data_dir = args[1] # The espeak-ng-data directory
    bundle_file = args[2] # The data.bundle file to create

    bundle_files = find_bundle_files(data_dir)
    bundle_data = make_bundle_data(data_dir, bundle_files)
    bundle_header = make_bundle_header(bundle_data)
    with open(bundle_file, 'wb') as bundle_file:
        bundle_file.write(bundle_header)
        bundle_file.write(bundle_data)

    return 0

if __name__ == '__main__':
    sys.exit(main(sys.argv))
