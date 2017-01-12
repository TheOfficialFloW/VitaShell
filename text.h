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

#ifndef __TEXT_H__
#define __TEXT_H__

#define MAX_LINES 0x10000
#define MAX_LINE_CHARACTERS 1024
#define MAX_COPY_BUFFER_SIZE 1024

#define MAX_SELECTION 1024

#define TEXT_START_X 97.0f

#define MAX_SEARCH_RESULTS 1024 * 1024
#define MIN_SEARCH_TERM_LENGTH 1

typedef struct TextListEntry {
	struct TextListEntry *next;
	struct TextListEntry *previous;
	int line_number;
	int selected;
	char line[MAX_LINE_CHARACTERS];
} TextListEntry;

typedef struct {
	TextListEntry *head;
	TextListEntry *tail;
	int length;
} TextList;

typedef struct CopyEntry {
	char line[MAX_LINE_CHARACTERS];
} CopyEntry;

void initTextContextMenuWidth();

int textViewer(char *file);

#endif