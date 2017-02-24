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

#include "main.h"
#include "init.h"
#include "io_process.h"
#include "context_menu.h"
#include "file.h"
#include "language.h"
#include "property_dialog.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "utils.h"
#include "usb.h"

enum MenuHomeEntrys {
	MENU_HOME_ENTRY_MOUNT_UMA0,
	MENU_HOME_ENTRY_MOUNT_USB_UX0,
	MENU_HOME_ENTRY_UMOUNT_USB_UX0,
};

MenuEntry menu_home_entries[] = {
	{ MOUNT_UMA0,     0, 0, CTX_INVISIBLE },
	{ MOUNT_USB_UX0,  2, 0, CTX_INVISIBLE },
	{ UMOUNT_USB_UX0, 3, 0, CTX_INVISIBLE },
};

#define N_MENU_HOME_ENTRIES (sizeof(menu_home_entries) / sizeof(MenuEntry))

enum MenuMainEntrys {
	MENU_MAIN_ENTRY_MARK_UNMARK_ALL,
	MENU_MAIN_ENTRY_MOVE,
	MENU_MAIN_ENTRY_COPY,
	MENU_MAIN_ENTRY_PASTE,
	MENU_MAIN_ENTRY_DELETE,
	MENU_MAIN_ENTRY_RENAME,
	MENU_MAIN_ENTRY_NEW_FOLDER,
	MENU_MAIN_ENTRY_PROPERTIES,
	MENU_MAIN_ENTRY_MORE,
	MENU_MAIN_ENTRY_SORT_BY,
};

MenuEntry menu_main_entries[] = {
	{ MARK_ALL,    0, 0, CTX_INVISIBLE },
	{ MOVE,        2, 0, CTX_INVISIBLE },
	{ COPY,        3, 0, CTX_INVISIBLE },
	{ PASTE,       4, 0, CTX_INVISIBLE },
	{ DELETE,      6, 0, CTX_INVISIBLE },
	{ RENAME,      7, 0, CTX_INVISIBLE },
	{ NEW_FOLDER,  9, 0, CTX_INVISIBLE },
	{ PROPERTIES, 10, 0, CTX_INVISIBLE },
	{ MORE,       12, 1, CTX_INVISIBLE },
	{ SORT_BY,    13, 1, CTX_VISIBLE },
};

#define N_MENU_MAIN_ENTRIES (sizeof(menu_main_entries) / sizeof(MenuEntry))

enum MenuSortEntrys {
	MENU_SORT_ENTRY_BY_NAME,
	MENU_SORT_ENTRY_BY_SIZE,
	MENU_SORT_ENTRY_BY_DATE,
};

MenuEntry menu_sort_entries[] = {
	{ BY_NAME, 13, 0, CTX_INVISIBLE },
	{ BY_SIZE, 14, 0, CTX_INVISIBLE },
	{ BY_DATE, 15, 0, CTX_INVISIBLE },
};

#define N_MENU_SORT_ENTRIES (sizeof(menu_sort_entries) / sizeof(MenuEntry))

enum MenuMoreEntrys {
	MENU_MORE_ENTRY_COMPRESS,
	MENU_MORE_ENTRY_INSTALL_ALL,
	MENU_MORE_ENTRY_INSTALL_FOLDER,
	MENU_MORE_ENTRY_EXPORT_MEDIA,
	MENU_MORE_ENTRY_CALCULATE_SHA1,
};

MenuEntry menu_more_entries[] = {
	{ COMPRESS,       12, 0, CTX_INVISIBLE },
	{ INSTALL_ALL,    13, 0, CTX_INVISIBLE },
	{ INSTALL_FOLDER, 14, 0, CTX_INVISIBLE },
	{ EXPORT_MEDIA,   15, 0, CTX_INVISIBLE },
	{ CALCULATE_SHA1, 16, 0, CTX_INVISIBLE },
};

#define N_MENU_MORE_ENTRIES (sizeof(menu_more_entries) / sizeof(MenuEntry))

static int contextMenuHomeEnterCallback(int sel, void *context);
static int contextMenuMainEnterCallback(int sel, void *context);
static int contextMenuSortEnterCallback(int sel, void *context);
static int contextMenuMoreEnterCallback(int sel, void *context);

ContextMenu context_menu_home = {
	.parent = NULL,
	.entries = menu_home_entries,
	.n_entries = N_MENU_HOME_ENTRIES,
	.max_width = 0.0f,
	.callback = contextMenuHomeEnterCallback,
	.sel = -1,
};

ContextMenu context_menu_main = {
	.parent = NULL,
	.entries = menu_main_entries,
	.n_entries = N_MENU_MAIN_ENTRIES,
	.max_width = 0.0f,
	.callback = contextMenuMainEnterCallback,
	.sel = -1,
};

ContextMenu context_menu_sort = {
	.parent = &context_menu_main,
	.entries = menu_sort_entries,
	.n_entries = N_MENU_SORT_ENTRIES,
	.max_width = 0.0f,
	.callback = contextMenuSortEnterCallback,
	.sel = -1,
};

ContextMenu context_menu_more = {
	.parent = &context_menu_main,
	.entries = menu_more_entries,
	.n_entries = N_MENU_MORE_ENTRIES,
	.max_width = 0.0f,
	.callback = contextMenuMoreEnterCallback,
	.sel = -1,
};

void initContextMenuWidth() {
	int i;

	// Home
	for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
		context_menu_home.max_width = MAX(context_menu_home.max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_home_entries[i].name]));
	}

	context_menu_home.max_width += 2.0f*CONTEXT_MENU_MARGIN;
	context_menu_home.max_width = MAX(context_menu_home.max_width, CONTEXT_MENU_MIN_WIDTH);

	// Main
	for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
		context_menu_main.max_width = MAX(context_menu_main.max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_main_entries[i].name]));

		if (menu_main_entries[i].name == MARK_ALL) {
			menu_main_entries[i].name = UNMARK_ALL;
			i--;
		}
	}

	context_menu_main.max_width += 2.0f*CONTEXT_MENU_MARGIN;
	context_menu_main.max_width = MAX(context_menu_main.max_width, CONTEXT_MENU_MIN_WIDTH);

	// Sort
	for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
		context_menu_sort.max_width = MAX(context_menu_sort.max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_sort_entries[i].name]));
	}

	context_menu_sort.max_width += 2.0f*CONTEXT_MENU_MARGIN;
	context_menu_sort.max_width = MAX(context_menu_sort.max_width, CONTEXT_MENU_MIN_WIDTH);

	// More
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		context_menu_more.max_width = MAX(context_menu_more.max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_more_entries[i].name]));
	}

	context_menu_more.max_width += 2.0f*CONTEXT_MENU_MARGIN;
	context_menu_more.max_width = MAX(context_menu_more.max_width, CONTEXT_MENU_MIN_WIDTH);
}

void setContextMenuHomeVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
		if (menu_home_entries[i].visibility == CTX_INVISIBLE)
			menu_home_entries[i].visibility = CTX_VISIBLE;
	}

	// Invisible if already mounted or if we're not on a Vita TV
	int uma_exist = 0;

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat("uma0:", &stat) >= 0)
		uma_exist = 1;
		
	if (uma_exist || isUx0Redirected() || sceKernelGetModel() == SCE_KERNEL_MODEL_VITA)
		menu_home_entries[MENU_HOME_ENTRY_MOUNT_UMA0].visibility = CTX_INVISIBLE;

	if (isUx0Redirected()) {
		menu_home_entries[MENU_HOME_ENTRY_MOUNT_USB_UX0].visibility = CTX_INVISIBLE;
	} else if (uma_exist) {
		menu_home_entries[MENU_HOME_ENTRY_UMOUNT_USB_UX0].visibility = CTX_INVISIBLE;
	} else {
		menu_home_entries[MENU_HOME_ENTRY_MOUNT_USB_UX0].visibility = CTX_INVISIBLE;
		menu_home_entries[MENU_HOME_ENTRY_UMOUNT_USB_UX0].visibility = CTX_INVISIBLE;
	}

	// Go to first entry
	for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
		if (menu_home_entries[i].visibility == CTX_VISIBLE) {
			context_menu_home.sel = i;
			break;
		}
	}

	if (i == N_MENU_HOME_ENTRIES)
		context_menu_home.sel = -1;
}

void setContextMenuMainVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
		if (menu_main_entries[i].visibility == CTX_INVISIBLE)
			menu_main_entries[i].visibility = CTX_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_COPY].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_PROPERTIES].visibility = CTX_INVISIBLE;
	}

	// Invisible 'Paste' if nothing is copied yet
	if (copy_list.length == 0)
		menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;

	// Invisible 'Paste' if the files to move are not from the same partition
	if (copy_mode == COPY_MODE_MOVE) {
		char *p = strchr(file_list.path, ':');
		char *q = strchr(copy_list.path, ':');
		if (p && q) {
			*p = '\0';
			*q = '\0';

			if (strcasecmp(file_list.path, copy_list.path) != 0) {
				menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
			}

			*q = ':';
			*p = ':';
		} else {
			menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
		}
	}

	// Invisble write operations in archives
	if (isInArchive()) { // TODO: read-only mount points
		menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
		menu_main_entries[MENU_MAIN_ENTRY_NEW_FOLDER].visibility = CTX_INVISIBLE;
	}

	// Mark/Unmark all text
	if (mark_list.length == (file_list.length-1)) { // All marked
		menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
	} else { // Not all marked yet
		// On marked entry
		if (fileListFindEntry(&mark_list, file_entry->name)) {
			menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
		} else {
			menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = MARK_ALL;
		}
	}

	// Go to first entry
	for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
		if (menu_main_entries[i].visibility == CTX_VISIBLE) {
			context_menu_main.sel = i;
			break;
		}
	}

	if (i == N_MENU_MAIN_ENTRIES)
		context_menu_main.sel = -1;
}

void setContextMenuSortVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
		if (menu_sort_entries[i].visibility == CTX_INVISIBLE)
			menu_sort_entries[i].visibility = CTX_VISIBLE;
	}

	// Invisible when it's the current mode
	if (sort_mode == SORT_BY_NAME)
		menu_sort_entries[MENU_SORT_ENTRY_BY_NAME].visibility = CTX_INVISIBLE;
	else if (sort_mode == SORT_BY_SIZE)
		menu_sort_entries[MENU_SORT_ENTRY_BY_SIZE].visibility = CTX_INVISIBLE;
	else if (sort_mode == SORT_BY_DATE)
		menu_sort_entries[MENU_SORT_ENTRY_BY_DATE].visibility = CTX_INVISIBLE;
	
	// Go to first entry
	for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
		if (menu_sort_entries[i].visibility == CTX_VISIBLE) {
			context_menu_sort.sel = i;
			break;
		}
	}

	if (i == N_MENU_SORT_ENTRIES)
		context_menu_sort.sel = -1;
}

void setContextMenuMoreVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_INVISIBLE)
			menu_more_entries[i].visibility = CTX_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
	}

	// Invisble operations in archives
	if (isInArchive()) {
		menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
	}

	if (file_entry->is_folder) {
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
		do {
			char check_path[MAX_PATH_LENGTH];
			SceIoStat stat;
			int statres;
			if (strcmp(file_list.path, "ux0:app/") == 0 || strcmp(file_list.path, "ux0:patch/") == 0) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
				break;
			}
			snprintf(check_path, MAX_PATH_LENGTH, "%s%s/eboot.bin", file_list.path, file_entry->name);
			statres = sceIoGetstat(check_path, &stat);
			if (statres < 0 || SCE_S_ISDIR(stat.st_mode)) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
				break;
			}
			snprintf(check_path, MAX_PATH_LENGTH, "%s%s/sce_sys/param.sfo", file_list.path, file_entry->name);
			statres = sceIoGetstat(check_path, &stat);
			if (statres < 0 || SCE_S_ISDIR(stat.st_mode)) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
				break;
			}
		} while(0);
	} else {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
	}

	if(file_entry->type != FILE_TYPE_VPK) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
	}

	// Invisible export for non-media files
	if (!file_entry->is_folder && file_entry->type != FILE_TYPE_BMP && file_entry->type != FILE_TYPE_JPEG && file_entry->type != FILE_TYPE_PNG && file_entry->type != FILE_TYPE_MP3) {
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
	}

	// Go to first entry
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_VISIBLE) {
			context_menu_more.sel = i;
			break;
		}
	}

	if (i == N_MENU_MORE_ENTRIES)
		context_menu_more.sel = -1;
}

static int contextMenuHomeEnterCallback(int sel, void *context) {
	switch (sel) {
		case MENU_HOME_ENTRY_MOUNT_UMA0:
		{
			SceUID fd = sceIoOpen("sdstor0:uma-lp-act-entire", SCE_O_RDONLY, 0);
			if (fd >= 0) {
				sceIoClose(fd);
				int res = vshIoMount(0xF00, NULL, 0, 0, 0, 0);
				if (res < 0)
					errorDialog(res);
				else
					infoDialog(language_container[USB_UMA0_MOUNTED]);
				refreshFileList();
			} else {
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[USB_WAIT_ATTACH]);
				setDialogStep(DIALOG_STEP_USB_ATTACH_WAIT);
			}
			
			break;
		}
		
		case MENU_HOME_ENTRY_MOUNT_USB_UX0:
		{
			mountUsbUx0();
			break;
		}
		
		case MENU_HOME_ENTRY_UMOUNT_USB_UX0:
		{
			umountUsbUx0();
			break;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

static int contextMenuMainEnterCallback(int sel, void *context) {
	switch (sel) {
		case MENU_MAIN_ENTRY_MARK_UNMARK_ALL:
		{
			int on_marked_entry = 0;
			int length = mark_list.length;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);
			if (fileListFindEntry(&mark_list, file_entry->name))
				on_marked_entry = 1;

			// Empty mark list
			fileListEmpty(&mark_list);

			// Mark all if not all entries are marked yet and we are not focusing on a marked entry
			if (length != (file_list.length-1) && !on_marked_entry) {
				FileListEntry *file_entry = file_list.head->next; // Ignore '..'

				int i;
				for (i = 0; i < file_list.length-1; i++) {
					FileListEntry *mark_entry = malloc(sizeof(FileListEntry));
					memcpy(mark_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&mark_list, mark_entry, SORT_NONE);

					// Next
					file_entry = file_entry->next;
				}
			}

			break;
		}
		
		case MENU_MAIN_ENTRY_MOVE:
		case MENU_MAIN_ENTRY_COPY:
		{
			// Mode
			if (sel == MENU_MAIN_ENTRY_MOVE) {
				copy_mode = COPY_MODE_MOVE;
			} else {
				copy_mode = isInArchive() ? COPY_MODE_EXTRACT : COPY_MODE_NORMAL;
			}
			
			file_type = getArchiveType();
			strcpy(archive_copy_path,archive_path);

			// Empty copy list at first
			fileListEmpty(&copy_list);

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

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
			if (copy_list.length > 1 && fileListFindEntry(&copy_list, file_entry->name)) {
				message = language_container[COPIED_FILES_FOLDERS];
			} else {
				message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
			}

			// Copy message
			infoDialog(message, copy_list.length);

			break;
		}

		case MENU_MAIN_ENTRY_PASTE:
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
			setDialogStep(DIALOG_STEP_PASTE);
			break;
		}

		case MENU_MAIN_ENTRY_DELETE:
		{
			char *message;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

			// On marked entry
			if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
				message = language_container[DELETE_FILES_FOLDERS_QUESTION];
			} else {
				message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
			}

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
			setDialogStep(DIALOG_STEP_DELETE_QUESTION);
			break;
		}
		
		case MENU_MAIN_ENTRY_RENAME:
		{
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

			char name[MAX_NAME_LENGTH];
			strcpy(name, file_entry->name);
			removeEndSlash(name);

			initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);

			setDialogStep(DIALOG_STEP_RENAME);
			break;
		}
		
		case MENU_MAIN_ENTRY_PROPERTIES:
		{
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			initPropertyDialog(cur_file, file_entry);

			break;
		}
		
		case MENU_MAIN_ENTRY_NEW_FOLDER:
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
				memset(&stat, 0, sizeof(SceIoStat));
				if (sceIoGetstat(path, &stat) < 0)
					break;

				count++;
			}

			initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path), MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);
			setDialogStep(DIALOG_STEP_NEW_FOLDER);
			break;
		}
		
		case MENU_MAIN_ENTRY_MORE:
		{
			setContextMenu(&context_menu_more);
			setContextMenuMoreVisibilities();
			return CONTEXT_MENU_MORE_OPENING;
		}
		
		case MENU_MAIN_ENTRY_SORT_BY:
		{
			setContextMenu(&context_menu_sort);
			setContextMenuSortVisibilities();
			return CONTEXT_MENU_MORE_OPENING;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

static int contextMenuSortEnterCallback(int sel, void *context) {
	switch (sel) {
		case MENU_SORT_ENTRY_BY_NAME:
			sort_mode = SORT_BY_NAME;
			break;
			
		case MENU_SORT_ENTRY_BY_SIZE:
			sort_mode = SORT_BY_SIZE;
			break;
			
		case MENU_SORT_ENTRY_BY_DATE:
			sort_mode = SORT_BY_DATE;
			break;
	}

	// Refresh list
	refreshFileList();

	return CONTEXT_MENU_CLOSING;
}

static int contextMenuMoreEnterCallback(int sel, void *context) {
	switch (sel) {
		case MENU_MORE_ENTRY_COMPRESS:
		{
			char path[MAX_NAME_LENGTH];

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

			// On marked entry
			if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
				int end_slash = removeEndSlash(file_list.path);

				char *p = strrchr(file_list.path, '/');
				if (!p)
					p = strrchr(file_list.path, ':');

				if (strlen(p+1) > 0) {
					strcpy(path, p+1);
				} else {
					strncpy(path, file_list.path, p - file_list.path);
					path[p - file_list.path] = '\0';
				}

				if (end_slash)
					addEndSlash(file_list.path);				
			} else {
				char *p = strrchr(file_entry->name, '.');
				if (!p)
					p = strrchr(file_entry->name, '/');
				if (!p)
					p = file_entry->name+strlen(file_entry->name);

				strncpy(path, file_entry->name, p-file_entry->name);
				path[p - file_entry->name] = '\0';
			}

			// Append .zip extension
			strcat(path, ".zip");

			initImeDialog(language_container[ARCHIVE_NAME], path, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);
			setDialogStep(DIALOG_STEP_COMPRESS_NAME);
			break;
		}
		
		case MENU_MORE_ENTRY_INSTALL_ALL:
		{
			// Empty install list
			fileListEmpty(&install_list);

			FileListEntry *file_entry = file_list.head->next; // Ignore '..'

			int i;
			for (i = 0; i < file_list.length-1; i++) {
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
			setDialogStep(DIALOG_STEP_INSTALL_QUESTION);
			
			break;
		}

		case MENU_MORE_ENTRY_INSTALL_FOLDER:
		{
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_FOLDER_QUESTION]);
			setDialogStep(DIALOG_STEP_INSTALL_QUESTION);
			break;
		}
		
		case MENU_MORE_ENTRY_EXPORT_MEDIA:
		{
			char *message;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos+rel_pos);

			// On marked entry
			if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
				message = language_container[EXPORT_FILES_FOLDERS_QUESTION];
			} else {
				message = language_container[file_entry->is_folder ? EXPORT_FOLDER_QUESTION : EXPORT_FILE_QUESTION];
			}

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
			setDialogStep(DIALOG_STEP_EXPORT_QUESTION);
			break;
		}
		
		case MENU_MORE_ENTRY_CALCULATE_SHA1:
		{
			// Ensure user wants to actually take the hash
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[HASH_FILE_QUESTION]);
			setDialogStep(DIALOG_STEP_HASH_QUESTION);
			break;
		}
	}

	return CONTEXT_MENU_CLOSING;
}