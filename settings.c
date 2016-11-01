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
#include "message_dialog.h"
#include "ime_dialog.h"
#include "utils.h"

#include "henkaku_config.h"

/*
	* HENkaku configuration *
	- Enable PSN spoofing
	- Enable unsafe homebrew
	- Enable version spoofing
	- Spoofed version

	* Main *
	- CPU
	- Language
	- Theme

	* FTP *
	- Auto-start

	* Status bar *
	- Display battery percentage
*/

static HENkakuConfig henkaku_config;

static char spoofed_version[8];

// Dummy
int language, theme;

static SettingsMenuEntry *settings_menu_entries = NULL;
static int n_settings_entries = 0;

SettingsMenuOption henkaku_configuration[] = {
	{ HENKAKU_ENABLE_PSN_SPOOFING,		SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.use_psn_spoofing },
	{ HENKAKU_ENABLE_UNSAFE_HOMEBREW,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.allow_unsafe_hb },
	{ HENKAKU_ENABLE_VERSION_SPOOFING,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &henkaku_config.use_spoofed_version },
	{ HENKAKU_SPOOFED_VERSION,			SETTINGS_OPTION_TYPE_STRING, spoofed_version, 5, NULL },
};

SettingsMenuOption vitashell_main[] = {
	{ VITASHELL_SETTINGS_LANGUAGE,		SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &language },
	{ VITASHELL_SETTINGS_THEME,			SETTINGS_OPTION_TYPE_BOOLEAN, NULL, 0, &theme },
};

SettingsMenuEntry molecularshell_settings_menu_entries[] = {
	{ HENKAKU_CONFIGURATION, henkaku_configuration, sizeof(henkaku_configuration) / sizeof(SettingsMenuOption) },
	{ VITASHELL_SETTINGS_MAIN, vitashell_main, sizeof(vitashell_main) / sizeof(SettingsMenuOption) },
};

SettingsMenuEntry vitashell_settings_menu_entries[] = {
	{ VITASHELL_SETTINGS_MAIN, vitashell_main, sizeof(vitashell_main) / sizeof(SettingsMenuOption) },
};

static SettingsMenu settings_menu;

static float easeOut(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.01f) ? (dx * a) : dx;
}

void initSettingsMenu() {
	memset(&settings_menu, 0, sizeof(SettingsMenu));
	settings_menu.status = SETTINGS_MENU_CLOSED;

	if (is_molecular_shell) {
		n_settings_entries = sizeof(molecularshell_settings_menu_entries) / sizeof(SettingsMenuEntry);
		settings_menu_entries = molecularshell_settings_menu_entries;
	} else {
		n_settings_entries = sizeof(vitashell_settings_menu_entries) / sizeof(SettingsMenuEntry);
		settings_menu_entries = vitashell_settings_menu_entries;
	}

	int i;
	for (i = 0; i < n_settings_entries; i++)
		settings_menu.n_options += settings_menu_entries[i].n_options;
}

void openSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_OPENING;
	settings_menu.entry_sel = 0;
	settings_menu.option_sel = 0;

	if (is_molecular_shell) {
		ReadFile(henkaku_config_path, &henkaku_config, sizeof(HENkakuConfig));

		if (henkaku_config.magic != HENKAKU_CONFIG_MAGIC) {
			memset(&henkaku_config, 0, sizeof(HENkakuConfig));
		}

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
			strcpy(spoofed_version, HENKAKU_DEFAULT_VERSION_STRING);
		}
	}
}

void closeSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_CLOSING;

	if (is_molecular_shell) {
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

		WriteFile(henkaku_config_path, &henkaku_config, sizeof(HENkakuConfig));
	}
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
	for (i = 0; i < n_settings_entries; i++) {
		// Title
		float x = vita2d_pgf_text_width(font, FONT_SIZE, language_container[settings_menu_entries[i].name]);
		pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, x), y, SETTINGS_MENU_TITLE_COLOR, FONT_SIZE, language_container[settings_menu_entries[i].name]);

		y += FONT_Y_SPACE;

		SettingsMenuOption *options = settings_menu_entries[i].options;

		int j;
		for (j = 0; j < settings_menu_entries[i].n_options; j++) {
			// Focus
			if (settings_menu.entry_sel == i && settings_menu.option_sel == j)
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

static int agreement = SETTINGS_AGREEMENT_NONE;

void settingsAgree() {
	agreement = SETTINGS_AGREEMENT_AGREE;
}

void settingsDisagree() {
	agreement = SETTINGS_AGREEMENT_DISAGREE;
}

void settingsMenuCtrl() {
	SettingsMenuOption *option = &settings_menu_entries[settings_menu.entry_sel].options[settings_menu.option_sel];

	// Agreement
	if (agreement != SETTINGS_AGREEMENT_NONE) {
		agreement = SETTINGS_AGREEMENT_NONE;

		if (option->name == HENKAKU_ENABLE_UNSAFE_HOMEBREW) {
			*(option->value) = !*(option->value);
		}
	}

	// Change options
	if (pressed_buttons & (SCE_CTRL_ENTER | SCE_CTRL_LEFT | SCE_CTRL_RIGHT)) {
		if (option->name == HENKAKU_ENABLE_UNSAFE_HOMEBREW) {
			if (*(option->value) == 0) {
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[HENKAKU_UNSAFE_HOMEBREW_MESSAGE]);
				dialog_step = DIALOG_STEP_SETTINGS_AGREEMENT;
			} else {
				*(option->value) = !*(option->value);
			}
		} else {
			if (option->type == SETTINGS_OPTION_TYPE_BOOLEAN) {
				*(option->value) = !*(option->value);
			} else if (option->type == SETTINGS_OPTION_TYPE_STRING) {
				initImeDialog(language_container[option->name], option->string, option->size_string, SCE_IME_TYPE_EXTENDED_NUMBER, 0);
				dialog_step = DIALOG_STEP_SETTINGS_STRING;
			}
		}
	}

	// Move
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		if (settings_menu.option_sel > 0) {
			settings_menu.option_sel--;
		} else if (settings_menu.entry_sel > 0) {
			settings_menu.entry_sel--;
			settings_menu.option_sel = settings_menu_entries[settings_menu.entry_sel].n_options - 1;
		}
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		if (settings_menu.option_sel < settings_menu_entries[settings_menu.entry_sel].n_options - 1) {
			settings_menu.option_sel++;
		} else if (settings_menu.entry_sel < n_settings_entries - 1) {
			settings_menu.entry_sel++;
			settings_menu.option_sel = 0;
		}
	}

	// Close
	if (pressed_buttons & (SCE_CTRL_CANCEL | SCE_CTRL_START)) {
		closeSettingsMenu();
	}
}
