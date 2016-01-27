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

#ifndef __PHOTO_H__
#define __PHOTO_H__

#define DEG_TO_RAD(x) ((float)x * M_PI / 180.0f)

enum PhotoModes {
	MODE_CUSTOM,
	MODE_FIT_HEIGHT,
	MODE_FIT_WIDTH,
};

#define ZOOM_MIN 0.1f
#define ZOOM_MAX 10.0f
#define ZOOM_FACTOR 1.02f
#define MOVE_INTERVAL 15.0f

int photoViewer(char *file, int type);

#endif