/*
	VitaShell
	Copyright (C) 2015-2016, TheFloW

	This program is free software: you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation, either version 3 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/

#ifndef __ARCHIVE_H__
#define __ARCHIVE_H__

#include "file.h"

#define ARCHIVE_FD 0x12345678

int fileListGetArchiveEntries(FileList *list, char *path);

int getArchivePathInfo(char *path, uint32_t *size, uint32_t *folders, uint32_t *files);
int extractArchivePath(char *src, char *dst, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max));

int archiveFileOpen(char *path);
int archiveFileGetSize(SceUID fd);
int archiveFileSeek(SceUID fd, int n);
int archiveFileRead(SceUID fd, void *data, int n);
int archiveFileClose(SceUID fd);

int ReadArchiveFile(char *file, void *buf, int size);

void archiveClose();
int archiveOpen(char *file);

#endif