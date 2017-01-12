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

#ifndef __HEX_H__
#define __HEX_H__

// TODO
enum GroupSizes {
	GROUP_SIZE_1_BYTE,
	GROUP_SIZE_2_BYTE,
	GROUP_SIZE_4_BYTE,
};

typedef struct HexListEntry {
	struct HexListEntry *next;
	struct HexListEntry *previous;
	uint8_t data[0x10];
} HexListEntry;

typedef struct {
	HexListEntry *head;
	HexListEntry *tail;
	int length;
} HexList;

int hexViewer(char *file);

#endif