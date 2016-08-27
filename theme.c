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
#include "config.h"
#include "theme.h"

extern unsigned char _binary_resources_colors_txt_start;
extern unsigned char _binary_resources_colors_txt_size;

int BACKGROUND_COLOR;
int GENERAL_COLOR;
int TITLE_COLOR;
int PATH_COLOR;
int DATE_TIME_COLOR;
int FOCUS_COLOR;
int FOLDER_COLOR;
int IMAGE_COLOR;
int ARCHIVE_COLOR;
int SCROLL_BAR_COLOR;
int SCROLL_BAR_BG_COLOR;
int MARKED_COLOR;
int INVISIBLE_COLOR;
int DIALOG_BG_COLOR;
int CONTEXT_MENU_COLOR;
int PROGRESS_BAR_COLOR;
int PROGRESS_BAR_BG_COLOR;
int HEX_OFFSET_COLOR;
int HEX_NIBBLE_COLOR;

vita2d_texture *bg_tex = NULL;

void loadTheme() {
	#define COLOR_ENTRY(name) { #name, CONFIG_TYPE_HEXDECIMAL, (void *)&name }
	ConfigEntry colors_entries[] = {
		COLOR_ENTRY(BACKGROUND_COLOR),
		COLOR_ENTRY(GENERAL_COLOR),
		COLOR_ENTRY(TITLE_COLOR),
		COLOR_ENTRY(PATH_COLOR),
		COLOR_ENTRY(DATE_TIME_COLOR),
		COLOR_ENTRY(FOCUS_COLOR),
		COLOR_ENTRY(FOLDER_COLOR),
		COLOR_ENTRY(IMAGE_COLOR),
		COLOR_ENTRY(ARCHIVE_COLOR),
		COLOR_ENTRY(SCROLL_BAR_COLOR),
		COLOR_ENTRY(SCROLL_BAR_BG_COLOR),
		COLOR_ENTRY(MARKED_COLOR),
		COLOR_ENTRY(INVISIBLE_COLOR),
		COLOR_ENTRY(DIALOG_BG_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_BG_COLOR),
		COLOR_ENTRY(HEX_OFFSET_COLOR),
		COLOR_ENTRY(HEX_NIBBLE_COLOR),
	};

	int loaded = -1;

	if (use_custom_config)
		loaded = readConfig("ux0:VitaShell/theme/colors.txt", colors_entries, sizeof(colors_entries) / sizeof(ConfigEntry));

	if (loaded < 0)
		readConfigBuffer(&_binary_resources_colors_txt_start, (int)&_binary_resources_colors_txt_size, colors_entries, sizeof(colors_entries) / sizeof(ConfigEntry));

	if (use_custom_config) {
		if (!bg_tex)
			bg_tex = vita2d_load_PNG_file("ux0:VitaShell/theme/wallpaper.png");
	}
}