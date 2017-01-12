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

#ifndef __CONTEXT_MENU_H__
#define __CONTEXT_MENU_H__

#define RIGHT_ARROW "\xE2\x96\xB6"
#define LEFT_ARROW "\xE2\x97\x80"

enum ContextMenuVisibilities {
	CTX_VISIBILITY_UNUSED,
	CTX_VISIBILITY_INVISIBLE,
	CTX_VISIBILITY_VISIBLE,
};

typedef struct {
	int name;
	int more;
	int visibility;
} MenuEntry;

typedef struct {
	MenuEntry *menu_entries;
	int n_menu_entries;
	MenuEntry *menu_more_entries;
	int n_menu_more_entries;
	float menu_max_width;
	float menu_more_max_width;
	int (* menuEnterCallback)(int pos, void *context);
	int (* menuMoreEnterCallback)(int pos, void *context);
	void *context;
} ContextMenu;

enum ContextMenuModes {
	CONTEXT_MENU_CLOSED = 0,
	CONTEXT_MENU_CLOSING = 1,
	CONTEXT_MENU_OPENED = 2,
	CONTEXT_MENU_OPENING = 3,
	CONTEXT_MENU_MORE_CLOSED = 2,
	CONTEXT_MENU_MORE_CLOSING = 4,
	CONTEXT_MENU_MORE_OPENED = 5,
	CONTEXT_MENU_MORE_OPENING = 6,
};

int getContextMenuPos();
void setContextMenuPos(int pos);
int getContextMenuMorePos();
void setContextMenuMorePos(int pos);
int getContextMenuMode();
void setContextMenuMode(int mode);

void drawContextMenu(ContextMenu *ctx);
void contextMenuCtrl(ContextMenu *ctx);

#endif
