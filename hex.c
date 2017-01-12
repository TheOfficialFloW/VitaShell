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
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"

void hexListAddEntry(HexList *list, HexListEntry *entry) {
	entry->next = NULL;
	entry->previous = NULL;

	if (list->head == NULL) {
		list->head = entry;
		list->tail = entry;
	} else {
		HexListEntry *tail = list->tail;
		tail->next = entry;
		entry->previous = tail;
		list->tail = entry;
	}

	list->length++;
}

void hexListEmpty(HexList *list) {
	HexListEntry *entry = list->head;

	while (entry) {
		HexListEntry *next = entry->next;
		free(entry);
		entry = next;
	}

	list->head = NULL;
	list->tail = NULL;
	list->length = 0;
}

HexListEntry *hexListGetNthEntry(HexList *list, int n) {
	HexListEntry *entry = list->head;

	while (n > 0 && entry) {
		n--;
		entry = entry->next;
	}

	if (n != 0)
		return NULL;

	return entry;
}

int hexViewer(char *file) {
	int text_viewer = 0;

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
	} else {
		size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	}

	if (size <= 0) {
		free(buffer);
		return size;
	}

	int modify_allowed = 1;

	if (isInArchive()) {
		modify_allowed = 0;
	}

	int changed = 0;
	int save_question = 0;

	int base_pos = 0, rel_pos = 0;
	uint8_t nibble_pos = 0;

	HexList list;
	memset(&list, 0, sizeof(HexList));

	int i;
	for (i = 0; i < 0x10; i++) {
		HexListEntry *entry = malloc(sizeof(HexListEntry));
		memcpy(entry->data, buffer + i * 0x10, 0x10);
		hexListAddEntry(&list, entry);
	}

	while (1) {
		readPad();

		if (!save_question) {
			if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
				if (rel_pos > 0) {
					rel_pos -= 0x10;
				} else {
					if (base_pos > 0) {
						base_pos -= 0x10;

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
						memcpy(list.head->data, buffer + base_pos, 0x10);
					}
				}
			} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
				if ((rel_pos + 0x10) < size) {
					if ((rel_pos + 0x10) < ((MAX_POSITION - 1) * 0x10)) {
						rel_pos += 0x10;
					} else {
						if ((base_pos + rel_pos + 0x10) < size) {
							base_pos += 0x10;

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
							memcpy(list.tail->data, buffer + base_pos + (0x10 - 1) * 0x10, 0x10);
						}
					}
				}
			}

			// Page skip
			if (hold_buttons & SCE_CTRL_LTRIGGER) {
				if ((base_pos + rel_pos) != 0) {
					if ((base_pos - 0x10 * 0x10) >= 0) {
						base_pos -= 0x10 * 0x10;
					} else {
						base_pos = 0;
						rel_pos = 0;
					}

					HexListEntry *entry = list.head;

					int i;
					for (i = 0; i < 0x10; i++) {
						memcpy(entry->data, buffer + base_pos + i * 0x10, 0x10);
						entry = entry->next;
					}
				}
			}

			if (hold_buttons & SCE_CTRL_RTRIGGER) {
				if (size >= 0xF0) {
					if ((base_pos + rel_pos + 0x1F0) < size) {
						base_pos += 0x10 * 0x10;
					} else {
						base_pos = ALIGN(size, 0x10) - 0xF0;
						rel_pos = 0xE0;
					}

					HexListEntry *entry = list.head;

					int i;
					for (i = 0; i < 0x10; i++) {
						memcpy(entry->data, buffer + base_pos + i * 0x10, 0x10);
						entry = entry->next;
					}
				}
			}

			uint8_t max_nibble = (2 * 0x10) - 1;

			// Last line
			if ((base_pos + rel_pos + 0x10) >= size) {
				uint8_t rest = size % 0x10;

				if (rest == 0)
					rest = 0x10;

				max_nibble = 2 * rest - 1;
			}

			if (nibble_pos > max_nibble) {
				nibble_pos = max_nibble;
			}

			if (hold_buttons & SCE_CTRL_LEFT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
				if (nibble_pos > 0)
					nibble_pos--;
			} else if (hold_buttons & SCE_CTRL_RIGHT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
				if (nibble_pos < max_nibble)
					nibble_pos++;
			}

			// Cancel or switch to text viewer
			if (pressed_buttons & SCE_CTRL_CANCEL || pressed_buttons & SCE_CTRL_SQUARE) {
				if (pressed_buttons & SCE_CTRL_CANCEL) {
					text_viewer = 0;
				} else {
					text_viewer = 1;
				}

				if (changed) {
					initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[SAVE_MODIFICATIONS]);
					save_question = 1;
				} else {
					break;
				}
			}

			// Increase nibble
			if (modify_allowed && hold_buttons & SCE_CTRL_ENTER) {
				changed = 1;
				int cur_pos = rel_pos + base_pos + nibble_pos / 2;

				uint8_t ch = buffer[cur_pos];
				uint8_t high_nibble = (ch >> 4) & 0xF;
				uint8_t low_nibble = ch & 0xF;

				int low = (nibble_pos % 2);
				uint8_t nibble = low ? (ch & 0xF) : ((ch >> 4) & 0xF);

				if (nibble < 0xF) {
					nibble++;
				} else {
					nibble = 0;
				}

				HexListEntry *entry = hexListGetNthEntry(&list, rel_pos / 0x10);

				if (low) {
					uint8_t byte = (high_nibble << 4) | nibble;
					buffer[cur_pos] = byte;
					entry->data[nibble_pos / 2] = byte;
				} else {
					uint8_t byte = (nibble << 4) | low_nibble;
					buffer[cur_pos] = byte;
					entry->data[nibble_pos / 2] = byte;
				}
			}
		} else {
			int msg_result = updateMessageDialog();
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				SceUID fd = sceIoOpen(file, SCE_O_WRONLY, 0777);
				if (fd >= 0) {
					sceIoWrite(fd, buffer, size);
					sceIoClose(fd);
				}

				break;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				break;
			}
		}

		// Start drawing
		startDrawing(bg_hex_image);

		// Draw shell info
		drawShellInfo(file);

		// Draw scroll bar
		int pos = (base_pos - (base_pos % 0x10)) / 0x10 + ((base_pos % 0x10) ? 1 : 0);
		int n_lines = (size - (size % 0x10)) / 0x10 + ((size % 0x10) ? 1 : 0);
		drawScrollBar(pos, n_lines);

		// Offset/size
		pgf_draw_textf(HEX_CHAR_X, START_Y, HEX_OFFSET_COLOR, FONT_SIZE, "%08X/%08X", rel_pos + base_pos, size);

		// Offset x
		pgf_draw_text(SHELL_MARGIN_X, START_Y, HEX_OFFSET_COLOR, FONT_SIZE, language_container[OFFSET]);

		int x;
		for (x = 0; x < 0x10; x++) {
			pgf_draw_textf(HEX_OFFSET_X + (x * HEX_OFFSET_SPACE), START_Y, HEX_OFFSET_COLOR, FONT_SIZE, "%02X", x);
		}

		HexListEntry *entry = list.head;

		int y;
		for (y = 0; y < 0x10; y++) {
			int x;
			for (x = 0; x < 0x10; x++) {
				uint8_t nibble_x = x * 2;

				uint8_t ch = entry->data[x];

				uint32_t offset = base_pos + x + y * 0x10;
				if (offset >= size)
					break;

				uint32_t color = HEX_COLOR;

				int on_line = 0;
				if (rel_pos == (y * 0x10)) {
					color = FOCUS_COLOR;
					on_line = 1;
				}

				// Character hex
				uint8_t high_nibble = (ch >> 4) & 0xF;
				uint8_t low_nibble = ch & 0xF;
				int w = pgf_draw_textf(HEX_OFFSET_X + (x * HEX_OFFSET_SPACE), START_Y + ((y + 1) * FONT_Y_SPACE), (on_line && nibble_x == nibble_pos) ? HEX_NIBBLE_COLOR : color, FONT_SIZE, "%01X", high_nibble);
				pgf_draw_textf(HEX_OFFSET_X + (x * HEX_OFFSET_SPACE) + w, START_Y + ((y + 1) * FONT_Y_SPACE), (on_line && (nibble_x + 1) == nibble_pos) ? HEX_NIBBLE_COLOR : color, FONT_SIZE, "%01X", low_nibble);

				// Character
				ch = (ch >= 0x20) ? ch : '.';
				int width = font_size_cache[(int)ch];
				uint8_t byte_nibble_pos = nibble_pos - (nibble_pos % 2);
				pgf_draw_textf(HEX_CHAR_X + (x * FONT_X_SPACE) + (FONT_X_SPACE - width) / 2.0f, START_Y + ((y + 1) * FONT_Y_SPACE), (on_line && nibble_x == byte_nibble_pos) ? HEX_NIBBLE_COLOR : color, FONT_SIZE, "%c", ch);
			}

			// Offset y
			if (x > 0)
				pgf_draw_textf(SHELL_MARGIN_X, START_Y + ((y + 1) * FONT_Y_SPACE), HEX_OFFSET_COLOR, FONT_SIZE, "%08X", base_pos + (y * 0x10));

			// It's the end, break
			if (x < 0x10)
				break;

			// Next
			entry = entry->next;
		}

		// End drawing
		endDrawing();
	}

	hexListEmpty(&list);

	free(buffer);

	if (text_viewer)
		textViewer(file);

	return 0;
}
