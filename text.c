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
#include "language.h"
#include "ime_dialog.h"
#include "message_dialog.h"

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

void updateTextEntries(char *buffer, int base_pos, int size, int *offset_list, TextListEntry *entry) {
	int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
		if (!entry) {
			break;
		}
		
		entry->line_number = base_pos + i;

		int length = textReadLine(buffer, offset_list[base_pos + i], size, entry->line);
		offset_list[base_pos + i + 1] = offset_list[base_pos + i] + length;

		entry = entry->next;
	}
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

	char *buffer_base = malloc(BIG_BUFFER_SIZE);
	if (!buffer_base)
		return -1;

	CopyEntry *copy_buffer = malloc(MAX_COPY_BUFFER_SIZE * sizeof(CopyEntry));
	if (!copy_buffer)
		return -1;

	int copy_current_size = 0;
	int copy_reset = 0;

	int size = 0;

	int modify_allowed = 1;

	if (isInArchive()) {
		size = ReadArchiveFile(file, buffer_base, BIG_BUFFER_SIZE);
		modify_allowed = 0;
	} else {
		size = ReadFile(file, buffer_base, BIG_BUFFER_SIZE);
	}

	if (size < 0) {
		free(buffer_base);
		return size;
	}

	char *buffer = buffer_base;

	int has_utf8_bom = 0;
	char utf8_bom[3] = {0xEF, 0xBB, 0xBF};
	if (size >= 3 && memcmp(buffer_base, utf8_bom, 3) == 0) {
		buffer += 3;
		has_utf8_bom = 1;
		size -= 3;
	}

	if (size == 0) {
		size = 1;
		buffer[0] = '\n';
	}


	int base_pos = 0, rel_pos = 0;

	int *offset_list = (int *)malloc(MAX_LINES * sizeof(int));
	memset(offset_list, 0, MAX_LINES * sizeof(int));

	TextList list;
	memset(&list, 0, sizeof(TextList));

	int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
		TextListEntry *entry = malloc(sizeof(TextListEntry));
		entry->line_number = i;

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

	int edit_line = 0;
	int changed = 0;
	int save_question = 0;

	while (1) {
		readPad();

		if (!save_question) {
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

						// Update line_number
						list.head->line_number = base_pos;

						// Read
						textReadLine(buffer, offset_list[base_pos], size, list.head->line);
					}
				}
				copy_reset = 1;
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

							// Update line_number
							list.tail->line_number = base_pos + MAX_ENTRIES - 1;

							// Read
							int length = textReadLine(buffer, offset_list[base_pos + MAX_ENTRIES - 1], size, list.tail->line);
							offset_list[base_pos + MAX_ENTRIES] = offset_list[base_pos + MAX_ENTRIES - 1] + length;
						}
					}
				}
				copy_reset = 1;
			} else if(modify_allowed && !edit_line && hold_buttons & SCE_CTRL_ENTER) {
				int line_start = offset_list[base_pos + rel_pos];
				
				char line[MAX_LINE_CHARACTERS];
				textReadLine(buffer, line_start, size, line);

				initImeDialog(language_container[EDIT_LINE], line, MAX_LINE_CHARACTERS, SCE_IME_TYPE_DEFAULT, SCE_IME_OPTION_MULTILINE);

				edit_line = 1;
			}

			if (edit_line) {
				int ime_result = updateImeDialog();

				if (ime_result == IME_DIALOG_RESULT_FINISHED) {
					int line_start = offset_list[base_pos + rel_pos];
					
					char line[MAX_LINE_CHARACTERS];
					int length = textReadLine(buffer, line_start, size, line);

					// Don't count newline 
					if (buffer[line_start + length - 1] == '\n') {
						length--;
					}

					char *new_line = (char *)getImeDialogInputTextUTF8();
					int new_length = strlen(new_line);

					// Move data if size has changed
					if (new_length != length) {
						memmove(&buffer[line_start+new_length], &buffer[line_start+length], size-line_start-length);
						size = size + (new_length - length);
					}

					// Copy new line into buffer
					memcpy(&buffer[line_start], new_line, new_length);

					// Add new lines to n_lines
					int i;
					for (i = 0; i < new_length; i++) {
						if (new_line[i] == '\n') {
							n_lines++;
						}
					}
					
					// Update entries
					updateTextEntries(buffer, base_pos, size, offset_list, list.head);

					edit_line = 0;
					changed = 1;

				} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
					edit_line = 0;
				}
			}

			// Cut line
			if (modify_allowed && hold_buttons & SCE_CTRL_LEFT && copy_current_size < MAX_COPY_BUFFER_SIZE) {
				if (copy_reset) {
					copy_reset = 0;
					copy_current_size = 0;
				}

				// Get current line
				int line_start = offset_list[base_pos + rel_pos];
				char line[MAX_LINE_CHARACTERS];
				int length = textReadLine(buffer, line_start, size, line);

				// Copy line into copy_buffer
				memcpy(copy_buffer[copy_current_size].line, &buffer[line_start], length);
				copy_buffer[copy_current_size].line[length] = '\0';
					
				// Remove line
				memmove(&buffer[line_start], &buffer[line_start+length], size-line_start);	
				size -= length;
				n_lines -= 1;

				// Add empty line if resulting buffer is empty
				if (size == 0) {
					size = 1;
					n_lines = 1;
					buffer[0] = '\n';
				} 
				
				// Update entries
				updateTextEntries(buffer, base_pos, size, offset_list, list.head);

				if (base_pos + rel_pos >= n_lines) {
					rel_pos = n_lines - 1;
				}

				changed = 1;
				copy_current_size++;
			} 

			// Paste lines
			if (modify_allowed && hold_buttons & SCE_CTRL_RIGHT && copy_current_size > 0) { 
				int line_start = offset_list[base_pos + rel_pos];

				// calculated size of pasted content
				int length = 0, i;
				for (i = 0; i < copy_current_size; i++) {
					length += strlen(copy_buffer[i].line);
				}

				// Make space for pasted line
				memmove(&buffer[line_start+length], &buffer[line_start], size-line_start);
				size += length;

				// Paste the lines
				for (i = 0; i < copy_current_size; i++) {
					int line_length = strlen(copy_buffer[i].line);

					memcpy(&buffer[line_start], copy_buffer[i].line, line_length);
					line_start += line_length;
				}

				n_lines += copy_current_size;

				// Update entries
				updateTextEntries(buffer, base_pos, size, offset_list, list.head);
				
				changed = 1;
				copy_reset = 1;
			}

			// Page skip
			if (hold_buttons & SCE_CTRL_LTRIGGER || hold_buttons & SCE_CTRL_RTRIGGER) {

				if (hold_buttons & SCE_CTRL_LTRIGGER) {  // Skip page up
					base_pos = base_pos - MAX_ENTRIES;
					if (base_pos < 0) {
						base_pos = 0;
						rel_pos = 0;
					}
				} else {  // Skip page down
					base_pos = base_pos + MAX_ENTRIES;
					if (base_pos >=  n_lines - MAX_POSITION) {
						base_pos = MAX(n_lines - MAX_POSITION, 0);
						rel_pos = MIN(MAX_POSITION - 1, n_lines-1);
					}
				}

				// Update entries
				updateTextEntries(buffer, base_pos, size, offset_list, list.head);

			}

			// Cancel
			if (pressed_buttons & SCE_CTRL_CANCEL) {
				if (changed) {
					initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[SAVE_MODIFICATIONS]);
					save_question = 1;
				} else {
					hex_viewer = 0;
					break;
				}
			}

			// Switch to hex viewer
			if (pressed_buttons & SCE_CTRL_SQUARE) {
				hex_viewer = 1;
				break;
			}
		} else {
			int msg_result = updateMessageDialog();
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				SceUID fd = sceIoOpen(file, SCE_O_WRONLY|SCE_O_TRUNC, 0777);
				if (fd >= 0) {
					sceIoWrite(fd, buffer_base, has_utf8_bom ? size + sizeof(utf8_bom) : size);
					sceIoClose(fd);
				}

				break;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				break;
			}
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

			if (entry->line_number < n_lines) {
				char line_str[5];
				snprintf(line_str, 5, "%04i", entry->line_number);

				pgf_draw_text(SHELL_MARGIN_X, START_Y + (i * FONT_Y_SPACE), (rel_pos == i) ? TEXT_LINE_NUMBER_COLOR_FOCUS : TEXT_LINE_NUMBER_COLOR, FONT_SIZE, line_str);
			}

			float x = TEXT_START_X;

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

	free(buffer_base);

	if (hex_viewer)
		hexViewer(file);

	return 0;
}
