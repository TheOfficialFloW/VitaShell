/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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

#ifndef __PROPERTY_DIALOG_H__
#define __PROPERTY_DIALOG_H__

#include "file.h"

enum PropertyEntryVisibilities {
  PROPERTY_ENTRY_UNUSED,
  PROPERTY_ENTRY_INVISIBLE,
  PROPERTY_ENTRY_VISIBLE,
};

enum PropertyDialogStatus {
  PROPERTY_DIALOG_CLOSED,
  PROPERTY_DIALOG_CLOSING,
  PROPERTY_DIALOG_OPENED,
  PROPERTY_DIALOG_OPENING,
};

int getPropertyDialogStatus();
int initPropertyDialog(char *path, FileListEntry *entry);
void propertyDialogCtrl();
void drawPropertyDialog();

#endif
