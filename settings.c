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
#include "config.h"
#include "init.h"
#include "theme.h"
#include "language.h"
#include "settings.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "utils.h"

#include "henkaku_config.h"

/*
	* HENkaku settings *
	- Enable PSN spoofing
	- Enable unsafe homebrew
	- Enable version spoofing
	- Spoofed version

	* Main *
	- Language
	- Theme
	- CPU
	- Disable auto-update

	* FTP *
	- Auto-start

	* Status bar *
	- Display battery percentage
*/

int taiReloadConfig();
int henkaku_reload_config();

static void taihenReloadConfig();
static void henkakuRestoreDefaultSettings();
static void rebootDevice();
static void shutdownDevice();
static void suspendDevice();

static int changed = 0;

static HENkakuConfig henkaku_config;

static char spoofed_version[6];

static SettingsMenuEntry *settings_menu_entries = NULL;
static int n_settings_entries = 0;

static char *select_button_options[2];

static ConfigEntry settings_entries[] = {
	{ "SELECT_BUTTON", CONFIG_TYPE_DECIMAL, (int *)&vitashell_config.select_button },
	{ "DISABLE_AUTOUPDATE", CONFIG_TYPE_BOOLEAN, (int *)&vitashell_config.disable_autoupdate },
};

SettingsMenuOption henkaku_settings[] = {
	{ HENKAKU_ENABLE_PSN_SPOOFING,		SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &henkaku_config.use_psn_spoofing },
	{ HENKAKU_ENABLE_UNSAFE_HOMEBREW,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &henkaku_config.allow_unsafe_hb },
	{ HENKAKU_ENABLE_VERSION_SPOOFING,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &henkaku_config.use_spoofed_version },
	{ HENKAKU_SPOOFED_VERSION,			SETTINGS_OPTION_TYPE_STRING, NULL, spoofed_version, sizeof(spoofed_version)-1, NULL, 0, NULL },
	{ HENKAKU_RESTORE_DEFAULT_SETTINGS,	SETTINGS_OPTION_TYPE_CALLBACK, (void *)henkakuRestoreDefaultSettings, NULL, 0, NULL, 0, NULL },
	{ HENKAKU_RELOAD_CONFIG,			SETTINGS_OPTION_TYPE_CALLBACK, (void *)taihenReloadConfig, NULL, 0, NULL, 0, NULL },
};

SettingsMenuOption main_settings[] = {
	// { VITASHELL_SETTINGS_LANGUAGE,		SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &language },
	// { VITASHELL_SETTINGS_THEME,			SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &theme },
	
	{ VITASHELL_SETTINGS_SELECT_BUTTON,		SETTINGS_OPTION_TYPE_OPTIONS, NULL, NULL, 0,
											select_button_options, sizeof(select_button_options)/sizeof(char **),
											&vitashell_config.select_button },
	{ VITASHELL_SETTINGS_NO_AUTO_UPDATE,	SETTINGS_OPTION_TYPE_BOOLEAN, NULL, NULL, 0, NULL, 0, &vitashell_config.disable_autoupdate },
};

SettingsMenuOption power_settings[] = {
	{ VITASHELL_SETTINGS_REBOOT,		SETTINGS_OPTION_TYPE_CALLBACK, (void *)rebootDevice, NULL, 0, NULL, 0, NULL },
	{ VITASHELL_SETTINGS_POWEROFF,		SETTINGS_OPTION_TYPE_CALLBACK, (void *)shutdownDevice, NULL, 0, NULL, 0, NULL },
	{ VITASHELL_SETTINGS_STANDBY,		SETTINGS_OPTION_TYPE_CALLBACK, (void *)suspendDevice, NULL, 0, NULL, 0, NULL },
};

SettingsMenuEntry molecularshell_settings_menu_entries[] = {
	{ HENKAKU_SETTINGS, henkaku_settings, sizeof(henkaku_settings)/sizeof(SettingsMenuOption) },
	{ VITASHELL_SETTINGS_MAIN, main_settings, sizeof(main_settings)/sizeof(SettingsMenuOption) },
	{ VITASHELL_SETTINGS_POWER, power_settings, sizeof(power_settings)/sizeof(SettingsMenuOption) },
};

SettingsMenuEntry vitashell_settings_menu_entries[] = {
	{ VITASHELL_SETTINGS_MAIN, main_settings, sizeof(main_settings)/sizeof(SettingsMenuOption) },
	{ VITASHELL_SETTINGS_POWER, power_settings, sizeof(power_settings)/sizeof(SettingsMenuOption) },
};

static SettingsMenu settings_menu;

void loadSettingsConfig() {
	// Load settings config file
	memset(&vitashell_config, 0, sizeof(VitaShellConfig));
	readConfig("ux0:VitaShell/settings.txt", settings_entries, sizeof(settings_entries)/sizeof(ConfigEntry));
}

void saveSettingsConfig() {
	// Save settings config file
	writeConfig("ux0:VitaShell/settings.txt", settings_entries, sizeof(settings_entries)/sizeof(ConfigEntry));
}

static void rebootDevice() {
	closeSettingsMenu();
	scePowerRequestColdReset();
}

static void shutdownDevice() {
	closeSettingsMenu();
	scePowerRequestStandby();
}

static void suspendDevice() {
	closeSettingsMenu();
	scePowerRequestSuspend();
}

static void henkakuRestoreDefaultSettings() {
	memset(&henkaku_config, 0, sizeof(HENkakuConfig));
	henkaku_config.use_psn_spoofing = 1;
	henkaku_config.use_spoofed_version = 1;
	strcpy(spoofed_version, HENKAKU_DEFAULT_VERSION_STRING);
	changed = 1;

	infoDialog(language_container[HENKAKU_RESTORE_DEFAULT_MESSAGE]);
}

static void taihenReloadConfig() {
	taiReloadConfig();
	infoDialog(language_container[HENKAKU_RELOAD_CONFIG_MESSAGE]);
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

	select_button_options[0] = language_container[VITASHELL_SETTINGS_SELECT_BUTTON_USB];
	select_button_options[1] = language_container[VITASHELL_SETTINGS_SELECT_BUTTON_FTP];
}

void openSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_OPENING;
	settings_menu.entry_sel = 0;
	settings_menu.option_sel = 0;

	if (is_molecular_shell) {
		memset(&henkaku_config, 0, sizeof(HENkakuConfig));
		int res = ReadFile(henkaku_config_path, &henkaku_config, sizeof(HENkakuConfig));

		if (res != sizeof(HENkakuConfig) || henkaku_config.magic != HENKAKU_CONFIG_MAGIC || henkaku_config.version != HENKAKU_VERSION) {
			memset(&henkaku_config, 0, sizeof(HENkakuConfig));
			henkaku_config.use_psn_spoofing = 1;
			henkaku_config.use_spoofed_version = 1;
		}

		char a = (henkaku_config.spoofed_version >> 24) & 0xF;
		char b = (henkaku_config.spoofed_version >> 20) & 0xF;
		char c = (henkaku_config.spoofed_version >> 16) & 0xF;
		char d = (henkaku_config.spoofed_version >> 12) & 0xF;

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

	changed = 0;
}

void closeSettingsMenu() {
	settings_menu.status = SETTINGS_MENU_CLOSING;

	if (changed) {
		if (is_molecular_shell) {
			if (IS_DIGIT(spoofed_version[0]) && spoofed_version[1] == '.' && IS_DIGIT(spoofed_version[2]) && IS_DIGIT(spoofed_version[3])) {
				char a = spoofed_version[0] - '0';
				char b = spoofed_version[2] - '0';
				char c = spoofed_version[3] - '0';
				char d = IS_DIGIT(spoofed_version[4]) ? spoofed_version[4] - '0' : '\0';

				henkaku_config.spoofed_version = ((a << 24) | (b << 20) | (c << 16) | (d << 12));
			} else {
				henkaku_config.spoofed_version = 0;
			}

			henkaku_config.magic = HENKAKU_CONFIG_MAGIC;
			henkaku_config.version = HENKAKU_VERSION;

			WriteFile(henkaku_config_path, &henkaku_config, sizeof(HENkakuConfig));

			henkaku_reload_config();
		}

		saveSettingsConfig();
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
			settings_menu.cur_pos -= easeOut(0.0f, settings_menu.cur_pos, 0.25f, 0.01f);
		} else {
			settings_menu.status = SETTINGS_MENU_CLOSED;
		}
	}

	// Opening settings menu
	if (settings_menu.status == SETTINGS_MENU_OPENING) {
		if (settings_menu.cur_pos < SCREEN_HEIGHT) {
			settings_menu.cur_pos += easeOut(settings_menu.cur_pos, SCREEN_HEIGHT, 0.25f, 0.01f);
		} else {
			settings_menu.status = SETTINGS_MENU_OPENED;
		}
	}

	// Draw settings menu
	vita2d_draw_texture(settings_image, 0.0f, SCREEN_HEIGHT-settings_menu.cur_pos);

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
				vita2d_draw_rectangle(SHELL_MARGIN_X, y+3.0f, MARK_WIDTH, FONT_Y_SPACE, SETTINGS_MENU_FOCUS_COLOR);

			if (options[j].type == SETTINGS_OPTION_TYPE_CALLBACK) {
				// Item
				float x = vita2d_pgf_text_width(font, FONT_SIZE, language_container[options[j].name]);
				pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, x), y, SETTINGS_MENU_ITEM_COLOR, FONT_SIZE, language_container[options[j].name]);
			} else {
				// Item
				float x = vita2d_pgf_text_width(font, FONT_SIZE, language_container[options[j].name]);
				pgf_draw_text(ALIGN_RIGHT(SCREEN_HALF_WIDTH-10.0f, x), y, SETTINGS_MENU_ITEM_COLOR, FONT_SIZE, language_container[options[j].name]);

				// Option
				switch (options[j].type) {
					case SETTINGS_OPTION_TYPE_BOOLEAN:
						pgf_draw_text(SCREEN_HALF_WIDTH+10.0f, y, SETTINGS_MENU_OPTION_COLOR, FONT_SIZE, *(options[j].value) ? language_container[ON] : language_container[OFF]);
						break;

					case SETTINGS_OPTION_TYPE_STRING:
						pgf_draw_text(SCREEN_HALF_WIDTH+10.0f, y, SETTINGS_MENU_OPTION_COLOR, FONT_SIZE, options[j].string);
						break;

					case SETTINGS_OPTION_TYPE_OPTIONS:
					{
						int value = *(options[j].value);
						pgf_draw_text(SCREEN_HALF_WIDTH+10.0f, y, SETTINGS_MENU_OPTION_COLOR, FONT_SIZE, options[j].options[value]);
						break;
					}
				}
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
		changed = 1;

		if (option->name == HENKAKU_ENABLE_UNSAFE_HOMEBREW) {
			if (*(option->value) == 0) {
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[HENKAKU_UNSAFE_HOMEBREW_MESSAGE]);
				setDialogStep(DIALOG_STEP_SETTINGS_AGREEMENT);
			} else {
				*(option->value) = !*(option->value);
			}
		} else {
			switch (option->type) {
				case SETTINGS_OPTION_TYPE_BOOLEAN:
					*(option->value) = !*(option->value);
					break;
				
				case SETTINGS_OPTION_TYPE_STRING:
					initImeDialog(language_container[option->name], option->string, option->size_string, SCE_IME_TYPE_EXTENDED_NUMBER, 0);
					setDialogStep(DIALOG_STEP_SETTINGS_STRING);
					break;
					
				case SETTINGS_OPTION_TYPE_CALLBACK:
					option->callback(&option);
					break;
					
				case SETTINGS_OPTION_TYPE_OPTIONS:
				{
					if (pressed_buttons & SCE_CTRL_LEFT) {
						if (*(option->value) > 0)
							(*(option->value))--;
						else
							*(option->value) = option->n_options-1;
					} else if (pressed_buttons & (SCE_CTRL_ENTER | SCE_CTRL_RIGHT)) {
						if (*(option->value) < option->n_options-1)
							(*(option->value))++;
						else
							*(option->value) = 0;
					}
					
					break;
				}
			}
		}
	}

	// Move
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		if (settings_menu.option_sel > 0) {
			settings_menu.option_sel--;
		} else if (settings_menu.entry_sel > 0) {
			settings_menu.entry_sel--;
			settings_menu.option_sel = settings_menu_entries[settings_menu.entry_sel].n_options-1;
		}
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		if (settings_menu.option_sel < settings_menu_entries[settings_menu.entry_sel].n_options-1) {
			settings_menu.option_sel++;
		} else if (settings_menu.entry_sel < n_settings_entries-1) {
			settings_menu.entry_sel++;
			settings_menu.option_sel = 0;
		}
	}

	// Close
	if (pressed_buttons & (SCE_CTRL_CANCEL | SCE_CTRL_START)) {
		closeSettingsMenu();
	}
}
