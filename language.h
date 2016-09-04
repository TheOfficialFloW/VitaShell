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

#ifndef __LANGUAGE_H__
#define __LANGUAGE_H__

enum LanguageContainer {
	ERROR,
	OK,
	YES,
	NO,
	CANCEL,
	OFFSET,
	MARK_ALL,
	UNMARK_ALL,
	MOVE,
	COPY,
	PASTE,
	DELETE,
	RENAME,
	NEW_FOLDER,
	FOLDER,
	COPIED_FILE,
	COPIED_FOLDER,
	COPIED_FILES_FOLDERS,
	MOVING,
	COPYING,
	DELETING,
	INSTALLING,
	DOWNLOADING,
	EXTRACTING,
	DELETE_FILE_QUESTION,
	DELETE_FOLDER_QUESTION,
	DELETE_FILES_FOLDERS_QUESTION,
	INSTALL_ALL_QUESTION,
	INSTALL_QUESTION,
	INSTALL_WARNING,
	SAVE_MODIFICATIONS,
	WIFI_ERROR,
	FTP_SERVER,
	SYS_INFO,
	INSTALL_ALL,
	UPDATE_QUESTION,
	LANGUAGE_CONTRAINER_SIZE,
};

extern char *language_container[LANGUAGE_CONTRAINER_SIZE];

void freeLanguageContainer();
void loadLanguage(int id);

#endif
