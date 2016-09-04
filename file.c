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

static char *mount_points[] = {
	"app0:",
	"gro0:",
	"grw0:",
	"os0:",
	"pd0:",
	"sa0:",
	"savedata0:",
	"tm0:",
	"ud0:",
	"ur0:",
	"ux0:",
	"vd0:",
	"vs0:",
};

#define N_MOUNT_POINTS (sizeof(mount_points) / sizeof(char **))

int ReadFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int read = sceIoRead(fd, buf, size);

	sceIoClose(fd);
	return read;
}

int WriteFile(char *file, void *buf, int size) {
	SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	int written = sceIoWrite(fd, buf, size);

	sceIoClose(fd);
	return written;
}

int getFileSize(char *pInputFileName)
{
	SceUID fd = sceIoOpen(pInputFileName, SCE_O_RDONLY, 0);
	if (fd < 0)
		return fd;

	int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);
	
	sceIoClose(fd);
	return fileSize;
}

int getPathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files) {
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, MAX_PATH_LENGTH, "%s/%s", path, dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					int ret = getPathInfo(new_path, size, folders, files);
					if (ret <= 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				} else {
					if (size)
						(*size) += dir.d_stat.st_size;

					if (files)
						(*files)++;
				}

				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);

		if (folders)
			(*folders)++;
	} else {
		if (size) {
			SceIoStat stat;
			memset(&stat, 0, sizeof(SceIoStat));

			int res = sceIoGetstat(path, &stat);
			if (res < 0)
				return res;

			(*size) += stat.st_size;
		}

		if (files)
			(*files)++;
	}

	return 1;
}

int removePath(char *path, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* cancelHandler)()) {
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, MAX_PATH_LENGTH, "%s/%s", path, dir.d_name);

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					int ret = removePath(new_path, value, max, SetProgress, cancelHandler);
					if (ret <= 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}
				} else {
					int ret = sceIoRemove(new_path);
					if (ret < 0) {
						free(new_path);
						sceIoDclose(dfd);
						return ret;
					}

					if (value)
						(*value)++;

					if (SetProgress)
						SetProgress(value ? *value : 0, max);

					if (cancelHandler && cancelHandler()) {
						free(new_path);
						sceIoDclose(dfd);
						return 0;
					}
				}

				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);

		int ret = sceIoRmdir(path);
		if (ret < 0)
			return ret;

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		if (cancelHandler && cancelHandler()) {
			return 0;
		}
	} else {
		int ret = sceIoRemove(path);
		if (ret < 0)
			return ret;

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		if (cancelHandler && cancelHandler()) {
			return 0;
		}
	}

	return 1;
}

int copyFile(char *src_path, char *dst_path, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* cancelHandler)()) {
	// The source and destination paths are identical
	if (strcmp(src_path, dst_path) == 0) {
		return -1;
	}

	// The destination is a subfolder of the source folder
	int len = strlen(src_path);
	if (strncmp(src_path, dst_path, len) == 0 && dst_path[len] == '/') {
		return -1;
	}

	SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
	if (fdsrc < 0)
		return fdsrc;

	SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fddst < 0) {
		sceIoClose(fdsrc);
		return fddst;
	}

	void *buf = malloc(TRANSFER_SIZE);

	int read;
	while ((read = sceIoRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
		int res = sceIoWrite(fddst, buf, read);
		if (res < 0) {
			free(buf);

			sceIoClose(fddst);
			sceIoClose(fdsrc);

			return res;
		}

		if (value)
			(*value) += read;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		if (cancelHandler && cancelHandler()) {
			free(buf);

			sceIoClose(fddst);
			sceIoClose(fdsrc);

			return 0;
		}
	}

	free(buf);

	sceIoClose(fddst);
	sceIoClose(fdsrc);

	return 1;
}

int copyPath(char *src_path, char *dst_path, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* cancelHandler)()) {
	// The source and destination paths are identical
	if (strcmp(src_path, dst_path) == 0) {
		return -1;
	}

	// The destination is a subfolder of the source folder
	int len = strlen(src_path);
	if (strncmp(src_path, dst_path, len) == 0 && dst_path[len] == '/') {
		return -1;
	}

	SceUID dfd = sceIoDopen(src_path);
	if (dfd >= 0) {
		int ret = sceIoMkdir(dst_path, 0777);
		if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
			sceIoDclose(dfd);
			return ret;
		}

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		if (cancelHandler && cancelHandler()) {
			sceIoDclose(dfd);
			return 0;
		}

		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
				snprintf(new_src_path, MAX_PATH_LENGTH, "%s/%s", src_path, dir.d_name);

				char *new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
				snprintf(new_dst_path, MAX_PATH_LENGTH, "%s/%s", dst_path, dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = copyPath(new_src_path, new_dst_path, value, max, SetProgress, cancelHandler);
				} else {
					ret = copyFile(new_src_path, new_dst_path, value, max, SetProgress, cancelHandler);
				}

				free(new_dst_path);
				free(new_src_path);

				if (ret <= 0) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		return copyFile(src_path, dst_path, value, max, SetProgress, cancelHandler);
	}

	return 1;
}

typedef struct {
	char *extension;
	int type;
} ExtensionType;

static ExtensionType extension_types[] = {
	{ ".BMP",  FILE_TYPE_BMP },
	{ ".INI",  FILE_TYPE_INI },
	{ ".JPG",  FILE_TYPE_JPEG },
	{ ".JPEG", FILE_TYPE_JPEG },
	{ ".MP3",  FILE_TYPE_MP3 },
	{ ".PNG",  FILE_TYPE_PNG },
	{ ".SFO",  FILE_TYPE_SFO },
	{ ".TXT",  FILE_TYPE_TXT },
	{ ".VPK",  FILE_TYPE_VPK },
	{ ".XML",  FILE_TYPE_XML },
	{ ".ZIP",  FILE_TYPE_ZIP },
};

int getFileType(char *file) {
	char *p = strrchr(file, '.');
	if (p) {
		int i;
		for (i = 0; i < (sizeof(extension_types) / sizeof(ExtensionType)); i++) {
			if (strcasecmp(p, extension_types[i].extension) == 0) {
				return extension_types[i].type;
			}
		}
	}

	return FILE_TYPE_UNKNOWN;
}

int getNumberMountPoints() {
	return N_MOUNT_POINTS;
}

char **getMountPoints() {
	return mount_points;
}

FileListEntry *fileListFindEntry(FileList *list, char *name) {
	FileListEntry *entry = list->head;

	int name_length = strlen(name);

	while (entry) {
		if (entry->name_length == name_length && strcmp(entry->name, name) == 0)
			return entry;

		entry = entry->next;
	}

	return NULL;
}

FileListEntry *fileListGetNthEntry(FileList *list, int n) {
	FileListEntry *entry = list->head;

	while (n > 0 && entry) {
		n--;
		entry = entry->next;
	}

	if (n != 0)
		return NULL;

	return entry;
}

int fileListGetNumberByName(FileList *list, char *name) {
	FileListEntry *entry = list->head;

	int name_length = strlen(name);

	int n = 0;

	while (entry) {
		if (entry->name_length == name_length && strcmp(entry->name, name) == 0)
			break;

		n++;
		entry = entry->next;
	}

	return n;
}

void fileListAddEntry(FileList *list, FileListEntry *entry, int sort) {
	entry->next = NULL;
	entry->previous = NULL;

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		if (sort != SORT_NONE) {
			FileListEntry *p = list->head;
			FileListEntry *previous = NULL;

			while (p) {
				// Sort by type
				if (entry->is_folder > p->is_folder)
					break;

				// '..' is always at first
				if (strcmp(entry->name, "..") == 0)
					break;

				if (strcmp(p->name, "..") != 0) {
					// Get the minimum length without /
					int len = MIN(entry->name_length, p->name_length);
					if (entry->name[len - 1] == '/' || p->name[len - 1] == '/')
						len--;

					// Sort by name
					if (entry->is_folder == p->is_folder) {
						int diff = strncasecmp(entry->name, p->name, len);
						if (diff < 0 || (diff == 0 && entry->name_length < p->name_length)) {
							break;
						}
					}
				}

				previous = p;
				p = p->next;
			}

			if (previous == NULL) { // Order: entry (new head) -> p (old head)
				entry->next = p;
				p->previous = entry;
				list->head = entry;
			} else if (previous->next == NULL) { // Order: p (old tail) -> entry (new tail)
				FileListEntry *tail = list->tail;
				tail->next = entry;
				entry->previous = tail;
				list->tail = entry;
			} else { // Order: previous -> entry -> p
				previous->next = entry;
				entry->previous = previous; 
				entry->next = p;
				p->previous = entry;
			}
		} else {
			FileListEntry *tail = list->tail;
			tail->next = entry;
			entry->previous = tail;
			list->tail = entry;
		}
	}

	list->length++;
}

int fileListRemoveEntry(FileList *list, FileListEntry *entry) {
	if (entry) {
		if (entry->previous) {
			entry->previous->next = entry->next;
		} else {
			list->head = entry->next;
		}

		if (entry->next) {
			entry->next->previous = entry->previous;
		} else {
			list->tail = entry->previous;
		}

		list->length--;
		free(entry);

		return 1;
	}

	return 0;
}

int fileListRemoveEntryByName(FileList *list, char *name) {
	FileListEntry *entry = list->head;
	FileListEntry *previous = NULL;

	int name_length = strlen(name);

	while (entry) {
		if (entry->name_length == name_length && strcmp(entry->name, name) == 0) {
			if (previous) {
				previous->next = entry->next;
			} else {
				list->head = entry->next;
			}

			if (list->tail == entry) {
				list->tail = previous;
			}

			list->length--;

			free(entry);

			return 1;
		}

		previous = entry;
		entry = entry->next;
	}

	return 0;
}

void fileListEmpty(FileList *list) {
	FileListEntry *entry = list->head;

	while (entry) {
		FileListEntry *next = entry->next;
		free(entry);
		entry = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->length = 0;
	list->files = 0;
	list->folders = 0;
}

int fileListGetMountPointEntries(FileList *list) {
	int i;
	for (i = 0; i < N_MOUNT_POINTS; i++) {
		if (mount_points[i]) {
			SceIoStat stat;
			memset(&stat, 0, sizeof(SceIoStat));
			if (sceIoGetstat(mount_points[i], &stat) >= 0) {
				FileListEntry *entry = malloc(sizeof(FileListEntry));
				strcpy(entry->name, mount_points[i]);
				entry->name_length = strlen(entry->name);
				entry->is_folder = 1;
				entry->type = FILE_TYPE_UNKNOWN;

				memcpy(&entry->time, (SceDateTime *)&stat.st_ctime, sizeof(SceDateTime));

				fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

				list->folders++;
			}
		}
	}

	return 0;
}

int fileListGetDirectoryEntries(FileList *list, char *path) {
	SceUID dfd = sceIoDopen(path);
	if (dfd < 0)
		return dfd;

	FileListEntry *entry = malloc(sizeof(FileListEntry));
	strcpy(entry->name, DIR_UP);
	entry->name_length = strlen(entry->name);
	entry->is_folder = 1;
	entry->type = FILE_TYPE_UNKNOWN;
	fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);

	int res = 0;

	do {
		SceIoDirent dir;
		memset(&dir, 0, sizeof(SceIoDirent));

		res = sceIoDread(dfd, &dir);
		if (res > 0) {
			if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
				continue;

			FileListEntry *entry = malloc(sizeof(FileListEntry));

			strcpy(entry->name, dir.d_name);

			entry->is_folder = SCE_S_ISDIR(dir.d_stat.st_mode);
			if (entry->is_folder) {
				addEndSlash(entry->name);
				entry->type = FILE_TYPE_UNKNOWN;
				list->folders++;
			} else {
				entry->type = getFileType(entry->name);
				list->files++;
			}

			entry->name_length = strlen(entry->name);
			entry->size = dir.d_stat.st_size;

			memcpy(&entry->time, (SceDateTime *)&dir.d_stat.st_mtime, sizeof(SceDateTime));

			fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);
		}
	} while (res > 0);

	sceIoDclose(dfd);

	return 0;
}

int fileListGetEntries(FileList *list, char *path) {
	if (isInArchive()) {
		return fileListGetArchiveEntries(list, path);
	}

	if (strcmp(path, HOME_PATH) == 0) {
		return fileListGetMountPointEntries(list);
	}

	return fileListGetDirectoryEntries(list, path);
}
