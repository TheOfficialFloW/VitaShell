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

/*
	TODO:
	- Hide mount points
	- Inverse sort, sort by date, size
	- Hex editor byte group size
	- Duplicate when same location or same name. /lol to /lol - Backup. or overwrite question.
	- Shortcuts
	- CPU changement
	- Media player
*/

#include "main.h"
#include "init.h"
#include "io_process.h"
#include "package_installer.h"
#include "network_update.h"
#include "context_menu.h"
#include "archive.h"
#include "photo.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "audioplayer.h"
#include "sfo.h"

int _newlib_heap_size_user = 64 * 1024 * 1024;

#define MAX_DIR_LEVELS 1024

// Context menu
static float ctx_menu_max_width = 0.0f, ctx_menu_more_max_width = 0.0f;

// File lists
static FileList file_list, mark_list, copy_list, install_list;

// Paths
static char cur_file[MAX_PATH_LENGTH], archive_path[MAX_PATH_LENGTH], install_path[MAX_PATH_LENGTH];

// Position
static int base_pos = 0, rel_pos = 0;
static int base_pos_list[MAX_DIR_LEVELS];
static int rel_pos_list[MAX_DIR_LEVELS];
static int dir_level = 0;

// Copy mode
static int copy_mode = COPY_MODE_NORMAL;

// Archive
static int is_in_archive = 0;
static int dir_level_archive = -1;

// FTP
static char vita_ip[16];
static unsigned short int vita_port;

// Enter and cancel buttons
int SCE_CTRL_ENTER = SCE_CTRL_CROSS, SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;

// Dialog step
volatile int dialog_step = DIALOG_STEP_NONE;

// Use custom config
int use_custom_config = 1;

void dirLevelUp() {
	base_pos_list[dir_level] = base_pos;
	rel_pos_list[dir_level] = rel_pos;
	dir_level++;
	base_pos_list[dir_level] = 0;
	rel_pos_list[dir_level] = 0;
	base_pos = 0;
	rel_pos = 0;
}

int isInArchive() {
	return is_in_archive;
}

void dirUpCloseArchive() {
	if (isInArchive() && dir_level_archive >= dir_level) {
		is_in_archive = 0;
		archiveClose();
		dir_level_archive = -1;
	}
}

void dirUp() {
	removeEndSlash(file_list.path);

	char *p;

	p = strrchr(file_list.path, '/');
	if (p) {
		p[1] = '\0';
		dir_level--;
		goto DIR_UP_RETURN;
	}

	p = strrchr(file_list.path, ':');
	if (p) {
		if (strlen(file_list.path) - ((p + 1) - file_list.path) > 0) {
			p[1] = '\0';
			dir_level--;
			goto DIR_UP_RETURN;
		}
	}

	strcpy(file_list.path, HOME_PATH);
	dir_level = 0;

DIR_UP_RETURN:
	base_pos = base_pos_list[dir_level];
	rel_pos = rel_pos_list[dir_level];
	dirUpCloseArchive();
}

void focusOnFilename(char *name) {
	int name_pos = fileListGetNumberByName(&file_list, name);
	if (name_pos < file_list.length) {
		while (1) {
			int index = base_pos + rel_pos;
			if (index == name_pos)
				break;

			if (index > name_pos) {
				if (rel_pos > 0) {
					rel_pos--;
				} else {
					if (base_pos > 0) {
						base_pos--;
					}
				}
			}

			if (index < name_pos) {
				if ((rel_pos + 1) < file_list.length) {
					if ((rel_pos + 1) < MAX_POSITION) {
						rel_pos++;
					} else {
						if ((base_pos + rel_pos + 1) < file_list.length) {
							base_pos++;
						}
					}
				}
			}
		}
	}
}

int refreshFileList() {
	int ret = 0, res = 0;

	do {
		fileListEmpty(&file_list);

		res = fileListGetEntries(&file_list, file_list.path);

		if (res < 0) {
			ret = res;
			dirUp();
		}
	} while (res < 0);

	// Correct position after deleting the latest entry of the file list
	while ((base_pos + rel_pos) >= file_list.length) {
		if (base_pos > 0) {
			base_pos--;
		} else {
			if (rel_pos > 0) {
				rel_pos--;
			}
		}
	}

	// Correct position after deleting an entry while the scrollbar is on the bottom
	if (file_list.length >= MAX_POSITION) {
		while ((base_pos + MAX_POSITION - 1) >= file_list.length) {
			if (base_pos > 0) {
				base_pos--;
				rel_pos++;
			}
		}
	}

	return ret;
}

void refreshMarkList() {
	FileListEntry *entry = mark_list.head;

	int length = mark_list.length;

	int i;
	for (i = 0; i < length; i++) {
		// Get next entry already now to prevent crash after entry is removed
		FileListEntry *next = entry->next;

		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, entry->name);

		// Check if the entry still exits. If not, remove it from list
		SceIoStat stat;
		if (sceIoGetstat(path, &stat) < 0)
			fileListRemoveEntry(&mark_list, entry);

		// Next
		entry = next;
	}
}

void refreshCopyList() {
	FileListEntry *entry = copy_list.head;

	int length = copy_list.length;

	int i;
	for (i = 0; i < length; i++) {
		// Get next entry already now to prevent crash after entry is removed
		FileListEntry *next = entry->next;

		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s%s", copy_list.path, entry->name);

		// Check if the entry still exits. If not, remove it from list
		SceIoStat stat;
		int res = sceIoGetstat(path, &stat);
		if (res < 0 && res != 0x80010014)
			fileListRemoveEntry(&copy_list, entry);

		// Next
		entry = next;
	}
}

int handleFile(char *file, FileListEntry *entry) {
	int res = 0;

	int type = getFileType(file);
	switch (type) {
		case FILE_TYPE_VPK:
		case FILE_TYPE_ZIP:
			if (isInArchive())
				type = FILE_TYPE_UNKNOWN;

			break;
	}

	switch (type) {
		case FILE_TYPE_INI:
		case FILE_TYPE_TXT:
		case FILE_TYPE_XML:
		case FILE_TYPE_UNKNOWN:
			res = textViewer(file);
			break;
			
		case FILE_TYPE_BMP:
		case FILE_TYPE_PNG:
		case FILE_TYPE_JPEG:
			res = photoViewer(file, type, &file_list, entry, &base_pos, &rel_pos);
			break;

		case FILE_TYPE_MP3:
			res = audioPlayer(file, &file_list, entry, &base_pos, &rel_pos, SCE_AUDIODEC_TYPE_MP3, 1);
			break;

		case FILE_TYPE_VPK:
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			break;
			
		case FILE_TYPE_ZIP:
			res = archiveOpen(file);
			break;

		case FILE_TYPE_SFO:
			res = SFOReader(file);
			break;
			
		default:
			errorDialog(type);
			break;
	}

	if (res < 0) {
		errorDialog(res);
		return res;
	}

	return type;
}

void drawScrollBar(int pos, int n) {
	if (n > MAX_POSITION) {
		vita2d_draw_rectangle(SCROLL_BAR_X, START_Y, SCROLL_BAR_WIDTH, MAX_ENTRIES * FONT_Y_SPACE, SCROLL_BAR_BG_COLOR);

		float y = START_Y + ((pos * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);
		float height = ((MAX_POSITION * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);

		vita2d_draw_rectangle(SCROLL_BAR_X, MIN(y, (START_Y + MAX_ENTRIES * FONT_Y_SPACE - height)), SCROLL_BAR_WIDTH, MAX(height, SCROLL_BAR_MIN_HEIGHT), SCROLL_BAR_COLOR);
	}
}

void drawShellInfo(char *path) {
	// Title
	char version[8];
	sprintf(version, "%X.%X", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR);
	if (version[3] == '0')
		version[3] = '\0';

	pgf_draw_textf(SHELL_MARGIN_X, SHELL_MARGIN_Y, TITLE_COLOR, FONT_SIZE, "VitaShell %s", version);

	// Battery
	float battery_x = ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_texture_get_width(battery_image));
	vita2d_draw_texture(battery_image, battery_x, SHELL_MARGIN_Y + 3.0f);

	vita2d_texture *battery_bar_image = battery_bar_green_image;

	if (scePowerIsBatteryCharging()) {
		battery_bar_image = battery_bar_charge_image;
	} else if (scePowerIsLowBattery()) {
		battery_bar_image = battery_bar_red_image;
	} 

	float percent = scePowerGetBatteryLifePercent() / 100.0f;

	float width = vita2d_texture_get_width(battery_bar_image);
	vita2d_draw_texture_part(battery_bar_image, battery_x + 3.0f + (1.0f - percent) * width, SHELL_MARGIN_Y + 5.0f, (1.0f - percent) * width, 0.0f, percent * width, vita2d_texture_get_height(battery_bar_image));

	// Date & time
	SceDateTime time;
	sceRtcGetCurrentClock(&time, 0);

	char date_string[16];
	getDateString(date_string, date_format, &time);

	char time_string[24];
	getTimeString(time_string, time_format, &time);

	char string[64];
	sprintf(string, "%s  %s", date_string, time_string);
	float date_time_x = ALIGN_LEFT(battery_x - 12.0f, vita2d_pgf_text_width(font, FONT_SIZE, string));
	pgf_draw_text(date_time_x, SHELL_MARGIN_Y, DATE_TIME_COLOR, FONT_SIZE, string);

	// FTP
	if (ftpvita_is_initialized())
		vita2d_draw_texture(ftp_image, date_time_x - 30.0f, SHELL_MARGIN_Y + 3.0f);

	// TODO: make this more elegant
	// Path
	int line_width = 0;

	int i;
	for (i = 0; i < strlen(path); i++) {
		char ch_width = font_size_cache[(int)path[i]];

		// Too long
		if ((line_width + ch_width) >= MAX_WIDTH)
			break;

		// Increase line width
		line_width += ch_width;
	}

	char path_first_line[256], path_second_line[256];

	strncpy(path_first_line, path, i);
	path_first_line[i] = '\0';

	strcpy(path_second_line, path + i);

	pgf_draw_text(SHELL_MARGIN_X, PATH_Y, PATH_COLOR, FONT_SIZE, path_first_line);
	pgf_draw_text(SHELL_MARGIN_X, PATH_Y + FONT_Y_SPACE, PATH_COLOR, FONT_SIZE, path_second_line);

	char str[128];

	// Show numbers of files and folders
	// sprintf(str, "%d files and %d folders", file_list.files, file_list.folders);

	// Show memory card
/*
	uint64_t free_size = 0, max_size = 0;
	sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

	char free_size_string[16], max_size_string[16];
	getSizeString(free_size_string, free_size);
	getSizeString(max_size_string, max_size);

	sprintf(str, "%s/%s", free_size_string, max_size_string);
*/

	// Draw on bottom left
	// pgf_draw_textf(ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, str)), SCREEN_HEIGHT - SHELL_MARGIN_Y - FONT_Y_SPACE - 2.0f, LITEGRAY, FONT_SIZE, str);
}

enum MenuEntrys {
	MENU_ENTRY_MARK_UNMARK_ALL,
	MENU_ENTRY_EMPTY_1,
	MENU_ENTRY_MOVE,
	MENU_ENTRY_COPY,
	MENU_ENTRY_PASTE,
	MENU_ENTRY_EMPTY_3,
	MENU_ENTRY_DELETE,
	MENU_ENTRY_RENAME,
	MENU_ENTRY_EMPTY_4,
	MENU_ENTRY_NEW_FOLDER,
	MENU_ENTRY_EMPTY_5,
	MENU_ENTRY_MORE,
};

MenuEntry menu_entries[] = {
	{ MARK_ALL, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ MOVE, CTX_VISIBILITY_INVISIBLE },
	{ COPY, CTX_VISIBILITY_INVISIBLE },
	{ PASTE, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ DELETE, CTX_VISIBILITY_INVISIBLE },
	{ RENAME, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ NEW_FOLDER, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ MORE, CTX_VISIBILITY_INVISIBLE }
};

#define N_MENU_ENTRIES (sizeof(menu_entries) / sizeof(MenuEntry))

enum MenuMoreEntrys {
	MENU_MORE_ENTRY_INSTALL_ALL,
	MENU_MORE_ENTRY_CALCULATE_SHA1,
};

MenuEntry menu_more_entries[] = {
	{ INSTALL_ALL, CTX_VISIBILITY_INVISIBLE },
	{ CALCULATE_SHA1, CTX_VISIBILITY_INVISIBLE },
};

#define N_MENU_MORE_ENTRIES (sizeof(menu_more_entries) / sizeof(MenuEntry))

void setContextMenuVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
			menu_entries[i].visibility = CTX_VISIBILITY_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_MOVE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_COPY].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Invisible 'Paste' if nothing is copied yet
	if (copy_list.length == 0)
		menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;

	// Invisible 'Paste' if the files to move are not from the same partition
	if (copy_mode == COPY_MODE_MOVE) {
		char *p = strchr(file_list.path, ':');
		char *q = strchr(copy_list.path, ':');
		if (p && q) {
			*p = '\0';
			*q = '\0';

			if (strcasecmp(file_list.path, copy_list.path) != 0) {
				menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
			}

			*q = ':';
			*p = ':';
		} else {
			menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
		}
	}

	// Invisble write operations in archives
	if (isInArchive()) { // TODO: read-only mount points
		menu_entries[MENU_ENTRY_MOVE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_NEW_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Mark/Unmark all text
	if (mark_list.length == (file_list.length - 1)) { // All marked
		menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
	} else { // Not all marked yet
		// On marked entry
		if (fileListFindEntry(&mark_list, file_entry->name)) {
			menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
		} else {
			menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = MARK_ALL;
		}
	}

	// Go to first entry
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
			setContextMenuPos(i);
			break;
		}
	}

	if (i == N_MENU_ENTRIES)
		setContextMenuPos(-1);
}

void setContextMenuMoreVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
			menu_more_entries[i].visibility = CTX_VISIBILITY_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Invisble operations in archives
	if (isInArchive()) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	if(file_entry->is_folder) {
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	if(file_entry->type != FILE_TYPE_VPK) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Go to first entry
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
			setContextMenuMorePos(i);
			break;
		}
	}

	if (i == N_MENU_MORE_ENTRIES)
		setContextMenuMorePos(-1);
}

int contextMenuEnterCallback(int pos, void* context) {
	switch (pos) {
		case MENU_ENTRY_MARK_UNMARK_ALL:
		{
			int on_marked_entry = 0;
			int length = mark_list.length;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
			if (fileListFindEntry(&mark_list, file_entry->name))
				on_marked_entry = 1;

			// Empty mark list
			fileListEmpty(&mark_list);

			// Mark all if not all entries are marked yet and we are not focusing on a marked entry
			if (length != (file_list.length - 1) && !on_marked_entry) {
				FileListEntry *file_entry = file_list.head->next; // Ignore '..'

				int i;
				for (i = 0; i < file_list.length - 1; i++) {
					FileListEntry *mark_entry = malloc(sizeof(FileListEntry));
					memcpy(mark_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&mark_list, mark_entry, SORT_NONE);

					// Next
					file_entry = file_entry->next;
				}
			}

			break;
		}
		
		case MENU_ENTRY_MOVE:
		case MENU_ENTRY_COPY:
		{
			// Mode
			if (pos == MENU_ENTRY_MOVE) {
				copy_mode = COPY_MODE_MOVE;
			} else {
				copy_mode = isInArchive() ? COPY_MODE_EXTRACT : COPY_MODE_NORMAL;
			}

			// Empty copy list at first
			if (copy_list.length > 0)
				fileListEmpty(&copy_list);

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

			// Paths
			if (fileListFindEntry(&mark_list, file_entry->name)) { // On marked entry
				// Copy mark list to copy list
				FileListEntry *mark_entry = mark_list.head;

				int i;
				for (i = 0; i < mark_list.length; i++) {
					FileListEntry *copy_entry = malloc(sizeof(FileListEntry));
					memcpy(copy_entry, mark_entry, sizeof(FileListEntry));
					fileListAddEntry(&copy_list, copy_entry, SORT_NONE);

					// Next
					mark_entry = mark_entry->next;
				}
			} else {
				FileListEntry *copy_entry = malloc(sizeof(FileListEntry));
				memcpy(copy_entry, file_entry, sizeof(FileListEntry));
				fileListAddEntry(&copy_list, copy_entry, SORT_NONE);
			}

			strcpy(copy_list.path, file_list.path);

			char *message;

			// On marked entry
			if (fileListFindEntry(&copy_list, file_entry->name)) {
				if (copy_list.length == 1) {
					message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
				} else {
					message = language_container[COPIED_FILES_FOLDERS];
				}
			} else {
				message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
			}

			// Copy message
			infoDialog(message, copy_list.length);

			break;
		}

		case MENU_ENTRY_PASTE:
		{
			int copy_text = 0;

			switch (copy_mode) {
				case COPY_MODE_NORMAL:
					copy_text = COPYING;
					break;
					
				case COPY_MODE_MOVE:
					copy_text = MOVING;
					break;
					
				case COPY_MODE_EXTRACT:
					copy_text = EXTRACTING;
					break;
			}

			initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[copy_text]);
			dialog_step = DIALOG_STEP_PASTE;
			break;
		}

		case MENU_ENTRY_DELETE:
		{
			char *message;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

			// On marked entry
			if (fileListFindEntry(&mark_list, file_entry->name)) {
				if (mark_list.length == 1) {
					message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
				} else {
					message = language_container[DELETE_FILES_FOLDERS_QUESTION];
				}
			} else {
				message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
			}

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
			dialog_step = DIALOG_STEP_DELETE_QUESTION;
			break;
		}

		case MENU_ENTRY_RENAME:
		{
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

			char name[MAX_NAME_LENGTH];
			strcpy(name, file_entry->name);
			removeEndSlash(name);

			initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);

			dialog_step = DIALOG_STEP_RENAME;
			break;
		}
		
		case MENU_ENTRY_NEW_FOLDER:
		{
			// Find a new folder name
			char path[MAX_PATH_LENGTH];

			int count = 1;
			while (1) {
				if (count == 1) {
					snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, language_container[NEW_FOLDER]);
				} else {
					snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path, language_container[NEW_FOLDER], count);
				}

				SceIoStat stat;
				if (sceIoGetstat(path, &stat) < 0)
					break;

				count++;
			}

			initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path), MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);
			dialog_step = DIALOG_STEP_NEW_FOLDER;
			break;
		}
		
		case MENU_ENTRY_MORE:
		{
			setContextMenuMoreVisibilities();
			return CONTEXT_MENU_MORE_OPENING;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

int contextMenuMoreEnterCallback(int pos, void* context) {
	switch (pos) {
		case MENU_MORE_ENTRY_INSTALL_ALL:
		{
			// Empty install list
			fileListEmpty(&install_list);

			FileListEntry *file_entry = file_list.head->next; // Ignore '..'

			int i;
			for (i = 0; i < file_list.length - 1; i++) {
				char path[MAX_PATH_LENGTH];
				snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				int type = getFileType(path);
				if (type == FILE_TYPE_VPK) {
					FileListEntry *install_entry = malloc(sizeof(FileListEntry));
					memcpy(install_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&install_list, install_entry, SORT_NONE);
				}

				// Next
				file_entry = file_entry->next;
			}

			strcpy(install_list.path, file_list.path);

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_ALL_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			
			break;
		}
		
		case MENU_MORE_ENTRY_CALCULATE_SHA1:
		{
			// Ensure user wants to actually take the hash
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[HASH_FILE_QUESTION]);
			dialog_step = DIALOG_STEP_HASH_QUESTION;
			break;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

int dialogSteps() {
	int refresh = 0;

	int msg_result = updateMessageDialog();
	int ime_result = updateImeDialog();

	switch (dialog_step) {
		// Without refresh
		case DIALOG_STEP_ERROR:
		case DIALOG_STEP_INFO:
		case DIALOG_STEP_SYSTEM:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		// With refresh
		case DIALOG_STEP_COPIED:
		case DIALOG_STEP_DELETED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_CANCELLED:
			refresh = 1;
			dialog_step = DIALOG_STEP_NONE;
			break;
			
		case DIALOG_STEP_MOVED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				fileListEmpty(&copy_list);
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_FTP:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				powerUnlock();
				ftpvita_fini();
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_PASTE:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				CopyArguments args;
				args.file_list = &file_list;
				args.copy_list = &copy_list;
				args.archive_path = archive_path;
				args.copy_mode = copy_mode;

				dialog_step = DIALOG_STEP_COPYING;

				SceUID thid = sceKernelCreateThread("copy_thread", (SceKernelThreadEntry)copy_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(CopyArguments), &args);
			}

			break;
			
		case DIALOG_STEP_DELETE_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DELETING]);
				dialog_step = DIALOG_STEP_DELETE_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_DELETE_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				DeleteArguments args;
				args.file_list = &file_list;
				args.mark_list = &mark_list;
				args.index = base_pos + rel_pos;

				dialog_step = DIALOG_STEP_DELETING;

				SceUID thid = sceKernelCreateThread("delete_thread", (SceKernelThreadEntry)delete_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(DeleteArguments), &args);
			}

			break;
			
		case DIALOG_STEP_RENAME:
			if (ime_result == IME_DIALOG_RESULT_FINISHED) {
				char *name = (char *)getImeDialogInputTextUTF8();
				if (name[0] == '\0') {
					dialog_step = DIALOG_STEP_NONE;
				} else {
					FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

					char old_name[MAX_NAME_LENGTH];
					strcpy(old_name, file_entry->name);
					removeEndSlash(old_name);

					if (strcasecmp(old_name, name) == 0) { // No change
						dialog_step = DIALOG_STEP_NONE;
					} else {
						char old_path[MAX_PATH_LENGTH];
						char new_path[MAX_PATH_LENGTH];

						snprintf(old_path, MAX_PATH_LENGTH, "%s%s", file_list.path, old_name);
						snprintf(new_path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

						int res = sceIoRename(old_path, new_path);
						if (res < 0) {
							errorDialog(res);
						} else {
							refresh = 1;
							dialog_step = DIALOG_STEP_NONE;
						}
					}
				}
			} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_NEW_FOLDER:
			if (ime_result == IME_DIALOG_RESULT_FINISHED) {
				char *name = (char *)getImeDialogInputTextUTF8();
				if (name[0] == '\0') {
					dialog_step = DIALOG_STEP_NONE;
				} else {
					char path[MAX_PATH_LENGTH];
					snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

					int res = sceIoMkdir(path, 0777);
					if (res < 0) {
						errorDialog(res);
					} else {
						refresh = 1;
						dialog_step = DIALOG_STEP_NONE;
					}
				}
			} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;

		case DIALOG_STEP_HASH_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				// Throw up the progress bar, enter hashing state
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[HASHING]);
				dialog_step = DIALOG_STEP_HASH_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				// Quit
				dialog_step = DIALOG_STEP_NONE;
			}

			break;

		case DIALOG_STEP_HASH_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				// User has confirmed desire to hash, get requested file entry
				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

				// Place the full file path in cur_file
				snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				HashArguments args;
				args.file_path = cur_file;

				dialog_step = DIALOG_STEP_HASHING;

				// Create a thread to run out actual sum
				SceUID thid = sceKernelCreateThread("hash_thread", (SceKernelThreadEntry)hash_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(HashArguments), &args);
			}

			break;

		case DIALOG_STEP_HASH_DISPLAY:
			// Reset dialog state when user selects yes/no
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;

		case DIALOG_STEP_INSTALL_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
				dialog_step = DIALOG_STEP_INSTALL_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_INSTALL_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				InstallArguments args;

				if (install_list.length > 0) {
					FileListEntry *entry = install_list.head;
					snprintf(install_path, MAX_PATH_LENGTH, "%s%s", install_list.path, entry->name);
					args.file = install_path;

					// Focus
					focusOnFilename(entry->name);

					// Remove entry
					fileListRemoveEntry(&install_list, entry);
				} else {
					args.file = cur_file;
				}

				dialog_step = DIALOG_STEP_INSTALLING;

				SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(InstallArguments), &args);
			}

			break;
			
		case DIALOG_STEP_INSTALL_WARNING:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				dialog_step = DIALOG_STEP_INSTALL_WARNING_AGREED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_CANCELLED;
			}

			break;
			
		case DIALOG_STEP_INSTALLED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				if (install_list.length > 0) {
					initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
					dialog_step = DIALOG_STEP_INSTALL_CONFIRMED;
					break;
				}

				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_UPDATE_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DOWNLOADING]);
				dialog_step = DIALOG_STEP_DOWNLOADING;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}
			
		case DIALOG_STEP_DOWNLOADED:
			if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);

				dialog_step = DIALOG_STEP_EXTRACTING;

				SceUID thid = sceKernelCreateThread("update_extract_thread", (SceKernelThreadEntry)update_extract_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, 0, NULL);
			}

			break;
			
		case DIALOG_STEP_EXTRACTED:
			launchAppByUriExit("VSUPDATER");
			dialog_step = DIALOG_STEP_NONE;
			break;
	}

	return refresh;
}

void fileBrowserMenuCtrl() {
	// System information
	if (current_buttons & SCE_CTRL_START) {
		// System software version
		SceSystemSwVersionParam sw_ver_param;
		sw_ver_param.size = sizeof(SceSystemSwVersionParam);
		sceKernelGetSystemSwVersion(&sw_ver_param);

		// MAC address
		SceNetEtherAddr mac;
		sceNetGetMacAddress(&mac, 0);

		char mac_string[32];
		sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X", mac.data[0], mac.data[1], mac.data[2], mac.data[3], mac.data[4], mac.data[5]);

		// Get IP
		char ip[16];

		SceNetCtlInfo info;
		if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) < 0) {
			strcpy(ip, "-");
		} else {
			strcpy(ip, info.ip_address);
		}

		// Memory card
		uint64_t free_size = 0, max_size = 0;
		sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

		char free_size_string[16], max_size_string[16];
		getSizeString(free_size_string, free_size);
		getSizeString(max_size_string, max_size);

		// System information dialog
		initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[SYS_INFO], sw_ver_param.version_string, sceKernelGetModelForCDialog(), mac_string, ip, free_size_string, max_size_string);
		dialog_step = DIALOG_STEP_SYSTEM;
	}

	// FTP
	if (pressed_buttons & SCE_CTRL_SELECT) {
		// Init FTP
		if (!ftpvita_is_initialized()) {
			int res = ftpvita_init(vita_ip, &vita_port);
			if (res < 0) {
				infoDialog(language_container[WIFI_ERROR]);
			} else {
				// Add all the current mountpoints to ftpvita
				int i;
				for (i = 0; i < getNumberMountPoints(); i++) {
					char **mount_points = getMountPoints();
					if (mount_points[i]) {
						ftpvita_add_device(mount_points[i]);
					}
				}
				ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);
			}

			// Lock power timers
			powerLock();
		}

		// Dialog
		if (ftpvita_is_initialized()) {
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
			dialog_step = DIALOG_STEP_FTP;
		}
	}
	
	// Move
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		if (rel_pos > 0) {
			rel_pos--;
		} else {
			if (base_pos > 0) {
				base_pos--;
			}
		}
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		if ((rel_pos + 1) < file_list.length) {
			if ((rel_pos + 1) < MAX_POSITION) {
				rel_pos++;
			} else {
				if ((base_pos + rel_pos + 1) < file_list.length) {
					base_pos++;
				}
			}
		}
	}

	// Not at 'home'
	if (dir_level > 0) {
		// Context menu trigger
		if (pressed_buttons & SCE_CTRL_TRIANGLE) {
			if (getContextMenuMode() == CONTEXT_MENU_CLOSED) {
				setContextMenuVisibilities();
				setContextMenuMode(CONTEXT_MENU_OPENING);
			}
		}

		// Mark entry
		if (pressed_buttons & SCE_CTRL_SQUARE) {
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
			if (strcmp(file_entry->name, DIR_UP) != 0) {
				if (!fileListFindEntry(&mark_list, file_entry->name)) {
					FileListEntry *mark_entry = malloc(sizeof(FileListEntry));
					memcpy(mark_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&mark_list, mark_entry, SORT_NONE);
				} else {
					fileListRemoveEntryByName(&mark_list, file_entry->name);
				}
			}
		}

		// Back
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			fileListEmpty(&mark_list);
			dirUp();
			WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);
			refreshFileList();
		}
	}

	// Handle
	if (pressed_buttons & SCE_CTRL_ENTER) {
		fileListEmpty(&mark_list);

		// Handle file or folder
		FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
		if (file_entry->is_folder) {
			if (strcmp(file_entry->name, DIR_UP) == 0) {
				dirUp();
			} else {
				if (dir_level == 0) {
					strcpy(file_list.path, file_entry->name);
				} else {
					if (dir_level > 1)
						addEndSlash(file_list.path);
					strcat(file_list.path, file_entry->name);
				}

				dirLevelUp();
			}

			// Save last dir
			WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);

			// Open folder
			int res = refreshFileList();
			if (res < 0)
				errorDialog(res);
		} else {
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			int type = handleFile(cur_file, file_entry);

			// Archive mode
			if (type == FILE_TYPE_ZIP) {
				is_in_archive = 1;
				dir_level_archive = dir_level;

				snprintf(archive_path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				strcat(file_list.path, file_entry->name);
				addEndSlash(file_list.path);

				dirLevelUp();
				refreshFileList();
			}
		}
	}
	
	void *buf = malloc(0x100);
	// Mount RW Partitions - Originaly writted for tomtomdu80
	if (current_buttons & SCE_CTRL_LTRIGGER && current_buttons & SCE_CTRL_RTRIGGER && current_buttons & SCE_CTRL_SQUARE) { // L + R + Square
		// Init Mount Unsafe Mode RW Partitions	
		{
			int i;
			sceKernelDelayThread(0 * 1000 * 1000);
			vshIoUmount(0x300, 0, 0, 0); // Dismount only vs0 partition
			
			sceKernelDelayThread(0 * 1000 * 1000);
			_vshIoMount(0x300, 0, 2, buf); // Mount only vs0 partition in RW
		}
	}
	if (current_buttons & SCE_CTRL_LTRIGGER && current_buttons & SCE_CTRL_RTRIGGER && current_buttons & SCE_CTRL_TRIANGLE) { // L + R + Triangle
		// Init Mount Brick Mode RW Partitions
		{
			int i;
			sceKernelDelayThread(0 * 1000 * 1000); // Dismount all system partitions
			vshIoUmount(0x00, 0, 0, 0);
			vshIoUmount(0x100, 0, 0, 0);
			vshIoUmount(0x200, 0, 0, 0);
			vshIoUmount(0x300, 0, 0, 0);
			vshIoUmount(0x400, 0, 0, 0);
			vshIoUmount(0x500, 0, 0, 0);
			vshIoUmount(0x600, 0, 0, 0);
			vshIoUmount(0x700, 0, 0, 0); 
			
			sceKernelDelayThread(0 * 1000 * 1000); // Mount all system partitions in RW
			_vshIoMount(0x00, 0, 2, buf);
			_vshIoMount(0x100, 0, 2, buf);
			_vshIoMount(0x200, 0, 2, buf);
			_vshIoMount(0x300, 0, 2, buf);
			_vshIoMount(0x400, 0, 2, buf);
			_vshIoMount(0x500, 0, 2, buf);
			_vshIoMount(0x600, 0, 2, buf);
			_vshIoMount(0x700, 0, 2, buf);
		}
	}		
}

int shellMain() {
	// Position
	memset(base_pos_list, 0, sizeof(base_pos_list));
	memset(rel_pos_list, 0, sizeof(rel_pos_list));

	// Paths
	memset(cur_file, 0, sizeof(cur_file));
	memset(archive_path, 0, sizeof(archive_path));

	// File lists
	memset(&file_list, 0, sizeof(FileList));
	memset(&mark_list, 0, sizeof(FileList));
	memset(&copy_list, 0, sizeof(FileList));
	memset(&install_list, 0, sizeof(FileList));

	// Current path is 'home'
	strcpy(file_list.path, HOME_PATH);

	// Last dir
	char lastdir[MAX_PATH_LENGTH];
	ReadFile(VITASHELL_LASTDIR, lastdir, sizeof(lastdir));

	// Calculate dir positions if the dir is valid
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat(lastdir, &stat) >= 0) {
		int i;
		for (i = 0; i < strlen(lastdir) + 1; i++) {
			if (lastdir[i] == ':' || lastdir[i] == '/') {
				char ch = lastdir[i + 1];
				lastdir[i + 1] = '\0';

				char ch2 = lastdir[i];
				lastdir[i] = '\0';

				char *p = strrchr(lastdir, '/');
				if (!p)
					p = strrchr(lastdir, ':');
				if (!p)
					p = lastdir - 1;

				lastdir[i] = ch2;

				refreshFileList();
				focusOnFilename(p + 1);

				strcpy(file_list.path, lastdir);

				lastdir[i + 1] = ch;

				dirLevelUp();
			}
		}
	}

	// Refresh file list
	refreshFileList();

	// Init context menu param
	ContextMenu context_menu;
	context_menu.menu_entries = menu_entries;
	context_menu.n_menu_entries = N_MENU_ENTRIES;
	context_menu.menu_more_entries = menu_more_entries;
	context_menu.n_menu_more_entries = N_MENU_MORE_ENTRIES;
	context_menu.menu_max_width = ctx_menu_max_width;
	context_menu.menu_more_max_width = ctx_menu_more_max_width;
	context_menu.more_pos = MENU_ENTRY_MORE;
	context_menu.menuEnterCallback = contextMenuEnterCallback;
	context_menu.menuMoreEnterCallback = contextMenuMoreEnterCallback;

	while (1) {
		readPad();

		int refresh = 0;

		// Control
		if (dialog_step == DIALOG_STEP_NONE) {
			if (getContextMenuMode() != CONTEXT_MENU_CLOSED) {
				contextMenuCtrl(&context_menu);
			} else {
				fileBrowserMenuCtrl();
			}
		} else {
			refresh = dialogSteps();
		}

		// Receive system event
		SceAppMgrSystemEvent event;
		sceAppMgrReceiveSystemEvent(&event);

		// Refresh on app resume
		if (event.systemEvent == SCE_APPMGR_SYSTEMEVENT_ON_RESUME) {
			refresh = 1;
		}

		if (refresh) {
			// Refresh lists
			refreshFileList();
			refreshMarkList();
			refreshCopyList();
		}

		// Start drawing
		startDrawing(bg_browser_image);

		// Draw shell info
		drawShellInfo(file_list.path);

		// Draw scroll bar
		drawScrollBar(base_pos, file_list.length);

		// Draw
		FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos);

		int i;
		for (i = 0; i < MAX_ENTRIES && (base_pos + i) < file_list.length; i++) {
			uint32_t color = FILE_COLOR;
			float y = START_Y + (i * FONT_Y_SPACE);

			vita2d_texture *icon = NULL;

			// Folder
			if (file_entry->is_folder) {
				color = FOLDER_COLOR;
				icon = folder_icon;
			} else {
				switch (file_entry->type) {
					case FILE_TYPE_BMP:
					case FILE_TYPE_PNG:
					case FILE_TYPE_JPEG:
						color = IMAGE_COLOR;
						icon = image_icon;
						break;
						
					case FILE_TYPE_VPK:
					case FILE_TYPE_ZIP:
						color = ARCHIVE_COLOR;
						icon = archive_icon;
						break;
						
					case FILE_TYPE_MP3:
						color = IMAGE_COLOR;
						icon = audio_icon;
						break;
						
					case FILE_TYPE_SFO:
						color = SFO_COLOR;
						icon = sfo_icon;
						break;
					
					case FILE_TYPE_INI:
					case FILE_TYPE_TXT:
					case FILE_TYPE_XML:
						color = TXT_COLOR;
						icon = text_icon;
						break;
						
					default:
						color = FILE_COLOR;
						icon = file_icon;
						break;
				}
			}

			// Draw icon
			if (icon)
				vita2d_draw_texture(icon, SHELL_MARGIN_X, y + 3.0f);

			// Current position
			if (i == rel_pos)
				color = FOCUS_COLOR;

			// Marked
			if (fileListFindEntry(&mark_list, file_entry->name))
				vita2d_draw_rectangle(SHELL_MARGIN_X, y + 3.0f, MARK_WIDTH, FONT_Y_SPACE, MARKED_COLOR);

			// File name
			int length = strlen(file_entry->name);
			int line_width = 0;

			int j;
			for (j = 0; j < length; j++) {
				char ch_width = font_size_cache[(int)file_entry->name[j]];

				// Too long
				if ((line_width + ch_width) >= MAX_NAME_WIDTH)
					break;

				// Increase line width
				line_width += ch_width;
			}

			char ch = 0;

			if (j != length) {
				ch = file_entry->name[j];
				file_entry->name[j] = '\0';
			}

			// Draw shortened file name
			pgf_draw_text(SHELL_MARGIN_X + 26.0f, y, color, FONT_SIZE, file_entry->name);

			if (j != length)
				file_entry->name[j] = ch;

			// File information
			if (strcmp(file_entry->name, DIR_UP) != 0) {
				// Folder/size
				char size_string[16];
				getSizeString(size_string, file_entry->size);

				char *str = file_entry->is_folder ? language_container[FOLDER] : size_string;

				pgf_draw_text(ALIGN_LEFT(INFORMATION_X, vita2d_pgf_text_width(font, FONT_SIZE, str)), y, color, FONT_SIZE, str);

				// Date
				char date_string[16];
				getDateString(date_string, date_format, &file_entry->time);

				char time_string[24];
				getTimeString(time_string, time_format, &file_entry->time);

				char string[64];
				sprintf(string, "%s %s", date_string, time_string);

				pgf_draw_text(ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, string)), y, color, FONT_SIZE, string);
			}

			// Next
			file_entry = file_entry->next;
		}

		// Draw context menu
		drawContextMenu(&context_menu);

		// End drawing
		endDrawing();
	}

	// Empty lists
	fileListEmpty(&copy_list);
	fileListEmpty(&mark_list);
	fileListEmpty(&file_list);

	return 0;
}

void initContextMenuWidth() {
	int i;
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility != CTX_VISIBILITY_UNUSED)
			ctx_menu_max_width = MAX(ctx_menu_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_entries[i].name]));

		if (menu_entries[i].name == MARK_ALL) {
			menu_entries[i].name = UNMARK_ALL;
			i--;
		}
	}

	ctx_menu_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	ctx_menu_max_width = MAX(ctx_menu_max_width, CONTEXT_MENU_MIN_WIDTH);

	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility != CTX_VISIBILITY_UNUSED)
			ctx_menu_more_max_width = MAX(ctx_menu_more_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_more_entries[i].name]));
	}

	ctx_menu_more_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	ctx_menu_more_max_width = MAX(ctx_menu_more_max_width, CONTEXT_MENU_MORE_MIN_WIDTH);
}

void ftpvita_PROM(ftpvita_client_info_t *client) {
	char cmd[64];
	char path[MAX_PATH_LENGTH];
	sscanf(client->recv_buffer, "%s %s", cmd, path);

	if (installPackage(path) == 0) {
		ftpvita_ext_client_send_ctrl_msg(client, "200 OK PROMOTING\r\n");
	} else {
		ftpvita_ext_client_send_ctrl_msg(client, "500 ERROR PROMOTING\r\n");
	}
}

int main(int argc, const char *argv[]) {
	// Init VitaShell
	initVitaShell();

	// No custom config, in case they are damaged or unuseable
	readPad();
	if (current_buttons & SCE_CTRL_LTRIGGER)
		use_custom_config = 0;

	// Load theme
	loadTheme();

	// Load language
	loadLanguage(language);

	// Init context menu width
	initContextMenuWidth();
	initTextContextMenuWidth();

	// Automatic network update
	SceUID thid = sceKernelCreateThread("network_update_thread", (SceKernelThreadEntry)network_update_thread, 0x40, 0x10000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);

	// Main
	shellMain();

	// Finish VitaShell
	finishVitaShell();
	
	return 0;
}
