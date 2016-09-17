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

#include "main.h"
#include "config.h"
#include "language.h"

INCLUDE_EXTERN_RESOURCE(english_us_txt);

static char *lang[] ={
	"japanese",
	"english_us",
	"french",
	"spanish",
	"german",
	"italian",
	"dutch",
	"portuguese",
	"russian",
	"korean",
	"chinese_t",
	"chinese_s",
	"finnish",
	"swedish",
	"danish",
	"norwegian",
	"polish",
	"portuguese_br",
	"turkish"
};

char *language_container[LANGUAGE_CONTRAINER_SIZE];

void freeLanguageContainer() {
	int i;
	for (i = 0; i < LANGUAGE_CONTRAINER_SIZE; i++) {
		if (language_container[i]) {
			free(language_container[i]);
			language_container[i] = NULL;
		}
	}
}

void loadLanguage(int id) {
	freeLanguageContainer();

	#define LANGUAGE_ENTRY(name) { #name, CONFIG_TYPE_STRING, (void *)&language_container[name] }
	ConfigEntry language_entries[] = {
		// General strings
		LANGUAGE_ENTRY(ERROR),
		LANGUAGE_ENTRY(OK),
		LANGUAGE_ENTRY(YES),
		LANGUAGE_ENTRY(NO),
		LANGUAGE_ENTRY(CANCEL),
		LANGUAGE_ENTRY(FOLDER),
		LANGUAGE_ENTRY(OFFSET),

		// Progress strings
		LANGUAGE_ENTRY(MOVING),
		LANGUAGE_ENTRY(COPYING),
		LANGUAGE_ENTRY(DELETING),
		LANGUAGE_ENTRY(EXPORTING),
		LANGUAGE_ENTRY(INSTALLING),
		LANGUAGE_ENTRY(DOWNLOADING),
		LANGUAGE_ENTRY(EXTRACTING),
		LANGUAGE_ENTRY(HASHING),

		// Audio player strings
		LANGUAGE_ENTRY(TITLE),
		LANGUAGE_ENTRY(ALBUM),
		LANGUAGE_ENTRY(ARTIST),
		LANGUAGE_ENTRY(GENRE),
		LANGUAGE_ENTRY(YEAR),

		// Hex editor strings
		LANGUAGE_ENTRY(CUT),
		LANGUAGE_ENTRY(OPEN_HEX_EDITOR),

		// Text editor strings
		LANGUAGE_ENTRY(EDIT_LINE),
		LANGUAGE_ENTRY(ENTER_SEARCH_TERM),
		LANGUAGE_ENTRY(CUT),
		LANGUAGE_ENTRY(INSERT_EMPTY_LINE),

		// File browser context menu strings
		LANGUAGE_ENTRY(MORE),
		LANGUAGE_ENTRY(MARK_ALL),
		LANGUAGE_ENTRY(UNMARK_ALL),
		LANGUAGE_ENTRY(MOVE),
		LANGUAGE_ENTRY(COPY),
		LANGUAGE_ENTRY(PASTE),
		LANGUAGE_ENTRY(DELETE),
		LANGUAGE_ENTRY(RENAME),
		LANGUAGE_ENTRY(NEW_FOLDER),
		LANGUAGE_ENTRY(INSTALL_ALL),
		LANGUAGE_ENTRY(CALCULATE_SHA1),
		LANGUAGE_ENTRY(EXPORT_MEDIA),
		LANGUAGE_ENTRY(SEARCH),

		// Tools context menu strings
		LANGUAGE_ENTRY(SYS_INFO_TITLE),

		// File browser strings
		LANGUAGE_ENTRY(COPIED_FILE),
		LANGUAGE_ENTRY(COPIED_FOLDER),
		LANGUAGE_ENTRY(COPIED_FILES_FOLDERS),

		// Dialog questions
		LANGUAGE_ENTRY(DELETE_FILE_QUESTION),
		LANGUAGE_ENTRY(DELETE_FOLDER_QUESTION),
		LANGUAGE_ENTRY(DELETE_FILES_FOLDERS_QUESTION),
		LANGUAGE_ENTRY(EXPORT_FILE_QUESTION),
		LANGUAGE_ENTRY(EXPORT_FOLDER_QUESTION),
		LANGUAGE_ENTRY(EXPORT_FILES_FOLDERS_QUESTION),
		LANGUAGE_ENTRY(EXPORT_NO_MEDIA),
		LANGUAGE_ENTRY(EXPORT_SONGS_INFO),
		LANGUAGE_ENTRY(EXPORT_PICTURES_INFO),
		LANGUAGE_ENTRY(EXPORT_SONGS_PICTURES_INFO),
		LANGUAGE_ENTRY(INSTALL_ALL_QUESTION),
		LANGUAGE_ENTRY(INSTALL_QUESTION),
		LANGUAGE_ENTRY(INSTALL_WARNING),
		LANGUAGE_ENTRY(HASH_FILE_QUESTION),

		// Others
		LANGUAGE_ENTRY(PLEASE_WAIT),
		LANGUAGE_ENTRY(SAVE_MODIFICATIONS),
		LANGUAGE_ENTRY(NO_SPACE_ERROR),
		LANGUAGE_ENTRY(WIFI_ERROR),
		LANGUAGE_ENTRY(FTP_SERVER),
		LANGUAGE_ENTRY(SYS_INFO),
		LANGUAGE_ENTRY(UPDATE_QUESTION),
	};

	// Load default config file
	readConfigBuffer(&_binary_resources_english_us_txt_start, (int)&_binary_resources_english_us_txt_size, language_entries, sizeof(language_entries) / sizeof(ConfigEntry));

	// Load custom config file
	if (use_custom_config) {
		if (id >= 0 && id < (sizeof(lang) / sizeof(char *))) {
			char path[128];
			sprintf(path, "ux0:VitaShell/language/%s.txt", lang[id]);
			readConfig(path, language_entries, sizeof(language_entries) / sizeof(ConfigEntry));
		}
	}
}
