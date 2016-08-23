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

#include "minizip/unzip.h"

static int archive_path_start = 0;
static unzFile uf = NULL;
static FileList archive_list;

int fileListGetArchiveEntries(FileList *list, char *path) {
	int res;

	if (!uf)
		return -1;

	FileListEntry *entry = malloc(sizeof(FileListEntry));
	strcpy(entry->name, DIR_UP);
	entry->name_length = strlen(entry->name);
	entry->is_folder = 1;
	entry->type = FILE_TYPE_UNKNOWN;
	fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

	char *archive_path = path + archive_path_start;
	int name_length = strlen(archive_path);

	FileListEntry *archive_entry = archive_list.head;

	int i;
	for (i = 0; i < archive_list.length; i++) {
		if (archive_entry->name_length >= name_length && strncmp(archive_entry->name, archive_path, name_length) == 0) { // Needs a / at end
			char *p = strchr(archive_entry->name + name_length, '/'); // it's a sub-directory if it has got a slash

			if (p)
				*p = '\0';

			char name[MAX_PATH_LENGTH];
			strcpy(name, archive_entry->name + name_length);
			if (p) {
				addEndSlash(name);
			}

			if (strlen(name) > 0 && !fileListFindEntry(list, name)) {
				FileListEntry *entry = malloc(sizeof(FileListEntry));

				strcpy(entry->name, archive_entry->name + name_length);

				if (p) {
					addEndSlash(entry->name);
					entry->is_folder = 1;
					entry->type = FILE_TYPE_UNKNOWN;
					list->folders++;
				} else {
					entry->is_folder = 0;
					entry->type = getFileType(entry->name);
					list->files++;
				}

				entry->name_length = strlen(entry->name);
				entry->size = archive_entry->size;

				memcpy(&entry->time, &archive_entry->time, sizeof(SceDateTime));

				fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);
			}

			if (p)
				*p = '/';
		}

		// Next
		archive_entry = archive_entry->next;
	}

	return 0;
}

int getArchivePathInfo(char *path, uint32_t *size, uint32_t *folders, uint32_t *files) {
	if (!uf)
		return -1;

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (archiveFileGetstat(path, &stat) < 0) {
		FileList list;
		memset(&list, 0, sizeof(FileList));
		fileListGetArchiveEntries(&list, path);

		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *new_path = malloc(strlen(path) + strlen(entry->name) + 2);
			snprintf(new_path, MAX_PATH_LENGTH, "%s%s", path, entry->name);

			getArchivePathInfo(new_path, size, folders, files);

			free(new_path);

			entry = entry->next;
		}

		if (folders)
			(*folders)++;

		fileListEmpty(&list);
	} else {
		if (size)
			(*size) += stat.st_size;

		if (files)
			(*files)++;
	}

	return 0;
}

int extractArchivePath(char *src, char *dst, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max), int (* cancelHandler)()) {
	if (!uf)
		return -1;

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (archiveFileGetstat(src, &stat) < 0) {
		FileList list;
		memset(&list, 0, sizeof(FileList));
		fileListGetArchiveEntries(&list, src);

		int ret = sceIoMkdir(dst, 0777);
		if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
			fileListEmpty(&list);
			return ret;
		}

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);
		
		if (cancelHandler && cancelHandler()) {
			fileListEmpty(&list);
			return 0;
		}

		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *src_path = malloc(strlen(src) + strlen(entry->name) + 2);
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", src, entry->name);

			char *dst_path = malloc(strlen(dst) + strlen(entry->name) + 2);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", dst, entry->name);

			int ret = extractArchivePath(src_path, dst_path, value, max, SetProgress, cancelHandler);

			free(dst_path);
			free(src_path);

			if (ret <= 0) {
				fileListEmpty(&list);
				return ret;
			}

			entry = entry->next;
		}
		
		fileListEmpty(&list);
	} else {
		SceUID fdsrc = archiveFileOpen(src, SCE_O_RDONLY, 0);
		if (fdsrc < 0)
			return fdsrc;

		SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fddst < 0) {
			archiveFileClose(fdsrc);
			return fddst;
		}

		void *buf = malloc(TRANSFER_SIZE);

		int read;
		while ((read = archiveFileRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
			int res = sceIoWrite(fddst, buf, read);
			if (res < 0) {
				free(buf);

				sceIoClose(fddst);
				archiveFileClose(fdsrc);

				return res;
			}

			if (value)
				(*value) += read;

			if (SetProgress)
				SetProgress(value ? *value : 0, max);

			if (cancelHandler && cancelHandler()) {
				free(buf);

				sceIoClose(fddst);
				archiveFileClose(fdsrc);

				return 0;
			}
		}

		free(buf);

		sceIoClose(fddst);
		archiveFileClose(fdsrc);
	}

	return 1;
}

int archiveFileGetstat(const char *file, SceIoStat *stat) {
	if (!uf)
		return -1;

	const char *archive_path = file + archive_path_start;
	int name_length = strlen(archive_path);

	FileListEntry *archive_entry = archive_list.head;

	int i;
	for (i = 0; i < archive_list.length; i++) {
		if (archive_entry->name_length == name_length && strcmp(archive_entry->name, archive_path) == 0) {
			if (stat) {
				// TODO: more stat
				stat->st_size = archive_entry->size;
			}

			return 0;
		}

		// Next
		archive_entry = archive_entry->next;
	}

	return -1;
}

int archiveFileOpen(const char *file, int flags, SceMode mode) {
	int res;

	if (!uf)
		return -1;

	const char *archive_path = file + archive_path_start;	
	int name_length = strlen(archive_path);

	FileListEntry *archive_entry = archive_list.head;

	int i;
	for (i = 0; i < archive_list.length; i++) {
		if (archive_entry->name_length == name_length && strcmp(archive_entry->name, archive_path) == 0) {
			// Set pos
			unzGoToFilePos64(uf, (unz64_file_pos *)&archive_entry->reserved);

			// File info
			char name[MAX_PATH_LENGTH];
			unz_file_info64 file_info;
			unzGetCurrentFileInfo64(uf, &file_info, name, MAX_PATH_LENGTH, NULL, 0, NULL, 0);
			
			res = unzOpenCurrentFile(uf);
			if (res < 0)
				return res;

			return ARCHIVE_FD;
		}

		// Next
		archive_entry = archive_entry->next;
	}

	return -1;
}

int archiveFileRead(SceUID fd, void *data, SceSize size) {
	if (!uf || fd != ARCHIVE_FD)
		return -1;

	return unzReadCurrentFile(uf, data, size);
}

int archiveFileClose(SceUID fd) {
	if (!uf || fd != ARCHIVE_FD)
		return -1;

	return unzCloseCurrentFile(uf);
}

int ReadArchiveFile(char *file, void *buf, int size) {
	SceUID fd = archiveFileOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int read = archiveFileRead(fd, buf, size);
	archiveFileClose(fd);
	return read;
}

int archiveClose() {
	if (!uf)
		return -1;

	fileListEmpty(&archive_list);

	unzClose(uf);
	uf = NULL;

	return 0;
}

// TODO: improve this, it's slow...
int archiveOpen(char *file) {
	int res;

	// Start position of the archive path
	archive_path_start = strlen(file) + 1;

	// Close previous zip file first
	if (uf)
		unzClose(uf);

	// Open zip file
	uf = unzOpen64(file);
	if (!uf)
		return -1;

	// Go to first entry
	res = unzGoToFirstFile(uf);
    if (res < 0)
		return res;

	// Clear archive list
	memset(&archive_list, 0, sizeof(FileList));

	// Go through all files
	do {
		FileListEntry *entry = malloc(sizeof(FileListEntry));

		// File info
		unz_file_info64 file_info;
		unzGetCurrentFileInfo64(uf, &file_info, entry->name, MAX_PATH_LENGTH, NULL, 0, NULL, 0);

		entry->is_folder = 0;
		entry->name_length = file_info.size_filename;
		entry->size = file_info.uncompressed_size;
		sceRtcSetDosTime(&entry->time, file_info.dosDate);

		// Get pos
		unzGetFilePos64(uf, (unz64_file_pos *)&entry->reserved);

		// Add entry
		fileListAddEntry(&archive_list, entry, SORT_BY_NAME_AND_FOLDER);

		// Next
        res = unzGoToNextFile(uf);
    } while (res >= 0);

	return 0;
}