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

#ifndef __INIT_H__
#define __INIT_H__

#include "file.h"

#define NET_INIT_SIZE 1 * 1024 * 1024

extern int language, enter_button, date_format, time_format;

typedef struct {
	char *path;
	void *buffer;
	int size;
	int replace;
} DefaultFile;

vita2d_pgf *loadSystemFonts();

void initVitaShell();
void finishVitaShell();

#endif