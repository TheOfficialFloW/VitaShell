/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

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

int fileListGetArchiveEntries(FileList *list, char *path, int sort);

int getArchivePathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files);
int extractArchivePath(char *src, char *dst, FileProcessParam *param);

int archiveFileGetstat(const char *file, SceIoStat *stat);
int archiveFileOpen(const char *file, int flags, SceMode mode);
int archiveFileRead(SceUID fd, void *data, SceSize size);
int archiveFileClose(SceUID fd);

int ReadArchiveFile(char *file, void *buf, int size);

int archiveClose();
int archiveOpen(char *file);

int archiveCheckFilesForUnsafeFself();

#endif