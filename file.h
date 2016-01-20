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

#ifndef __FILE_H__
#define __FILE_H__

#define MAX_PATH_LENGTH 1024
#define MAX_NAME_LENGTH 256
#define MAX_MOUNT_POINT_LENGTH 16

#define TRANSFER_SIZE 64 * 1024

#define HOME_PATH "home"
#define DIR_UP ".."

enum FileTypes {
	FILE_TYPE_UNKNOWN,
	FILE_TYPE_BMP,
	FILE_TYPE_ELF,
	FILE_TYPE_JPEG,
	FILE_TYPE_PNG,
	FILE_TYPE_PBP,
	FILE_TYPE_RAR,
	FILE_TYPE_7ZIP,
	FILE_TYPE_ZIP,
};

enum SortFlags {
	SORT_NONE,
	SORT_BY_NAME_AND_FOLDER,
};

typedef struct FileListEntry {
	struct FileListEntry *next;
	char name[MAX_NAME_LENGTH];
	int is_folder;
	int type;
	int size;
	SceRtcTime time;
} FileListEntry;

typedef struct {
	FileListEntry *head;
	FileListEntry *tail;
	int length;
} FileList;

int ReadFile(char *file, void *buf, int size);
int WriteFile(char *file, void *buf, int size);

int getPathInfo(char *path, uint32_t *size, uint32_t *folders, uint32_t *files);
int removePath(char *path, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max));
int copyPath(char *src, char *dst, uint32_t *value, uint32_t max, void (* SetProgress)(uint32_t value, uint32_t max));

int getFileType(char *file);

int getNumberMountPoints();
char **getMountPoints();
int addMountPoint(char *mount_point);
int replaceMountPoint(char *old_mount_point, char *new_mount_point);

FileListEntry *fileListFindEntry(FileList *list, char *name);
FileListEntry *fileListGetNthEntry(FileList *list, int n);
void fileListAddEntry(FileList *list, FileListEntry *entry, int sort);
int fileListRemoveEntry(FileList *list, char *name);
void fileListEmpty(FileList *list);
int fileListGetEntries(FileList *list, char *path);

#endif