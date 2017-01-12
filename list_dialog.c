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
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "list_dialog.h"

static int is_open = LIST_DIALOG_CLOSE;
static List* current_list = NULL;
static ListDialog list_dialog;
static int max_entry_show = 0;
static int base_pos = 0;
static int rel_pos = 0;

int getListDialogMode()
{
	return is_open;
}

void loadListDialog(List* list) {
	base_pos = 0;
	rel_pos = 0;
	current_list = list;

	list_dialog.height = 0.0f;
	list_dialog.width = 0.0f;

	// Calculate max width
	int z;
	float width;
	for (z = 0;z < current_list->n_list_entries; z++)
	{
		width = vita2d_pgf_text_width(font, FONT_SIZE, language_container[current_list->list_entries[z].name]);
		if (width > list_dialog.width)
				list_dialog.width = width;
	}

	width = vita2d_pgf_text_width(font, FONT_SIZE, language_container[current_list->title]);
	if (width > list_dialog.width)
			list_dialog.width = width;

	// Calculate height
	int list_calc_height = current_list->n_list_entries;
	if (list_calc_height > LIST_MAX_ENTRY)
		list_calc_height = LIST_MAX_ENTRY;

	max_entry_show = list_calc_height;

	list_dialog.height = (SHELL_MARGIN_Y * 3) + (list_calc_height * SHELL_MARGIN_Y);

	// Margin
	list_dialog.width += 2.0f * SHELL_MARGIN_X;
	list_dialog.height += 4.0f * SHELL_MARGIN_Y;

	list_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, list_dialog.width);
	list_dialog.y = ALIGN_CENTER(SCREEN_HEIGHT, list_dialog.height) - 2.0f * FONT_Y_SPACE;
	list_dialog.scale = 0.0f;
	list_dialog.animation_mode = LIST_DIALOG_OPENING;
	list_dialog.status = SCE_COMMON_DIALOG_STATUS_RUNNING;
	list_dialog.enter_pressed = 0;

	is_open = LIST_DIALOG_OPEN;
}

float easeOut3(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.01f) ? (dx * a) : dx;
}

void drawListDialog() {
	if (is_open == LIST_DIALOG_CLOSE)
		return;

	// Dialog background
	vita2d_draw_texture_scale_rotate_hotspot(dialog_image, list_dialog.x + list_dialog.width / 2.0f,
														list_dialog.y + list_dialog.height / 2.0f,
														list_dialog.scale * (list_dialog.width / vita2d_texture_get_width(dialog_image)),
														list_dialog.scale * (list_dialog.height / vita2d_texture_get_height(dialog_image)),
														0.0f, vita2d_texture_get_width(dialog_image) / 2.0f, vita2d_texture_get_height(dialog_image) / 2.0f);

	// Easing out
	if (list_dialog.animation_mode == LIST_DIALOG_CLOSING) {
		if (list_dialog.scale > 0.0f) {
			list_dialog.scale -= easeOut3(0.0f, list_dialog.scale, 0.25f);
		} else {
			list_dialog.animation_mode = LIST_DIALOG_CLOSED;
			list_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
		}
	}

	if (list_dialog.animation_mode == LIST_DIALOG_OPENING) {
		if (list_dialog.scale < 1.0f) {
			list_dialog.scale += easeOut3(list_dialog.scale, 1.0f, 0.25f);
		} else {
			list_dialog.animation_mode = LIST_DIALOG_OPENED;
		}
	}

	if (list_dialog.animation_mode == LIST_DIALOG_OPENED) {
		pgf_draw_text(list_dialog.x + ALIGN_CENTER(list_dialog.width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[current_list->title])), list_dialog.y + FONT_Y_SPACE, TEXT_COLOR, FONT_SIZE, language_container[current_list->title]);

		int z;
		for (z = 0;z < LIST_MAX_ENTRY && (base_pos + z) < current_list->n_list_entries; z++) {
			uint32_t color = (rel_pos == z) ? TEXT_FOCUS_COLOR : TEXT_COLOR;
			pgf_draw_text(list_dialog.x + ALIGN_CENTER(list_dialog.width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[current_list->list_entries[(z + base_pos)].name])), list_dialog.y + (FONT_Y_SPACE * 3) + (FONT_Y_SPACE * z), color, FONT_SIZE, language_container[current_list->list_entries[(z + base_pos)].name]);
		}

		int max_base_pos = current_list->n_list_entries - LIST_MAX_ENTRY;
		if (base_pos < max_base_pos  && current_list->n_list_entries > LIST_MAX_ENTRY) {
			pgf_draw_text(list_dialog.x + ALIGN_CENTER(list_dialog.width, vita2d_pgf_text_width(font, FONT_SIZE, DOWN_ARROW)), list_dialog.y + (FONT_Y_SPACE * 3) + (FONT_Y_SPACE * max_entry_show), TEXT_COLOR, FONT_SIZE, DOWN_ARROW);
		}

		if (base_pos > 0 && current_list->n_list_entries > LIST_MAX_ENTRY) {
			pgf_draw_text(list_dialog.x + ALIGN_CENTER(list_dialog.width, vita2d_pgf_text_width(font, FONT_SIZE, UP_ARROW)), list_dialog.y + (FONT_Y_SPACE * 2), TEXT_COLOR, FONT_SIZE, UP_ARROW);
		}
	} else if (list_dialog.animation_mode == LIST_DIALOG_CLOSED) {
		if (list_dialog.enter_pressed) {
			if (current_list->listSelectCallback)
				current_list->listSelectCallback(base_pos + rel_pos);
		}

		is_open = LIST_DIALOG_CLOSE;
	}
}

void listDialogCtrl() {
	if (is_open == LIST_DIALOG_CLOSE)
		return;

	if ((pressed_buttons & (SCE_CTRL_CANCEL | SCE_CTRL_START)) && current_list->can_escape)
		list_dialog.animation_mode = LIST_DIALOG_CLOSING;

	if (pressed_buttons & SCE_CTRL_ENTER) {
		list_dialog.enter_pressed = 1;
		list_dialog.animation_mode = LIST_DIALOG_CLOSING;
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
		if ((rel_pos + 1) < current_list->n_list_entries) {
			if ((rel_pos + 1) < LIST_MAX_ENTRY) {
				rel_pos++;
			} else {
				if ((base_pos + rel_pos + 1) < current_list->n_list_entries) {
					base_pos++;
				}
			}
		}
	}		
}
