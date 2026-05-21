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

#ifndef ESPEAK_NG_DATA_H
#define ESPEAK_NG_DATA_H

#include <stdio.h>

// Gets the length of a data file
int DataGetFileLength(const char *path);

// Opens a read-only data file
FILE *DataFopen(const char *path);

#endif
