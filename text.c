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
#include "context_menu.h"
#include "archive.h"
#include "archiveRAR.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "theme.h"
#include "utils.h"
#include "language.h"
#include "ime_dialog.h"
#include "message_dialog.h"

float text_ctx_menu_max_width = 0.0f;

enum TextMenuEntrys {
	TEXT_MENU_ENTRY_MARK_UNMARK_ALL,
	TEXT_MENU_ENTRY_EMPTY_1,
	TEXT_MENU_ENTRY_CUT,
	TEXT_MENU_ENTRY_COPY,
	TEXT_MENU_ENTRY_PASTE,
	TEXT_MENU_ENTRY_DELETE,
	TEXT_MENU_ENTRY_INSERT_EMPTY_LINE,
	TEXT_MENU_ENTRY_EMPTY_2,
	TEXT_MENU_ENTRY_SEARCH,
	TEXT_MENU_ENTRY_EMPTY_3,
	TEXT_MENU_ENTRY_HEX_EDITOR,
};

MenuEntry text_menu_entries[] = {
	{ UNMARK_ALL, 0, CTX_VISIBILITY_INVISIBLE },
	{ -1, 0, CTX_VISIBILITY_UNUSED },
	{ CUT, 0, CTX_VISIBILITY_INVISIBLE },
	{ COPY, 0, CTX_VISIBILITY_INVISIBLE },
	{ PASTE, 0, CTX_VISIBILITY_INVISIBLE },
	{ DELETE, 0, CTX_VISIBILITY_INVISIBLE },
	{ INSERT_EMPTY_LINE, 0, CTX_VISIBILITY_INVISIBLE },
	{ -1, 0, CTX_VISIBILITY_UNUSED },
	{ SEARCH, 0, CTX_VISIBILITY_VISIBLE },
	{ -1, 0, CTX_VISIBILITY_UNUSED },
	{ OPEN_HEX_EDITOR, 0, CTX_VISIBILITY_VISIBLE },
};

typedef struct TextEditorState {
	int running;
	char *buffer;
	int size;
	int base_pos;
	int rel_pos;
	int offset_list[MAX_LINES];
	int selection_list[MAX_SELECTION];
	int n_selections;
	int n_copied_lines;
	int copy_reset;
	int modify_allowed;
	ContextMenu context_menu;
	CopyEntry copy_buffer[MAX_COPY_BUFFER_SIZE];
	TextList list;
	int changed;
	int save_question;
	int edit_line;
	char search_term[MAX_LINE_CHARACTERS];
	int search_result_offsets[MAX_SEARCH_RESULTS];
	int search_term_input;
	int n_search_results;
	int search_thid;
	int count_lines_thid;
	int hex_viewer;
	int count_lines_running;
	int n_lines;
	int search_running;
} TextEditorState;

typedef struct SearchParams {
	TextEditorState *state;
	char search_term[MAX_LINE_CHARACTERS];
} SearchParams;

typedef struct CountParams {
	TextEditorState *state;
} CountParams;

#define N_TEXT_MENU_ENTRIES (sizeof(text_menu_entries) / sizeof(MenuEntry))

void initTextContextMenuWidth() {
	int i;
	for (i = 0; i < N_TEXT_MENU_ENTRIES; i++) {
		if (text_menu_entries[i].visibility != CTX_VISIBILITY_UNUSED)
			text_ctx_menu_max_width = MAX(text_ctx_menu_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[text_menu_entries[i].name]));
	}

	text_ctx_menu_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	text_ctx_menu_max_width = MAX(text_ctx_menu_max_width, CONTEXT_MENU_MIN_WIDTH);
}

static void textListAddEntry(TextList *list, TextListEntry *entry) {
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

static void textListEmpty(TextList *list) {
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

static int textReadLine(char *buffer, int offset, int size, char *line) {
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
		if ((line_width + ch_width) >= (MAX_WIDTH - TEXT_START_X + SHELL_MARGIN_X))
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

static void updateTextEntry(TextEditorState *state, TextListEntry* entry, int rel_pos) {
	entry->line_number = state->base_pos + rel_pos;

	// Mark entry as selected
	entry->selected = 0;
	int j = 0;
	for (j = 0; j < state->n_selections; j++) {
		if (entry->line_number == state->selection_list[j]) {
			entry->selected = 1;
			break;
		}
	}

	int length = textReadLine(state->buffer, state->offset_list[state->base_pos + rel_pos], state->size, entry->line);
	state->offset_list[state->base_pos + rel_pos + 1] = state->offset_list[state->base_pos + rel_pos] + length;
}

static void updateTextEntries(TextEditorState *state) {
	TextListEntry *entry = state->list.head;
	int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
		if (!entry) {
			break;
		}
		
		updateTextEntry(state, entry, i);

		entry = entry->next;
	}
}


static CopyEntry *copy_line(TextEditorState *state, int line_number) {
	if (state->copy_reset) {
		state->copy_reset = 0;
		state->n_copied_lines = 0;
	}

	// Get current line
	int line_start = state->offset_list[line_number];
	char line[MAX_LINE_CHARACTERS];
	int length = textReadLine(state->buffer, line_start, state->size, line);

	CopyEntry *entry = &state->copy_buffer[state->n_copied_lines];

	// Copy line into copy_buffer
	memcpy(entry->line, &state->buffer[line_start], length);

	// Make sure line end with a newline
	if (entry->line[length - 1] != '\n') {
		entry->line[length] = '\n';
		length++;
	}
	
	// Terminate line
	state->copy_buffer[state->n_copied_lines].line[length] = '\0';
	
	state->n_copied_lines++;

	return entry;
}


static void delete_line(TextEditorState *state, int line_number) {
	// Get current line
	int line_start = state->offset_list[line_number];
	char line[MAX_LINE_CHARACTERS];
	int length = textReadLine(state->buffer, line_start, state->size, line);

	// Remove line
	memmove(&state->buffer[line_start], &state->buffer[line_start+length], state->size-line_start);	
	state->size -= length;
	state->n_lines -= 1;

	// Add empty line if resulting buffer is empty
	if (state->size == 0) {
		state->size = 1;
		state->n_lines = 1;
		state->buffer[0] = '\n';
	} 

	if (state->base_pos + state->rel_pos >= state->n_lines) {
		state->rel_pos = state->n_lines - state->base_pos - 1;
	}

	if (state->rel_pos < 0) {
		state->base_pos += state->rel_pos;
		state->rel_pos = 0;
	}

	state->changed = 1;
	state->n_selections = 0;
	
	// Update entries
	updateTextEntries(state);
}

static void insert_line(TextEditorState *state, char *line, int pos) {
	int offset = state->offset_list[pos];

	// calculated size of inserted line
	int length = strlen(line);

	// Make space for inserted line
	memmove(&state->buffer[offset+length], &state->buffer[offset], state->size-offset);
	state->size += length;

	// Insert the lines
	memcpy(&state->buffer[offset], line, length);

	int i;
	for (i = 0; i < length; i++) {
		if (line[i] == '\n') {
			state->n_lines++;
		}
	}
	
	state->n_selections = 0;
	state->changed = 1;
	state->copy_reset = 1;

	// Update entries
	updateTextEntries(state);
}

static void cut_line(TextEditorState *state, int line_number) {
	copy_line(state, line_number);
	delete_line(state, line_number);
}

static void paste_lines(TextEditorState *state, int pos) {
	int line_start = state->offset_list[pos];

	// calculated size of pasted content
	int length = 0, i;
	for (i = 0; i < state->n_copied_lines; i++) {
		length += strlen(state->copy_buffer[i].line);
	}

	// Make space for pasted lines
	memmove(&state->buffer[line_start+length], &state->buffer[line_start], state->size-line_start);
	state->size += length;

	// Paste the lines
	for (i = 0; i < state->n_copied_lines; i++) {
		int line_length = strlen(state->copy_buffer[i].line);

		memcpy(&state->buffer[line_start], state->copy_buffer[i].line, line_length);
		line_start += line_length;
	}

	state->n_lines += state->n_copied_lines;
	
	state->changed = 1;
	state->copy_reset = 1;
	state->n_selections = 0;

	// Update entries
	updateTextEntries(state);
}

static int cmp (const void * a, const void * b) {
   return ( *(int*)a - *(int*)b );
}

static int contextMenuEnterCallback(int pos, void *context) {
	TextEditorState *state = (TextEditorState*) context;

	// debugPrintf("ContextMenu Callback: %i\n", pos);

	switch (pos) {
		case TEXT_MENU_ENTRY_SEARCH:
			initImeDialog(language_container[ENTER_SEARCH_TERM], "", MAX_LINE_CHARACTERS, SCE_IME_TYPE_DEFAULT, 0);
			state->search_term_input = 1;
			break;

		case TEXT_MENU_ENTRY_HEX_EDITOR:
			state->hex_viewer = 1;
			if (state->changed) {
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[SAVE_MODIFICATIONS]);
				state->save_question = 1;
			} else {
				state->running = 0;
			}
			return CONTEXT_MENU_CLOSED;

		case TEXT_MENU_ENTRY_COPY:
			state->n_copied_lines = 0;

			// Sort the selection list
			qsort(state->selection_list, state->n_selections, sizeof(int), cmp);
		
			int i;
			for (i = 0; i < state->n_selections; i++) {
				copy_line(state, state->selection_list[i]);
			}

			state->n_selections = 0;
			break;

		case TEXT_MENU_ENTRY_CUT:
			state->n_copied_lines = 0;

			// Sort the selection list
			qsort(state->selection_list, state->n_selections, sizeof(int), cmp);

			// Cut the lines in reversed order to not break the offsets
			for (i = state->n_selections - 1; i >= 0; i--) {
				cut_line(state, state->selection_list[i]);
			}

			// Reverse the order of the copied lines
			int j;
			for (i = 0, j = state->n_selections - 1; i < j; i++, j--) {
				CopyEntry tmp = state->copy_buffer[i];
				state->copy_buffer[j] = state->copy_buffer[i];
				state->copy_buffer[i] = tmp;
			}

			state->n_selections = 0;
			break;

		case TEXT_MENU_ENTRY_PASTE:
			paste_lines(state, state->base_pos + state->rel_pos + 1);
			break;

		case TEXT_MENU_ENTRY_DELETE:
			delete_line(state, state->base_pos + state->rel_pos);
			break;

		case TEXT_MENU_ENTRY_INSERT_EMPTY_LINE:
			insert_line(state, "\n", state->base_pos + state->rel_pos + 1);
			break;

		case TEXT_MENU_ENTRY_MARK_UNMARK_ALL:
			state->n_selections = 0;
			updateTextEntries(state);
			break;
	}

	return CONTEXT_MENU_CLOSING;
}

static void setContextMenuVisibilities(TextEditorState *state) {
	MenuEntry *menu_entries = state->context_menu.menu_entries;

	// Cut & Copy & Unmark only visible when at least one line is selected
	menu_entries[TEXT_MENU_ENTRY_MARK_UNMARK_ALL].visibility = state->n_selections == 0 ? CTX_VISIBILITY_INVISIBLE : CTX_VISIBILITY_VISIBLE;
	menu_entries[TEXT_MENU_ENTRY_CUT].visibility = state->n_selections == 0 ? CTX_VISIBILITY_INVISIBLE : CTX_VISIBILITY_VISIBLE;
	menu_entries[TEXT_MENU_ENTRY_COPY].visibility = state->n_selections == 0 ? CTX_VISIBILITY_INVISIBLE : CTX_VISIBILITY_VISIBLE;

	// Paste only visible when at least one line is in copy buffer
	menu_entries[TEXT_MENU_ENTRY_PASTE].visibility = state->n_copied_lines == 0 ? CTX_VISIBILITY_INVISIBLE : CTX_VISIBILITY_VISIBLE;

	// Go to first entry
	int i;
	for (i = 0; i < N_TEXT_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
			setContextMenuPos(i);
			break;
		}
	}

	if (i == N_TEXT_MENU_ENTRIES)
		setContextMenuPos(-1);
}

static int count_lines_thread(SceSize args, CountParams *params) {
	TextEditorState *state = params->state;

	state->count_lines_running = 1;
	state->n_lines = 0;

	int offset = 0;

	while (state->count_lines_running && offset < state->size && state->n_lines < MAX_LINES) {
		offset += textReadLine(state->buffer, offset, state->size, NULL);
		state->n_lines++;

		sceKernelDelayThread(1000);
	}

	return sceKernelExitDeleteThread(0);
}

static int search_thread(SceSize args, SearchParams *argp) {
	TextEditorState *state = argp->state;
	char *search_term = argp->search_term;
	int search_term_length = strlen(search_term);
	int *search_result_offsets = state->search_result_offsets; 

	state->search_running = 1;
	state->n_search_results = 0;

	int offset = 0;

	// make sure buffer is null-terminated
	state->buffer[state->size] = '\0';

	char *r;
	while (state->search_running && offset < state->size && state->n_search_results < MAX_SEARCH_RESULTS) {
		r = strcasestr(state->buffer + offset, search_term);

		if (r == NULL) {
			state->search_running = 0;
			continue;
		}

		int index = r - state->buffer;

		search_result_offsets[state->n_search_results++] = index;
		offset = index + 1;

		sceKernelDelayThread(1000);
	}

	state->search_running = 0;

	return sceKernelExitDeleteThread(0);
}

int textViewer(char *file) {
	TextEditorState *s = malloc(sizeof(TextEditorState));
	if (!s) 
		return -1;

	char *buffer_base = malloc(BIG_BUFFER_SIZE);
	if (!buffer_base)
		return -1;

    s->running = 1;
	s->hex_viewer = 0; 
	s->n_copied_lines = 0;
	s->copy_reset = 0;
	s->modify_allowed = 1;
	s->offset_list[0] = 0;
	s->count_lines_running = 0;
	s->n_lines = 0;
	s->search_running = 0;
	s->edit_line = -1;

	if (isInArchive()) {
		enum FileTypes archiveType = getArchiveType();
		switch(archiveType){
			case FILE_TYPE_ZIP:
				s->size = ReadArchiveFile(file, buffer_base, BIG_BUFFER_SIZE);
				break;
			case FILE_TYPE_RAR:
				s->size = ReadArchiveRARFile(file,buffer_base,BIG_BUFFER_SIZE);
				break;
			default:
				s->size = -1;
				break;
			}
		s->modify_allowed = 0;
	} else {
		s->size = ReadFile(file, buffer_base, BIG_BUFFER_SIZE);
	}

	if (s->size < 0) {
		free(buffer_base);
		return s->size;
	}

	s->buffer = buffer_base;

	int has_utf8_bom = 0;
	char utf8_bom[3] = {0xEF, 0xBB, 0xBF};
	if (s->size >= 3 && memcmp(buffer_base, utf8_bom, 3) == 0) {
		s->buffer += 3;
		has_utf8_bom = 1;
		s->size -= 3;
	}

	if (s->size == 0) {
		s->size = 1;
		s->buffer[0] = '\n';
	}

	if (s->buffer[s->size-1] != '\n') {
		s->buffer[s->size++] = '\n';
	}

	s->base_pos = 0;
	s->rel_pos = 0;
	s->n_selections = 0;
	memset(&s->list, 0, sizeof(TextList));


	// Init context menu param
	s->context_menu.menu_entries = text_menu_entries;
	s->context_menu.n_menu_entries = N_TEXT_MENU_ENTRIES;
	s->context_menu.menu_max_width = text_ctx_menu_max_width;
	s->context_menu.context = s;
	s->context_menu.menuEnterCallback = contextMenuEnterCallback;
	//context_menu.menuMoreEnterCallback = contextMenuMoreEnterCallback;

	int i;
	for (i = 0; i < MAX_ENTRIES; i++) {
		TextListEntry *entry = malloc(sizeof(TextListEntry));
		entry->line_number = i;
		entry->selected = 0;

		int length = textReadLine(s->buffer, s->offset_list[i], s->size, entry->line);
		s->offset_list[i + 1] = s->offset_list[i] + length;
		
		textListAddEntry(&s->list, entry);
	}

	CountParams count_params;
	count_params.state = s;

	s->count_lines_thid = sceKernelCreateThread("count_lines_thread", (SceKernelThreadEntry)count_lines_thread, 0x10000100, 0x10000, 0, 0x70000, NULL);
	if (s->count_lines_thid >= 0)
		sceKernelStartThread(s->count_lines_thid, sizeof(CountParams), &count_params);

	s->edit_line = -1;
	s->changed = 0;
	s->save_question = 0;


	s->search_term_input = 0;
	s->search_thid = 0;
	s->n_search_results = 0;

	while (s->running) {
		readPad();

		if (!s->save_question) {
			if (getContextMenuMode() != CONTEXT_MENU_CLOSED) {
				contextMenuCtrl(&s->context_menu);
			} else {
				// Context menu trigger
				if (pressed_buttons & SCE_CTRL_TRIANGLE) {
					if (getContextMenuMode() == CONTEXT_MENU_CLOSED) {
						setContextMenuVisibilities(s);
						setContextMenuMode(CONTEXT_MENU_OPENING);
					}
				}

				if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
					if (s->rel_pos > 0) {
						s->rel_pos--;
					} else {
						if (s->base_pos > 0) {
							s->base_pos--;

							// Tail to head
							s->list.tail->next = s->list.head;
							s->list.head->previous = s->list.tail;
							s->list.head = s->list.tail;

							// Second last to tail
							s->list.tail = s->list.tail->previous;
							s->list.tail->next = NULL;

							// No previous
							s->list.head->previous = NULL;

							// Update line_number
							s->list.head->line_number = s->base_pos;

							// Read
							textReadLine(s->buffer, s->offset_list[s->base_pos], s->size, s->list.head->line);

							// Update the entry
							updateTextEntry(s, s->list.head, 0);
						}
					}
					s->copy_reset = 1;
				} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
					if (s->offset_list[s->rel_pos + 1] < s->size) {
						if ((s->rel_pos + 1) < MAX_POSITION) {
							if (s->base_pos + s->rel_pos < s->n_lines - 1) 
								s->rel_pos++;
						} else {
							if (s->offset_list[s->base_pos + s->rel_pos + 1] < s->size) {
								s->base_pos++;

								// Head to tail
								s->list.head->previous = s->list.tail;
								s->list.tail->next = s->list.head;
								s->list.tail = s->list.head;

								// Second first to head
								s->list.head = s->list.head->next;
								s->list.head->previous = NULL;

								// No next
								s->list.tail->next = NULL;

								// Update line_number
								s->list.tail->line_number = s->base_pos + MAX_ENTRIES - 1;

								// Read
								int length = textReadLine(s->buffer, s->offset_list[s->base_pos + MAX_ENTRIES - 1], s->size, s->list.tail->line);
								s->offset_list[s->base_pos + MAX_ENTRIES] = s->offset_list[s->base_pos + MAX_ENTRIES - 1] + length;

								// Update the entry
								updateTextEntry(s, s->list.tail, MAX_ENTRIES - 1);
							}
						}
					}
					s->copy_reset = 1;
				}

				if (s->n_search_results > 0) {

					TextListEntry *entry = s->list.head;
					
					int i;
					for (i = 0; i < s->rel_pos; i++)
						entry = entry->next;

					int entry_start_offset = s->offset_list[entry->line_number];
					int entry_end_offset = s->offset_list[entry->line_number + 1]; 

					int target_offset = 0;

					// Skip to next search result
					if (pressed_buttons & SCE_CTRL_RTRIGGER) {
						for (i = 0; i < s->n_search_results; i++) {
							if (s->search_result_offsets[i] > entry_end_offset) {
								target_offset = s->search_result_offsets[i] - entry_start_offset;
								break;
							}
						}
					} // Skip to next last result
					else if (pressed_buttons & SCE_CTRL_LTRIGGER) {
						for (i = s->n_search_results - 1; i >= 0; i--) {
							if (s->search_result_offsets[i] < entry_start_offset) {
								target_offset = s->search_result_offsets[i] - entry_start_offset;
								break;
							}
						}
					}

					if (target_offset != 0) {
						int dir = target_offset > 0 ? 1 : -1;
						int line = s->base_pos + s->rel_pos;
						int offset = s->offset_list[line];

						while (offset < s->size && offset >= 0 && target_offset != 0) {
							offset += dir;
							target_offset -= dir;

							if (s->buffer[offset] == '\n') {
								line += dir;
								if (dir > 0) {
									s->offset_list[line] = offset + 1;
								} else {
									s->offset_list[line+1] = offset + 1;
								}
							}
						}

						if (target_offset == 0) {
							s->base_pos = line;
							s->rel_pos = 0;

							updateTextEntries(s);
						}
					}
				} else {
					// Page skip
					if (hold_buttons & SCE_CTRL_LTRIGGER || hold_buttons & SCE_CTRL_RTRIGGER) {

						if (hold_buttons & SCE_CTRL_LTRIGGER) {  // Skip page up
							s->base_pos = s->base_pos - MAX_ENTRIES;
							if (s->base_pos < 0) {
								s->base_pos = 0;
								s->rel_pos = 0;
							}
						} else {  // Skip page down
							s->base_pos = s->base_pos + MAX_ENTRIES;
							if (s->base_pos >=  s->n_lines - MAX_POSITION) {
								s->base_pos = MAX(s->n_lines - MAX_POSITION, 0);
								s->rel_pos = MIN(MAX_POSITION - 1, s->n_lines-1);
							}
						}

						// Update entries
						updateTextEntries(s);
					}
				}

				if (s->search_term_input) {
					int ime_result = updateImeDialog();

					if (ime_result == IME_DIALOG_RESULT_FINISHED) {
						char *search_term = (char *)getImeDialogInputTextUTF8();

						int length = strlen(search_term);

						if (length >= MIN_SEARCH_TERM_LENGTH) {

							// kill old search if it is already running
							if (s->search_running) {
								s->search_running = 0;
								sceKernelWaitThreadEnd(s->search_thid, NULL, NULL);
							}
							
							SearchParams search_params;
							search_params.state = s;
							strcpy(search_params.search_term, search_term);

							strcpy(s->search_term, search_term);

							s->search_thid = sceKernelCreateThread("search_thread", (SceKernelThreadEntry)search_thread, 0x10000100, 0x10000, 0, 0x70000, NULL);
							if (s->search_thid >= 0)
								sceKernelStartThread(s->search_thid, sizeof(SearchParams), &search_params);
						}

						s->search_term_input = 0;

					} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
						s->search_term_input = 0;
					}
				}
			
				// buffer modifying actions
				if (s->modify_allowed && !s->search_running) {
					if(s->edit_line <= 0 && pressed_buttons & SCE_CTRL_ENTER) {
						int line_start = s->offset_list[s->base_pos + s->rel_pos];
						
						char line[MAX_LINE_CHARACTERS];
						textReadLine(s->buffer, line_start, s->size, line);

						initImeDialog(language_container[EDIT_LINE], line, MAX_LINE_CHARACTERS, SCE_IME_TYPE_DEFAULT, SCE_IME_OPTION_MULTILINE);

						s->edit_line = s->base_pos + s->rel_pos;
					}

					if (s->edit_line >= 0) {
						int ime_result = updateImeDialog();

						if (ime_result == IME_DIALOG_RESULT_FINISHED) {
							int line_start = s->offset_list[s->edit_line];
							
							char line[MAX_LINE_CHARACTERS];
							int length = textReadLine(s->buffer, line_start, s->size, line);

							// Don't count newline 
							if (s->buffer[line_start + length - 1] == '\n') {
								length--;
							}

							char *new_line = (char *)getImeDialogInputTextUTF8();
							int new_length = strlen(new_line);

							// Move data if size has changed
							if (new_length != length) {
								memmove(&s->buffer[line_start+new_length], &s->buffer[line_start+length], s->size-line_start-length);
								s->size += (new_length - length);
							}

							// Copy new line into buffer
							memcpy(&s->buffer[line_start], new_line, new_length);

							// Add new lines to n_lines
							int i;
							for (i = 0; i < new_length; i++) {
								if (new_line[i] == '\n') {
									s->n_lines++;
								}
							}
							
							// Update entries
							updateTextEntries(s);

							s->edit_line = -1;
							s->changed = 1;

						} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
							s->edit_line = -1;
						}
					}

					// Delete line
					if (pressed_buttons & SCE_CTRL_LEFT && s->n_copied_lines < MAX_COPY_BUFFER_SIZE) {
						delete_line(s, s->base_pos + s->rel_pos);
					} 

					// Insert new line
					if (pressed_buttons & SCE_CTRL_RIGHT) {
						insert_line(s, "\n", s->base_pos + s->rel_pos + 1);
					}
				}

				// Cancel
				if (pressed_buttons & SCE_CTRL_CANCEL) {
					if (s->n_search_results) {
						s->n_search_results = 0;
					} else {
						if (s->changed) {
							initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[SAVE_MODIFICATIONS]);
							s->save_question = 1;
						} else {
							s->hex_viewer = 0;
							break;
						}
					}
				}

				// (De-)select current line
				if (pressed_buttons & SCE_CTRL_SQUARE) {
					int cur_line = s->base_pos + s->rel_pos;
					int line_selected = 1;

					int i;
					for (i = 0; i < s->n_selections; i++) {
						if (s->selection_list[i] == cur_line) {
							line_selected = 0;

							// Remove current line from selections
							s->selection_list[i] = s->selection_list[--s->n_selections];

							break;
						}
					}

					if (line_selected) {
						// Add current line to selections
						s->selection_list[s->n_selections++] = cur_line;
					}

					TextListEntry *entry = s->list.head;
					for (i = 0; i < s->rel_pos; i++) 
						entry = entry->next;

					entry->selected = line_selected;
				}
			}
		} else {
			int msg_result = updateMessageDialog();
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				SceUID fd = sceIoOpen(file, SCE_O_WRONLY|SCE_O_TRUNC, 0777);
				if (fd >= 0) {
					sceIoWrite(fd, buffer_base, has_utf8_bom ? s->size + sizeof(utf8_bom) : s->size);
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
		drawScrollBar(s->base_pos, s->n_lines);

		// Text
		TextListEntry *entry = s->list.head;

		int i;
		for (i = 0; i < s->list.length; i++) {
			char *line = entry->line;
			int line_lenght = strlen(line);

			int search_result_on_line = 0;

			int entry_start_offset = s->offset_list[entry->line_number];
			int entry_end_offset = entry_start_offset + line_lenght; 

			if (s->n_search_results > 0) {
				int j; 
				for (j = 0; j < s->n_search_results; j++) {
					int search_offset = s->search_result_offsets[j];
					if (entry_start_offset <= search_offset && entry_end_offset >= search_offset) {
						search_result_on_line = 1;
					}
				}
			}

			if (entry->line_number < s->n_lines) {
				char line_str[5];
				snprintf(line_str, 5, "%04i", entry->line_number);

				int color = (s->rel_pos == i) ? TEXT_LINE_NUMBER_COLOR_FOCUS : TEXT_LINE_NUMBER_COLOR;
				pgf_draw_text(SHELL_MARGIN_X, START_Y + (i * FONT_Y_SPACE), color, FONT_SIZE, line_str);
			}

			float x = TEXT_START_X;

			if (entry->selected) {
				vita2d_draw_rectangle(x, START_Y + (i * FONT_Y_SPACE) + 3.0f, MAX_WIDTH - TEXT_START_X + SHELL_MARGIN_X, FONT_Y_SPACE, MARKED_COLOR);
			}
 
			while (*line) {

				char *p = strchr(line, '\t');
				if (p)
					*p = '\0';

				char *search_highlight = NULL;
				if (search_result_on_line) {
					search_highlight = strcasestr(line, s->search_term);
				}

				char tmp = '\0';
				if (search_highlight) {
					tmp = *search_highlight;
					*search_highlight = '\0';
				}

				int width = pgf_draw_text(x, START_Y + (i * FONT_Y_SPACE), (s->rel_pos == i) ? TEXT_FOCUS_COLOR : TEXT_COLOR, FONT_SIZE, line);
				line += strlen(line);


				if (p) {
					*p = '\t';
					x += width + TAB_SIZE * font_size_cache[' '];
					line++;
				}

				if (search_highlight) {
					*search_highlight = tmp;

					int search_term_length = strlen(s->search_term);
					tmp = search_highlight[search_term_length];
					search_highlight[search_term_length] = '\0';

					x += width;
					x += pgf_draw_text(x, START_Y + (i * FONT_Y_SPACE), TEXT_HIGHLIGHT_COLOR, FONT_SIZE, line);
					
					search_highlight[search_term_length] = tmp;
					line += strlen(s->search_term); 
				}
			}


			entry = entry->next;
		} 

		// Draw context menu
		drawContextMenu(&s->context_menu);

		// End drawing
		endDrawing();
	}

	s->count_lines_running = 0;
	sceKernelWaitThreadEnd(s->count_lines_thid, NULL, NULL);

	if (s->search_running) {
		s->search_running = 0;
		sceKernelWaitThreadEnd(s->search_thid, NULL, NULL);
	}

	textListEmpty(&s->list);

	int hex_viewer = s->hex_viewer;

	free(s);

	free(buffer_base); 

	if (hex_viewer)
		hexViewer(file);

	return 0;
}
