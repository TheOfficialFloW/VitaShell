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

typedef struct {
	char *mount_point;
	char *original_path;
} MountPoint;

static char *mount_points[] = {
	"app0:",
	"cache0:",
#ifdef USE_HOST0
	HOST0,
#endif
	"music0:",
	"photo0:",
	"sa0:",
	// "savedata0:",

	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
};

/*
	sa0:
	ux0:app/XXXXXXXXX/
	ux0:cache/XXXXXXXXX/
	ux0:music/
	ux0:picture/
	ux0:pspemu/
	"vs0:data/external"
	"vs0:sys/external"
*/

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

int getPathInfo(char *path, uint32_t *size, uint32_t *folders, uint32_t *files) {
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

				getPathInfo(new_path, size, folders, files);

				free(new_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);

		(*folders)++;
	}
	else
	{
		SceIoStat stat;
		memset(&stat, 0, sizeof(SceIoStat));

		int res = sceIoGetstat(path, &stat);
		if (res < 0)
			return res;

		(*size) += stat.st_size;
		(*files)++;
	}

	return 0;
}

int removePath(char *path, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max)) {
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
	
				removePath(new_path, value, max, SetProgress);

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
			SetProgress(*value, max);
	} else {
		int ret = sceIoRemove(path);
		if (ret < 0)
			return ret;

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(*value, max);
	}

	return 0;
}

int copyPath(char *src, char *dst, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max)) {
	SceUID dfd = sceIoDopen(src);
	if (dfd >= 0) {
		int ret = sceIoMkdir(dst, 0777);
		if (ret < 0 && ret != 0x80010011) {
			sceIoDclose(dfd);
			return ret;
		}

		if (value)
			(*value)++;

		if (SetProgress)
			SetProgress(*value, max);

		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (strcmp(dir.d_name, ".") == 0 || strcmp(dir.d_name, "..") == 0)
					continue;

				char *src_path = malloc(strlen(src) + strlen(dir.d_name) + 2);
				snprintf(src_path, MAX_PATH_LENGTH, "%s/%s", src, dir.d_name);

				char *dst_path = malloc(strlen(dst) + strlen(dir.d_name) + 2);
				snprintf(dst_path, MAX_PATH_LENGTH, "%s/%s", dst, dir.d_name);

				copyPath(src_path, dst_path, value, max, SetProgress);

				free(dst_path);
				free(src_path);
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		SceUID fdsrc = sceIoOpen(src, SCE_O_RDONLY, 0);
		if (fdsrc < 0)
			return fdsrc;

		SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (fddst < 0) {
			sceIoClose(fdsrc);
			return fddst;
		}

		void *buf = malloc(TRANSFER_SIZE);

		int read;
		while ((read = sceIoRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
			sceIoWrite(fddst, buf, read);

			if (value)
				(*value) += read;

			if (SetProgress)
				SetProgress(*value, max);
		}

		free(buf);

		sceIoClose(fddst);
		sceIoClose(fdsrc);
	}

	return 0;
}

typedef struct {
	char *extension;
	int type;
} ExtensionType;

ExtensionType extension_types[] = {
	{ ".7Z",   FILE_TYPE_7ZIP },
	{ ".BMP",  FILE_TYPE_BMP },
	{ ".ELF",  FILE_TYPE_ELF },
	{ ".JPG",  FILE_TYPE_JPEG },
	{ ".JPEG", FILE_TYPE_JPEG },
	{ ".PBP",  FILE_TYPE_PBP },
	{ ".PNG",  FILE_TYPE_PNG },
	{ ".RAR",  FILE_TYPE_RAR },
	{ ".SELF", FILE_TYPE_ELF },
	//{ ".TXT", FILE_TYPE_TXT },
	{ ".VELF", FILE_TYPE_ELF },
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

int addMountPoint(char *mount_point) {
	if (mount_point[0] == '\0')
		return 0;

	int i;
	for (i = 0; i < N_MOUNT_POINTS; i++) {
		if (mount_points[i] == NULL) {
			mount_points[i] = mount_point;
			return 1;
		}
	}

	return 0;
}

int replaceMountPoint(char *old_mount_point, char *new_mount_point) {
	int i;
	for (i = 0; i < N_MOUNT_POINTS; i++) {
		if (strcmp(mount_points[i], old_mount_point) == 0) {
			mount_points[i] = new_mount_point;
			return 1;
		}
	}

	return 0;
}

FileListEntry *fileListFindEntry(FileList *list, char *name) {
	FileListEntry *entry = list->head;

	while (entry) {
		if (strcmp(entry->name, name) == 0)
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

void fileListAddEntry(FileList *list, FileListEntry *entry, int sort) {
	entry->next = NULL;
	entry->previous = NULL;

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		if (sort != SORT_NONE) {
			int entry_length = strlen(entry->name);

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
					int name_length = strlen(p->name);
					int len = MIN(entry_length, name_length);
					if (entry->name[len - 1] == '/' || p->name[len - 1] == '/')
						len--;

					// Sort by name
					if (entry->is_folder == p->is_folder) {
						int diff = strncasecmp(entry->name, p->name, len);
						if (diff < 0 || (diff == 0 && entry_length < name_length)) {
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

	while (entry) {
		if (strcmp(entry->name, name) == 0) {
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
}

int fileListGetMountPointEntries(FileList *list) {
	int i;
	for (i = 0; i < N_MOUNT_POINTS; i++) {
		if (mount_points[i]) {
			FileListEntry *entry = malloc(sizeof(FileListEntry));
			strcpy(entry->name, mount_points[i]);
			entry->is_folder = 1;
			entry->type = FILE_TYPE_UNKNOWN;

			SceIoStat stat;
			memset(&stat, 0, sizeof(SceIoStat));
			sceIoGetstat(entry->name, &stat);
			memcpy(&entry->time, (SceRtcTime *)&stat.st_mtime, sizeof(SceRtcTime));

			fileListAddEntry(list, entry, SORT_BY_NAME_AND_FOLDER);
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
			} else {
/*
				char file[MAX_PATH_LENGTH];
				snprintf(file, MAX_PATH_LENGTH, "%s%s", path, entry->name);
				entry->type = getFileType(file);
*/
				entry->type = FILE_TYPE_UNKNOWN;
			}

			memcpy(&entry->time, (SceRtcTime *)&dir.d_stat.st_mtime, sizeof(SceRtcTime));

			entry->size = dir.d_stat.st_size;

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