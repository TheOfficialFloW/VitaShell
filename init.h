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

#ifndef __INIT_H__
#define __INIT_H__

#include "file.h"

extern char pspemu_path[MAX_MOUNT_POINT_LENGTH], data_external_path[MAX_MOUNT_POINT_LENGTH], sys_external_path[MAX_MOUNT_POINT_LENGTH];
extern char mms_400_path[MAX_MOUNT_POINT_LENGTH], mms_401_path[MAX_MOUNT_POINT_LENGTH], mms_402_path[MAX_MOUNT_POINT_LENGTH];

extern int language, enter_button, date_format, time_format;

void initSceAppUtil();
void finishSceAppUtil();

void initVita2dLib();
void finishVita2dLib();

void addMountPoints();

int findSceSysmoduleFunctions();
int findScePafFunctions();

int initSceLibPgf();

void initVitaShell();
void finishVitaShell();

#endif