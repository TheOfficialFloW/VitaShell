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

#ifndef __PHOTO_H__
#define __PHOTO_H__

#include "file.h"

enum PhotoModes {
	MODE_CUSTOM,
	MODE_PERFECT,
	MODE_ORIGINAL,
	MODE_FIT_HEIGHT,
	MODE_FIT_WIDTH,
};

#define ZOOM_MIN 0.1f
#define ZOOM_MAX 1000.0f
#define ZOOM_FACTOR 1.02f

#define MOVE_DIVISION 7.0f

#define ZOOM_TEXT_TIME 2 * 1000 * 1000

int photoViewer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos);

#endif