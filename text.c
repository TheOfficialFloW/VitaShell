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
#include "archive.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "theme.h"
#include "utils.h"

void textListAddEntry(TextList *list, TextListEntry *entry) {
	entry->next = NULL;
	entry->previous = NULL;

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		TextListEntry *tail = list->tail;
		tail->next = entry;
		entry->previous = tail;
		list->tail = entry;
	}

	list->length++;
}

void textListEmpty(TextList *list) {
	TextListEntry *entry = list->head;

	while (entry) {
		TextListEntry *next = entry->next;
		free(entry);
		entry = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->length = 0;
}

#define TAB_SIZE 4

int textReadLine(char *buffer, int offset, int size, char *line) {
	// Get line
	int line_width = 0;
	int count = 0;

	int i;
	for (i = 0; i < MIN(size, MIN(size - offset, MAX_LINE_CHARACTERS - 1)); i++) {
		char ch = buffer[offset + i];
		char ch_width = 0;

		// Line break
		if (ch == '\n') {
			i++; // Skip it
			break;
		}

		// Tab
		if (ch == '\t') {
			ch_width = TAB_SIZE * font_size_cache[' '];
		} else {
			ch_width = font_size_cache[(int)ch];
			if (ch_width == 0) {
				ch = ' '; // Change invalid characters to space
				ch_width = font_size_cache[(int)ch];
			}
		}

		// Too long
		if ((line_width + ch_width) >= MAX_WIDTH)
			break;

		// Increase line width
		line_width += ch_width;

		// Add to line string
		if (line)
			line[count++] = ch;
	}

	// End of line
	if (line)
		line[count] = '\0';

	return i;
}

static int running = 0;
static int n_lines = 0;

int text_thread(SceSize args, uint32_t *argp) {
	char *buffer = (char *)argp[0];
	int size = (int)argp[1];

	running = 1;
	n_lines = 0;

	int offset = 0;

	while (running && offset < size && n_lines < MAX_LINES) {
		offset += textReadLine(buffer, offset, size, NULL);
		n_lines++;

		sceKernelDelayThread(1000);
	}

	return sceKernelExitDeleteThread(0);
}

int textViewer(char *file) {
	int hex_viewer = 0;

	char *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	int size = 0;

	if (isInArchive()) {
		size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE);
	} else {
		size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	}

	if (size <= 0) {
		free(buffer);
		return size;
	}

	int base_pos = 0, rel_pos = 0;

	int *offset_list = (int *)malloc(MAX_LINES * sizeof(int));
	memset(offset_list, 0, MAX_LINES * sizeof(int));

	TextList list;
	memset(&list, 0, sizeof(TextList));

	int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
		TextListEntry *entry = malloc(sizeof(TextListEntry));

		int length = textReadLine(buffer, offset_list[i], size, entry->line);
		offset_list[i + 1] = offset_list[i] + length;

		textListAddEntry(&list, entry);
	}

	uint32_t argp[2];
	argp[0] = (uint32_t)buffer;
	argp[1] = (uint32_t)size;

	SceUID thid = sceKernelCreateThread("text_thread", (SceKernelThreadEntry)text_thread, 0x10000100, 0x10000, 0, 0x70000, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, sizeof(argp), &argp);

	while (1) {
		readPad();

		if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			if (rel_pos > 0) {
				rel_pos--;
			} else {
				if (base_pos > 0) {
					base_pos--;

					// Tail to head
					list.tail->next = list.head;
					list.head->previous = list.tail;
					list.head = list.tail;

					// Second last to tail
					list.tail = list.tail->previous;
					list.tail->next = NULL;

					// No previous
					list.head->previous = NULL;

					// Read
					textReadLine(buffer, offset_list[base_pos], size, list.head->line);
				}
			}
		} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
			if (offset_list[rel_pos + 1] < size) {
				if ((rel_pos + 1) < MAX_POSITION) {
					rel_pos++;
				} else {
					if (offset_list[base_pos + rel_pos + 1] < size) {
						base_pos++;

						// Head to tail
						list.head->previous = list.tail;
						list.tail->next = list.head;
						list.tail = list.head;

						// Second first to head
						list.head = list.head->next;
						list.head->previous = NULL;

						// No next
						list.tail->next = NULL;

						// Read
						int length = textReadLine(buffer, offset_list[base_pos + MAX_ENTRIES - 1], size, list.tail->line);
						offset_list[base_pos + MAX_ENTRIES] = offset_list[base_pos + MAX_ENTRIES - 1] + length;
					}
				}
			}
		}

		// Page skip
		if (hold_buttons & SCE_CTRL_LTRIGGER) {

		}

		if (hold_buttons & SCE_CTRL_RTRIGGER) {

		}

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			hex_viewer = 0;
			break;
		}

		// Switch to hex viewer
		if (pressed_buttons & SCE_CTRL_SQUARE) {
			hex_viewer = 1;
			break;
		}

		// Start drawing
		startDrawing(bg_text_image);

		// Draw shell info
		drawShellInfo(file);

		// Draw scroll bar
		drawScrollBar(base_pos, n_lines);

		// Text
		TextListEntry *entry = list.head;

		int i;
		for (i = 0; i < list.length; i++) {
			char *line = entry->line;

			float x = SHELL_MARGIN_X;

			while (*line) {
				char *p = strchr(line, '\t');
				if (p)
					*p = '\0';

				int width = pgf_draw_text(x, START_Y + (i * FONT_Y_SPACE), (rel_pos == i) ? FOCUS_COLOR : GENERAL_COLOR, FONT_SIZE, line);
				line += strlen(line);

				if (p) {
					*p = '\t';
					x += width + TAB_SIZE * font_size_cache[' '];
					line++;
				}
			}

			entry = entry->next;
		}

		// End drawing
		endDrawing();
	}

	running = 0;
	sceKernelWaitThreadEnd(thid, NULL, NULL);

	free(offset_list);
	textListEmpty(&list);

	free(buffer);

	if (hex_viewer)
		hexViewer(file);

	return 0;
}
