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
	OFFSET,
	MARK_ALL,
	UNMARK_ALL,
	MOVE,
	COPY,
	PASTE,
	DELETE,
	RENAME,
	NEW_FOLDER,
	SPLIT,
	JOIN,
	FOLDER,
	COPIED_FILE,
	COPIED_FOLDER,
	COPIED_FILES_FOLDERS,
	MOVING,
	COPYING,
	DELETING,
	SPLITTING,
	JOINING,
	DELETE_FILE_QUESTION,
	DELETE_FOLDER_QUESTION,
	DELETE_FILES_FOLDERS_QUESTION,
	SPLIT_QUESTION,
	JOIN_QUESTION,
	SIGN_QUESTION,
	SIGN_ERROR,
	SIGN_SUCCESS,
	COMPILE_SUCCESS,
	COMPILE_ERROR,
	WIFI_ERROR,
	FTP_SERVER,
	WAIT_CONNECTION,
	LANGUAGE_CONTRAINER_SIZE,
};

extern char *language_container[LANGUAGE_CONTRAINER_SIZE];

void freeLanguageContainer();
void loadLanguage(int id);

#endif