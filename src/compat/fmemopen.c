// SPDX-License-Identifier: MIT
// Copyright 2026 Jookia <contact@jookia.org>

#if _WIN32

#include <stdio.h>
#include <windows.h>
#include <io.h>

FILE *fmemopen(void *buf, size_t size, const char *mode)
{
	(void)mode; // We must use w+b

	static char temp_path[MAX_PATH];
	static char temp_name[MAX_PATH];

	int path_len = GetTempPathA(sizeof(temp_path), temp_path);
	if (path_len == 0 || path_len > sizeof(temp_path))
		return NULL;

	int name_len = GetTempFileNameA(temp_path, NULL, 0, temp_name);
	if (name_len == 0)
		return NULL;

	HANDLE temp_file = CreateFileA(
		temp_name,
		(GENERIC_READ | GENERIC_WRITE),
		0,
		NULL,
		OPEN_EXISTING,
		FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE,
		NULL);
	if (temp_file == INVALID_HANDLE_VALUE)
		return NULL;

	int fd = _open_osfhandle((intptr_t)temp_file, 0);
	if (fd == -1) {
		CloseHandle(temp_file);
		return NULL;
	}

	FILE *file = fdopen(fd, "w+b");
	if (file == NULL) {
		_close(fd);
		return NULL;
	}

	if (fwrite(buf, size, 1, file) != 1) {
		fclose(file);
		return NULL;
	}

	rewind(file);

	return file;
}

#endif
