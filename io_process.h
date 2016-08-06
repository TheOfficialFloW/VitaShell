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

#ifndef __IO_PROCESS_H__
#define __IO_PROCESS_H__

#include "file.h"

#define COUNTUP_WAIT 100 * 1000
#define DIALOG_WAIT 900 * 1000

#define COPY_MODE_NORMAL 0
#define COPY_MODE_MOVE 1
#define COPY_MODE_EXTRACT 2

typedef struct {
	uint32_t max;
} UpdateArguments;

typedef struct {
	FileList *file_list;
	FileList *mark_list;
	int index;
} DeleteArguments;

typedef struct {
	FileList *file_list;
	FileList *copy_list;
	char *archive_path;
	int copy_mode;
} CopyArguments;

void closeWaitDialog();
int cancelHandler();
void SetProgress(uint32_t value, uint32_t max);
SceUID createStartUpdateThread(uint32_t max);

int delete_thread(SceSize args_size, DeleteArguments *args);
int copy_thread(SceSize args_size, CopyArguments *args);

#endif