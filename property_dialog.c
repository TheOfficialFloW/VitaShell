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
#include "archive.h"
#include "archiveRAR.h"
#include "init.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "property_dialog.h"

typedef struct {
	int dialog_status;
	float info_x;
	float x;
	float y;
	float width;
	float height;
	float scale;
} PropertyDialog;

static PropertyDialog property_dialog;

static char property_name[128], property_type[32], property_fself_mode[16];
static char property_size[16], property_compressed_size[16], property_contains[32];
static char property_creation_date[64], property_modification_date[64];

#define PROPERTY_DIALOG_ENTRY_MIN_WIDTH 240
#define PROPERTY_DIALOG_ENTRY_MAX_WIDTH 580

typedef struct {
	int name;
	int visibility;
	char *entry;
	int entry_size;
} PropertyEntry;

/*
	TODO:
	- Audio information
	- Image information
*/

PropertyEntry property_entries[] = {
	{ PROPERTY_NAME, PROPERTY_ENTRY_VISIBLE, property_name, sizeof(property_name) },
	{ PROPERTY_TYPE, PROPERTY_ENTRY_VISIBLE, property_type, sizeof(property_type) },
	{ PROPERTY_FSELF_MODE, PROPERTY_ENTRY_VISIBLE, property_fself_mode, sizeof(property_fself_mode) },
	{ PROPERTY_SIZE, PROPERTY_ENTRY_VISIBLE, property_size, sizeof(property_size) },
	{ PROPERTY_COMPRESSED_SIZE, PROPERTY_ENTRY_VISIBLE, property_compressed_size, sizeof(property_compressed_size) },
	{ PROPERTY_CONTAINS, PROPERTY_ENTRY_VISIBLE, property_contains, sizeof(property_contains) },
	{ -1, PROPERTY_ENTRY_UNUSED, NULL },
	{ PROPERTY_CREATION_DATE, PROPERTY_ENTRY_VISIBLE, property_creation_date, sizeof(property_creation_date) },
	{ PROPERTY_MODFICATION_DATE, PROPERTY_ENTRY_VISIBLE, property_modification_date, sizeof(property_modification_date) },
};

enum PropertyEntries {
	PROPERTY_ENTRY_NAME,
	PROPERTY_ENTRY_TYPE,
	PROPERTY_ENTRY_FSELF_MODE,
	PROPERTY_ENTRY_SIZE,
	PROPERTY_ENTRY_COMPRESSED_SIZE,
	PROPERTY_ENTRY_CONTAINS,
	PROPERTY_ENTRY_EMPTY_1,
	PROPERTY_ENTRY_CREATION_DATE,
	PROPERTY_ENTRY_MODIFICATION_DATE,
};

#define N_PROPERTIES_ENTRIES (sizeof(property_entries) / sizeof(PropertyEntry))

int getPropertyDialogStatus() {
	return property_dialog.dialog_status;
}

int copyStringLimited(char *out, char *in, int limit) {
	int line_width = 0;

	int j;
	for (j = 0; j < strlen(in); j++) {
		char ch_width = font_size_cache[(int)in[j]];

		// Too long
		if ((line_width + ch_width) >= limit)
			break;

		// Increase line width
		line_width += ch_width;
	}

	strncpy(out, in, j);
	out[j] = '\0';

	return line_width;
}

typedef struct {
	char *path;
} InfoArguments;

SceUID info_thid = -1;
int info_done = 0;

int propertyCancelHandler() {
	return info_done;
}

int info_thread(SceSize args_size, InfoArguments *args) {
	uint64_t size = 0;
	uint32_t folders = 0, files = 0;

	info_done = 0;
	int res = getPathInfo(args->path, &size, &folders, &files, propertyCancelHandler);
	info_done = 1;

	if (folders > 0)
		folders--;

	getSizeString(property_size, size);

	snprintf(property_contains, sizeof(property_contains), language_container[PROPERTY_CONTAINS_FILES_FOLDERS], files, folders);

	return sceKernelExitDeleteThread(0);
}

int initPropertyDialog(char *path, FileListEntry *entry) {
	int i;

	memset(&property_dialog, 0, sizeof(PropertyDialog));

	// Opening status
	property_dialog.dialog_status = PROPERTY_DIALOG_OPENING;

	// Get info x
	property_dialog.info_x = 0.0f;

	for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
		if (property_entries[i].name != -1) {
			float width = vita2d_pgf_text_width(font, FONT_SIZE, language_container[property_entries[i].name]);
			if (width > property_dialog.info_x)
				property_dialog.info_x = width;

			property_entries[i].visibility = PROPERTY_ENTRY_VISIBLE;
		} else {
			property_entries[i].visibility = PROPERTY_ENTRY_UNUSED;
		}
	}

	// Extend
	property_dialog.info_x += 2.0f * SHELL_MARGIN_X;

	// Entries
	int width = 0, max_width = 0;

	char *p;

	// Name
	char name[128];
	strcpy(name, entry->name);
	
	if (entry->is_folder)
		removeEndSlash(name);

	width = copyStringLimited(property_name, name, PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
	if (width > max_width)
		max_width = width;

	// Type
	int type = entry->is_folder ? FOLDER : FILE_;

	int size = 0;
	uint32_t buffer[0x88/4];

	if (isInArchive()) {
		enum FileTypes archiveType = getArchiveType();
		switch(archiveType){
			case FILE_TYPE_ZIP:
				size = ReadArchiveFile(path, buffer, sizeof(buffer));
				break;
			case FILE_TYPE_RAR:
				size = ReadArchiveRARFile(path,buffer,sizeof(buffer));
				break;
			default:
				size = -1;
				break;
		}
	} else {
		size = ReadFile(path, buffer, sizeof(buffer));
	}

	// FSELF mode
	if (*(uint32_t *)buffer == 0x00454353) {
		type = PROPERTY_TYPE_FSELF;

		// Check authid flag
		uint64_t authid = *(uint64_t *)((uint32_t)buffer + 0x80);
		if (authid == 0x2F00000000000001 || authid == 0x2F00000000000003) {			
			width = copyStringLimited(property_fself_mode, language_container[PROPERTY_FSELF_MODE_UNSAFE], PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
		} else if (authid == 0x2F00000000000002) {
			width = copyStringLimited(property_fself_mode, language_container[PROPERTY_FSELF_MODE_SAFE], PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
		} else {
			width = copyStringLimited(property_fself_mode, language_container[PROPERTY_FSELF_MODE_SCE], PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
		}

		if (width > max_width)
			max_width = width;

		property_entries[PROPERTY_ENTRY_FSELF_MODE].visibility = PROPERTY_ENTRY_VISIBLE;
	} else {
		property_entries[PROPERTY_ENTRY_FSELF_MODE].visibility = PROPERTY_ENTRY_INVISIBLE;
	}

	switch (entry->type) {
		case FILE_TYPE_BMP:
			type = PROPERTY_TYPE_BMP;
			break;
			
		case FILE_TYPE_INI:
			type = PROPERTY_TYPE_INI;
			break;
			
		case FILE_TYPE_JPEG:
			type = PROPERTY_TYPE_JPEG;
			break;
			
		case FILE_TYPE_MP3:
			type = PROPERTY_TYPE_MP3;
			break;
			
		case FILE_TYPE_OGG:
			type = PROPERTY_TYPE_OGG;
			break;
			
		case FILE_TYPE_PNG:
			type = PROPERTY_TYPE_PNG;
			break;
			
		case FILE_TYPE_SFO:
			type = PROPERTY_TYPE_SFO;
			break;
			
		case FILE_TYPE_TXT:
			type = PROPERTY_TYPE_TXT;
			break;
			
		case FILE_TYPE_VPK:
			type = PROPERTY_TYPE_VPK;
			break;
			
		case FILE_TYPE_XML:
			type = PROPERTY_TYPE_XML;
			break;
			
		case FILE_TYPE_ZIP:	
			type = PROPERTY_TYPE_ZIP;
			break;
			
		case FILE_TYPE_RAR:
			type = PROPERTY_TYPE_RAR;
			break;
	}

	width = copyStringLimited(property_type, language_container[type], PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
	if (width > max_width)
		max_width = width;

	// Size & contains
	if (entry->is_folder) {
		if (isInArchive()) {
			property_entries[PROPERTY_ENTRY_SIZE].visibility = PROPERTY_ENTRY_INVISIBLE;
			property_entries[PROPERTY_ENTRY_CONTAINS].visibility = PROPERTY_ENTRY_INVISIBLE;
		} else {
			strcpy(property_size, "...");
			strcpy(property_contains, "...");

			// Info thread
			InfoArguments info_args;
			info_args.path = path;

			info_thid = sceKernelCreateThread("info_thread", (SceKernelThreadEntry)info_thread, 0x10000100, 0x100000, 0, 0, NULL);
			if (info_thid >= 0)
				sceKernelStartThread(info_thid, sizeof(InfoArguments), &info_args);
			
			property_entries[PROPERTY_ENTRY_CONTAINS].visibility = PROPERTY_ENTRY_VISIBLE;
		}

		property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_INVISIBLE;
	} else {
		getSizeString(property_size, entry->size);
		property_entries[PROPERTY_ENTRY_CONTAINS].visibility = PROPERTY_ENTRY_INVISIBLE;
		
		// Compressed size
		if (isInArchive()) {
			getSizeString(property_compressed_size, entry->size2);
			property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_VISIBLE;
		} else {
			property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_INVISIBLE;
		}
	}

	// Dates
	char date_string[16];
	char time_string[24];
	char string[64];

	// Modification date
	getDateString(date_string, date_format, &entry->mtime);
	getTimeString(time_string, time_format, &entry->mtime);
	sprintf(string, "%s %s", date_string, time_string);
	width = copyStringLimited(property_modification_date, string, PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
	if (width > max_width)
		max_width = width;

	// Creation date
	getDateString(date_string, date_format, &entry->ctime);
	getTimeString(time_string, time_format, &entry->ctime);
	sprintf(string, "%s %s", date_string, time_string);
	width = copyStringLimited(property_creation_date, string, PROPERTY_DIALOG_ENTRY_MAX_WIDTH);
	if (width > max_width)
		max_width = width;

	// Number of entries
	int j = 0;

	for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
		if (property_entries[i].visibility != PROPERTY_ENTRY_INVISIBLE)
			j++;
	}

	// Width and height
	property_dialog.width = property_dialog.info_x + (float)MAX(max_width, PROPERTY_DIALOG_ENTRY_MIN_WIDTH);
	property_dialog.height = FONT_Y_SPACE * j;

	// For buttons
	property_dialog.height += 2.0f * FONT_Y_SPACE;

	// Margin
	property_dialog.width += 2.0f * SHELL_MARGIN_X;
	property_dialog.height += 2.0f * SHELL_MARGIN_Y;

	// Position
	property_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, property_dialog.width);
	property_dialog.y = ALIGN_CENTER(SCREEN_HEIGHT, property_dialog.height);

	// Align
	int y_n = (int)((float)(property_dialog.y - 2.0f) / FONT_Y_SPACE);
	property_dialog.y = (float)y_n * FONT_Y_SPACE + 2.0f;

	// Scale
	property_dialog.scale = 0.0f;

	return 0;
}

void propertyDialogCtrl() {
	if (pressed_buttons & SCE_CTRL_ENTER) {
		info_done = 1;
		sceKernelWaitThreadEnd(info_thid, NULL, NULL);
		property_dialog.dialog_status = PROPERTY_DIALOG_CLOSING;
	}
}

static float easeOut(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.01f) ? (dx * a) : dx;
}

void drawPropertyDialog() {
	if (property_dialog.dialog_status == PROPERTY_DIALOG_CLOSED)
		return;

	// Dialog background
	float dialog_width = vita2d_texture_get_width(dialog_image);
	float dialog_height = vita2d_texture_get_height(dialog_image);
	vita2d_draw_texture_scale_rotate_hotspot(dialog_image, property_dialog.x + property_dialog.width / 2.0f,
														property_dialog.y + property_dialog.height / 2.0f,
														property_dialog.scale * (property_dialog.width / dialog_width),
														property_dialog.scale * (property_dialog.height / dialog_height),
														0.0f, dialog_width / 2.0f, dialog_height / 2.0f);

	// Easing out
	if (property_dialog.dialog_status == PROPERTY_DIALOG_CLOSING) {
		if (property_dialog.scale > 0.0f) {
			property_dialog.scale -= easeOut(0.0f, property_dialog.scale, 0.25f);
		} else {
			property_dialog.dialog_status = PROPERTY_DIALOG_CLOSED;
		}
	}

	if (property_dialog.dialog_status == PROPERTY_DIALOG_OPENING) {
		if (property_dialog.scale < 1.0f) {
			property_dialog.scale += easeOut(property_dialog.scale, 1.0f, 0.25f);
		} else {
			property_dialog.dialog_status = PROPERTY_DIALOG_OPENED;
		}
	}

	if (property_dialog.dialog_status == PROPERTY_DIALOG_OPENED) {
		float string_y = property_dialog.y + SHELL_MARGIN_Y - 2.0f;

		int i;
		for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
			if (property_entries[i].visibility == PROPERTY_ENTRY_VISIBLE) {
				pgf_draw_text(property_dialog.x + SHELL_MARGIN_X, string_y, DIALOG_COLOR, FONT_SIZE, language_container[property_entries[i].name]);

				if (property_entries[i].entry != NULL)
					pgf_draw_text(property_dialog.x + SHELL_MARGIN_X + property_dialog.info_x, string_y, DIALOG_COLOR, FONT_SIZE, property_entries[i].entry);
			}

			if (property_entries[i].visibility != PROPERTY_ENTRY_INVISIBLE)
				string_y += FONT_Y_SPACE;
		}

		char button_string[32];
		sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK]);
		pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, button_string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, FONT_SIZE, button_string);
	}
}