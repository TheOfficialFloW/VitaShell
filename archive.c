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

#include "main.h"
#include "archive.h"
#include "file.h"
#include "utils.h"

static int path_length = 0;

static fex_t *fex = NULL;
static fex_pos_t archive_file_pos = 0;
static int archive_file_size = 0;

int fileListGetArchiveEntries(FileList *list, char *path) {
	FileListEntry *entry = malloc(sizeof(FileListEntry));
	strcpy(entry->name, DIR_UP);
	entry->is_folder = 1;
	entry->type = FILE_TYPE_UNKNOWN;
	fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

	char *archive_path = path + path_length;
	int archive_path_length = strlen(archive_path);

	fex_rewind(fex);

	int count = 0;

	while (!fex_done(fex)) {
		fex_stat(fex);

		const char *name = fex_name(fex);
		int size = fex_size(fex);

		if (strncmp(name, archive_path, archive_path_length) == 0) { // Needs a / at end
			char *p = strchr(name + archive_path_length, '/');

			if (p)
				*p = '\0';

			FileListEntry *entry = malloc(sizeof(FileListEntry));

			strcpy(entry->name, name + archive_path_length);

			if (p) {
				entry->is_folder = 1;
				addEndSlash(entry->name);
			} else {
				entry->is_folder = 0;
			}

			entry->type = FILE_TYPE_UNKNOWN;

			entry->size = size;

			sceRtcSetDosTime(&entry->time, fex_dos_date(fex));

			if (!fileListFindEntry(list, entry->name)) {
				fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);
				count++;
			}

			if (p)
				*p = '/';
		}

		fex_next(fex);
	}

	return count;
}

int getArchivePathInfo(char *path, uint32_t *size, uint32_t *folders, uint32_t *files) {
	FileList list;
	memset(&list, 0, sizeof(FileList));
	int count = fileListGetArchiveEntries(&list, path);

	if (count > 0) {
		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *new_path = malloc(strlen(path) + strlen(entry->name) + 2);
			snprintf(new_path, MAX_PATH_LENGTH, "%s%s", path, entry->name);

			addEndSlash(new_path);

			getArchivePathInfo(new_path, size, folders, files);

			free(new_path);

			entry = entry->next;
		}

		if (folders)
			(*folders)++;
	} else {
		removeEndSlash(path);

		SceUID fd = archiveFileOpen(path);
		if (fd < 0)
			return fd;

		if (size)
			(*size) += archiveFileGetSize(fd);

		if (files)
			(*files)++;

		archiveFileClose(fd);
	}

	fileListEmpty(&list);

	return 0;
}

int extractArchivePath(char *src, char *dst, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max)) {
	FileList list;
	memset(&list, 0, sizeof(FileList));
	int count = fileListGetArchiveEntries(&list, src);

	if (count > 0) {
		int ret = sceIoMkdir(dst, 0777);
		if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
			fileListEmpty(&list);
			return ret;
		}

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *src_path = malloc(strlen(src) + strlen(entry->name) + 2);
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", src, entry->name);

			char *dst_path = malloc(strlen(dst) + strlen(entry->name) + 2);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", dst, entry->name);

			addEndSlash(src_path);
			addEndSlash(dst_path);

			extractArchivePath(src_path, dst_path, value, max, SetProgress);

			free(dst_path);
			free(src_path);

			entry = entry->next;
		}
	} else {
		removeEndSlash(src);
		removeEndSlash(dst);

		SceUID fdsrc = archiveFileOpen(src);
		if (fdsrc < 0) {
			fileListEmpty(&list);
			return fdsrc;
		}

		SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fddst < 0) {
			archiveFileClose(fdsrc);
			fileListEmpty(&list);
			return fddst;
		}

		void *buf = malloc(TRANSFER_SIZE);

		int read;
		while ((read = archiveFileRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
			sceIoWrite(fddst, buf, read);

			if (value)
				(*value) += read;

			if (SetProgress)
				SetProgress(value ? *value : 0, max);
		}

		free(buf);

		sceIoClose(fddst);
		archiveFileClose(fdsrc);
	}

	fileListEmpty(&list);

	return 0;
}

int archiveFileOpen(char *path) {
	char *archive_path = path + path_length;

	if (!fex_done(fex)) {
		if (strcmp(fex_name(fex), archive_path) == 0) {
			archive_file_pos = fex_tell_arc(fex);
			archive_file_size = archiveFileGetSize(ARCHIVE_FD);
			return ARCHIVE_FD;
		}
	}

	fex_rewind(fex);

	while (!fex_done(fex)) {
		if (strcmp(fex_name(fex), archive_path) == 0) {
			archive_file_pos = fex_tell_arc(fex);
			archive_file_size = archiveFileGetSize(ARCHIVE_FD);
			return ARCHIVE_FD;
		}

		fex_next(fex);
	}

	return -1;
}

int archiveFileGetSize(SceUID fd) {
	if (fd != ARCHIVE_FD)
		return -1;

	fex_stat(fex);
	return fex_size(fex);
}

int archiveFileSeek(SceUID fd, int n) {
	if (fd != ARCHIVE_FD)
		return -1;

	archiveFileClose(fd);

	void *buf = malloc(TRANSFER_SIZE);

	int remain = 0, seek = 0;
	while ((remain = n - seek) > 0) {
		remain = MIN(remain, TRANSFER_SIZE);

		fex_err_t res = fex_read(fex, buf, remain);
		if (res != 0) {
			free(buf);
			return -fex_err_code(res);
		}

		seek += remain;
	}

	free(buf);
	return 0;
}

int archiveFileRead(SceUID fd, void *data, int n) {
	if (fd != ARCHIVE_FD)
		return -1;

	int previous_pos = fex_tell(fex);

	int remain = MIN(archive_file_size - previous_pos, n);

	fex_err_t res = fex_read(fex, data, remain);
	if (res != NULL)
		return -fex_err_code(res);

	return remain;
}

int archiveFileClose(SceUID fd) {
	if (fd != ARCHIVE_FD)
		return -1;

	fex_seek_arc(fex, archive_file_pos);

	return 0;
}

int ReadArchiveFile(char *file, void *buf, int size) {
	SceUID fd = archiveFileOpen(file);
	if (fd < 0)
		return fd;

	int read = archiveFileRead(fd, buf, size);
	archiveFileClose(fd);
	return read;
}

void archiveClose() {
	fex_close(fex);
	fex = NULL;
}

int archiveOpen(char *file) {
	path_length = strlen(file) + 1;

	if (fex)
		archiveClose();

	fex_err_t res = fex_open(&fex, file);
	if (res != NULL)
		return -fex_err_code(res);

	return 0;
}