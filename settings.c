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
#include "init.h"
#include "theme.h"
#include "language.h"
#include "settings.h"
#include "ime_dialog.h"
#include "utils.h"

#include "henkaku_config.h"

#define DEFAULT_VERSION_STRING "3.61"

HENkakuConfig henkaku_config;

char spoofed_version[8];

typedef struct {
	int status;
	float cur_pos;
	int sel;
	int n_options;
} SettingsMenu;

typedef struct {
	int name;
	int type;
	char *string;
	int size_string;
	int *value;
} SettingsMenuOption;

typedef struct {
	int name;
	SettingsMenuOption *options;
	int n_options;
} SettingsMenuEntry;

SettingsMenuOption henkaku_configuration[] = {
	{ HENKAKU_ENABLE_PSN_SPOOFING,		SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.use_psn_spoofing },
	{ HENKAKU_ENABLE_UNSAFE_HOMEBREW,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.allow_unsafe_hb },
	{ HENKAKU_ENABLE_VERSION_SPOOFING,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.use_spoofed_version },
	{ HENKAKU_SPOOFED_VERSION,			SETTINGS_OPTION_TYPE_STRING, spoofed_version, sizeof(spoofed_version), NULL },
};

SettingsMenuEntry settings_menu_entries[] = {
	{ HENKAKU_CONFIGURATION, henkaku_configuration, sizeof(henkaku_configuration) / sizeof(SettingsMenuOption) },
};

#define N_SETTINGS_ENTRIES (sizeof(settings_menu_entries) / sizeof(SettingsMenuEntry))

SettingsMenu settings_menu;

static float easeOut(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.01f) ? (dx * a) : dx;
}

void initSettingsMenu() {
	memset(&settings_menu, 0, sizeof(SettingsMenu));
	settings_menu.status = SETTINGS_MENU_CLOSED;

	int i;
	for (i = 0; i < N_SETTINGS_ENTRIES; i++)
		settings_menu.n_options += settings_menu_entries[i].n_options;
}

void openSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_OPENING;
	settings_menu.sel = 0;

	ReadFile("savedata0:config.bin", &henkaku_config, sizeof(HENkakuConfig));

	char a = (henkaku_config.spoofed_version >> 28) & 0xF;
	char b = (henkaku_config.spoofed_version >> 24) & 0xF;
	char c = (henkaku_config.spoofed_version >> 20) & 0xF;
	char d = (henkaku_config.spoofed_version >> 16) & 0xF;

	memset(spoofed_version, 0, sizeof(spoofed_version));

	if (a || b || c || d) {
		spoofed_version[0] = '0' + a;
		spoofed_version[1] = '.';
		spoofed_version[2] = '0' + b;
		spoofed_version[3] = '0' + c;
		spoofed_version[4] = '\0';

		if (d) {
			spoofed_version[4] = '0' + d;
			spoofed_version[5] = '\0';
		}
	} else {
		strcpy(spoofed_version, DEFAULT_VERSION_STRING);
	}
}

void closeSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_CLOSING;

	if (IS_DIGIT(spoofed_version[0]) && spoofed_version[1] == '.' && IS_DIGIT(spoofed_version[2]) && IS_DIGIT(spoofed_version[3])) {
		char a = spoofed_version[0] - '0';
		char b = spoofed_version[2] - '0';
		char c = spoofed_version[3] - '0';
		char d = IS_DIGIT(spoofed_version[4]) ? spoofed_version[4] - '0' : '\0';

		henkaku_config.spoofed_version = ((a << 28) | (b << 24) | (c << 20) | (d << 16));
	} else {
		henkaku_config.spoofed_version = 0;
	}

	henkaku_config.magic = HENKAKU_CONFIG_MAGIC;
	henkaku_config.version = HENKAKU_VERSION;
	WriteFile("savedata0:config.bin", &henkaku_config, sizeof(HENkakuConfig));
}

int getSettingsMenuStatus() {
	return settings_menu.status;
}

void drawSettingsMenu() {
	if (settings_menu.status == SETTINGS_MENU_CLOSED)
		return;

	// Closing settings menu
	if (settings_menu.status == SETTINGS_MENU_CLOSING) {
		if (settings_menu.cur_pos > 0.0f) {
			settings_menu.cur_pos -= easeOut(0.0f, settings_menu.cur_pos, 0.25f);
		} else {
			settings_menu.status = SETTINGS_MENU_CLOSED;
		}
	}

	// Opening settings menu
	if (settings_menu.status == SETTINGS_MENU_OPENING) {
		if (settings_menu.cur_pos < SCREEN_HEIGHT) {
			settings_menu.cur_pos += easeOut(settings_menu.cur_pos, SCREEN_HEIGHT, 0.25f);
		} else {
			settings_menu.status = SETTINGS_MENU_OPENED;
		}
	}

	// Draw settings menu
	vita2d_draw_texture(settings_image, 0.0f, SCREEN_HEIGHT - settings_menu.cur_pos);

	float y = SCREEN_HEIGHT - settings_menu.cur_pos + START_Y;

	int i;
	for (i = 0; i < N_SETTINGS_ENTRIES; i++) {
		// Title
		float x = vita2d_pgf_text_width(font, FONT_SIZE, language_container[settings_menu_entries[i].name]);
		pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, x), y, SETTINGS_MENU_TITLE_COLOR, FONT_SIZE, language_container[settings_menu_entries[i].name]);

		y += FONT_Y_SPACE;

		SettingsMenuOption *options = settings_menu_entries[i].options;

		int j;
		for (j = 0; j < settings_menu_entries[i].n_options; j++) {
			// Focus
			if (settings_menu.sel == j)
				vita2d_draw_rectangle(SHELL_MARGIN_X, y + 3.0f, MARK_WIDTH, FONT_Y_SPACE, SETTINGS_MENU_FOCUS_COLOR);

			// Item
			float x = vita2d_pgf_text_width(font, FONT_SIZE, language_container[options[j].name]);
			pgf_draw_text(ALIGN_RIGHT(SCREEN_HALF_WIDTH - 10.0f, x), y, SETTINGS_MENU_ITEM_COLOR, FONT_SIZE, language_container[options[j].name]);

			// Option
			if (options[j].type == SETTINGS_OPTION_TYPE_BOOLEAN) {
				pgf_draw_text(SCREEN_HALF_WIDTH + 10.0f, y, SETTINGS_MENU_OPTION_COLOR, FONT_SIZE, *(options[j].value) ? language_container[ON] : language_container[OFF]);
			} else if (options[j].type == SETTINGS_OPTION_TYPE_STRING) {
				pgf_draw_text(SCREEN_HALF_WIDTH + 10.0f, y, SETTINGS_MENU_OPTION_COLOR, FONT_SIZE, options[j].string);
			}

			y += FONT_Y_SPACE;
		}

		y += FONT_Y_SPACE;
	}
}

void settingsMenuCtrl() {
	// Close
	if (pressed_buttons & (SCE_CTRL_CANCEL | SCE_CTRL_START)) {
		closeSettingsMenu();
	}

	// Move
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		if (settings_menu.sel > 0)
			settings_menu.sel--;
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		if (settings_menu.sel < settings_menu.n_options - 1)
			settings_menu.sel++;
	}

	// TODO                                             |
	SettingsMenuOption *option = &settings_menu_entries[0].options[settings_menu.sel];

	// Change options
	if (pressed_buttons & (SCE_CTRL_ENTER | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
		if (option->type == SETTINGS_OPTION_TYPE_BOOLEAN) {	
			*(option->value) = !*(option->value);
		} else if (option->type == SETTINGS_OPTION_TYPE_STRING) {
			initImeDialog(language_container[option->name], option->string, option->size_string, SCE_IME_TYPE_EXTENDED_NUMBER, 0);
			dialog_step = DIALOG_STEP_SETTINGS_STRING;
		}
	}
}
