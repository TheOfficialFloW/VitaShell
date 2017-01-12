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

#ifndef __LIST_DIALOG_H__
#define __LIST_DIALOG_H__

#define LIST_MAX_ENTRY 5

#define UP_ARROW "\xe2\x96\xb2"
#define DOWN_ARROW "\xe2\x96\xbc"

typedef struct
{
	int enter_pressed;
	int animation_mode;
	int status;
	float x;
	float y;
	float width;
	float height;
	float scale;
} ListDialog;

typedef struct {
	int name;
} ListEntry;

typedef struct {
	int title;
	ListEntry *list_entries;
	int n_list_entries;
	int can_escape;
	int (* listSelectCallback)(int pos);
} List;

enum ListDialogState
{
	LIST_DIALOG_OPEN,
	LIST_DIALOG_CLOSE
};

enum ListDialogCall
{
	LIST_CALL_NONE,
	LIST_CALL_CLOSE
};

enum ListAnimationMode {
	LIST_DIALOG_CLOSED,
	LIST_DIALOG_CLOSING,
	LIST_DIALOG_OPENED,
	LIST_DIALOG_OPENING,
};

int getListDialogMode();
void loadListDialog(List* list);
void drawListDialog();
void listDialogCtrl();

#endif