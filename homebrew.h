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

#ifndef __HOMEBREW_H__
#define __HOMEBREW_H__

#define INSERT_UID(uid_list, uid) \
{ \
	int i; \
	for (i = 0; i < MAX_UIDS; i++) { \
		if (uid_list[i] < 0) { \
			uid_list[i] = uid; \
			break; \
		} \
	} \
}

#define DELETE_UID(uid_list, uid) \
{ \
	int i; \
	for (i = 0; i < MAX_UIDS; i++) { \
		if (uid_list[i] == uid) { \
			uid_list[i] = INVALID_UID; \
			break; \
		} \
	} \
}

#define MAX_CODE_SIZE 8 * 1024 * 1024 // 12 * 1024 * 1024
#define MAX_UVL_SIZE 1 * 1024 * 1024

#define MAX_AUDIO_PORTS 3
#define MAX_SYNC_OBJECTS 3
#define MAX_GXM_PRGRAMS 16
#define MAX_UIDS 32
#define MAX_LW_MUTEXES 32

typedef struct {
	char *library;
	uint32_t nid;
	void *function;
} PatchNID;

typedef struct {
	uint32_t addr;
	uint32_t value;
	uint32_t stub;
	void *function;
} PatchValue;

void initCodeMemory(SceUID blockid);

int isValidElf(char *file);
void loadHomebrew(char *file);

int PatchUVL();
void restoreUVLPatches();

#endif