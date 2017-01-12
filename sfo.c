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
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"

int getSfoValue(void *buffer, char *name, uint32_t *value) {
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
    	return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
			*value = *(uint32_t *)(buffer + header->valofs + entries[i].dataofs);
			return 0;
		}
	}

	return -2;
}

int getSfoString(void *buffer, char *name, char *string, int length) {
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
    	return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
			memset(string, 0, length);
			strncpy(string, buffer + header->valofs + entries[i].dataofs, length);
			string[length - 1] = '\0';
			return 0;
		}
	}

	return -2;
}

int setSfoValue(void *buffer, char *name, uint32_t value) {
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
    	return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
			*(uint32_t *)(buffer + header->valofs + entries[i].dataofs) = value;
			return 0;
		}
	}

	return -2;
}

int setSfoString(void *buffer, char *name, char *string) {
	SfoHeader *header = (SfoHeader *)buffer;
	SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

	if (header->magic != SFO_MAGIC)
    	return -1;

	int i;
	for (i = 0; i < header->count; i++) {
		if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
			strcpy(buffer + header->valofs + entries[i].dataofs, string);
			return 0;
		}
	}

	return -2;
}

int SFOReader(char *file) {
	uint8_t *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	int size = 0;

	if (isInArchive()) {
		enum FileTypes archiveType = getArchiveType();
		switch(archiveType){
			case FILE_TYPE_ZIP: 
				size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE); 
				break; 
			case FILE_TYPE_RAR: 
				size = ReadArchiveRARFile(file,buffer,BIG_BUFFER_SIZE);
				break;
			default:
				size = -1;
				break;
			}
	}else {
		size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	}

	if (size <= 0) {
		free(buffer);
		return size;
	}

	SfoHeader *sfo_header = (SfoHeader *)buffer;
	if (sfo_header->magic != SFO_MAGIC)
    	return -1;

	int base_pos = 0, rel_pos = 0;

	while (1) {
		readPad();

		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;	
		}

		if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			if (rel_pos > 0) {
				rel_pos--;
			} else {
				if (base_pos > 0) {
					base_pos--;
				}
			}
		} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
			if ((rel_pos + 1) < sfo_header->count) {
				if ((rel_pos + 1) < MAX_POSITION) {
					rel_pos++;
				} else {
					if ((base_pos + rel_pos + 1) < sfo_header->count) {
						base_pos++;
					}
				}
			}
		}

		// Start drawing
		startDrawing(bg_text_image);

		// Draw shell info
		drawShellInfo(file);

	    // Draw scroll bar
	   	drawScrollBar(base_pos, sfo_header->count);

		int i;
		for (i = 0; i < MAX_ENTRIES && (base_pos + i) < sfo_header->count; i++) {
			SfoEntry *entries = (SfoEntry *)(buffer + sizeof(SfoHeader) + (sizeof(SfoEntry) * (i + base_pos)));

			uint32_t color = (rel_pos == i) ? TEXT_FOCUS_COLOR : TEXT_COLOR;

	    	char *name = (char *)buffer + sfo_header->keyofs + entries->nameofs;
			pgf_draw_textf(SHELL_MARGIN_X, START_Y + (FONT_Y_SPACE * i), color, FONT_SIZE, "%s", name);

			char string[128];

			void *data = (void *)buffer + sfo_header->valofs + entries->dataofs;
			switch (entries->type) {
				case PSF_TYPE_BIN:
					break;
					
				case PSF_TYPE_STR:
					snprintf(string, sizeof(string), "%s", (char *)data);
					break;
					
				case PSF_TYPE_VAL:
					snprintf(string, sizeof(string), "%X", *(unsigned int *)data);
					break;
			}

			pgf_draw_textf(ALIGN_RIGHT(INFORMATION_X, vita2d_pgf_text_width(font, FONT_SIZE, string)), START_Y + (FONT_Y_SPACE * i), color, FONT_SIZE, string);
		}

		// End drawing
		endDrawing();
	}

	free(buffer);
	return 0;
}
