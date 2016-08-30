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
#include "file.h"
#include "config.h"
#include "theme.h"

extern unsigned char _binary_resources_ftp_png_start;
extern unsigned char _binary_resources_dialog_png_start;
extern unsigned char _binary_resources_context_png_start;
extern unsigned char _binary_resources_battery_png_start;
extern unsigned char _binary_resources_battery_bar_red_png_start;
extern unsigned char _binary_resources_battery_bar_green_png_start;

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

vita2d_texture *ftp_image = NULL, *dialog_image = NULL, *context_image = NULL, *battery_image = NULL, *battery_bar_red_image = NULL, *battery_bar_green_image = NULL;

vita2d_texture *wallpaper_image[MAX_WALLPAPERS];

vita2d_texture *previous_wallpaper_image = NULL, *current_wallpaper_image = NULL;

int wallpaper_count = 0;

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

	// Load default config file
	readConfigBuffer(&_binary_resources_colors_txt_start, (int)&_binary_resources_colors_txt_size, colors_entries, sizeof(colors_entries) / sizeof(ConfigEntry));

	// Load custom config file
	if (use_custom_config) {
		char path[MAX_PATH_LENGTH];

		char *theme_name = NULL;
		ConfigEntry theme_entries[] = {
			{ "THEME_NAME", CONFIG_TYPE_STRING, (void *)&theme_name },
		};

		// Load theme config
		readConfig("ux0:VitaShell/theme/theme.txt", theme_entries, sizeof(theme_entries) / sizeof(ConfigEntry));

		if (theme_name) {
			// Load colors config
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/colors.txt", theme_name);
			readConfig(path, colors_entries, sizeof(colors_entries) / sizeof(ConfigEntry));

			// Load pngs
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/ftp.png", theme_name);
			ftp_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/dialog.png", theme_name);
			dialog_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/context.png", theme_name);
			context_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery.png", theme_name);
			battery_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery_bar_red.png", theme_name);
			battery_bar_red_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery_bar_green.png", theme_name);
			battery_bar_green_image = vita2d_load_PNG_file(path);

			// Wallpapers
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/wallpaper.png", theme_name);
			vita2d_texture *image = vita2d_load_PNG_file(path);
			if (image)
				wallpaper_image[wallpaper_count++] = image;

			int i;
			for (i = 1; i < MAX_WALLPAPERS; i++) {
				snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/wallpaper%d.png", theme_name, i + 1);
				vita2d_texture *image = vita2d_load_PNG_file(path);
				if (image)
					wallpaper_image[wallpaper_count++] = image;
			}

			// Set random wallpaper
			int random_num = randomNumber(0, wallpaper_count - 1);
			current_wallpaper_image = wallpaper_image[random_num];

			if (current_wallpaper_image) {
				vita2d_free_texture(default_wallpaper);
				default_wallpaper = NULL;
				
				default_wallpaper = current_wallpaper_image;
			}
		}
	}

	// Load default pngs
	if (!ftp_image)
		ftp_image = vita2d_load_PNG_buffer(&_binary_resources_ftp_png_start);

	if (!dialog_image)
		dialog_image = vita2d_load_PNG_buffer(&_binary_resources_dialog_png_start);

	if (!context_image)
		context_image = vita2d_load_PNG_buffer(&_binary_resources_context_png_start);

	if (!battery_image)
		battery_image = vita2d_load_PNG_buffer(&_binary_resources_battery_png_start);

	if (!battery_bar_red_image)
		battery_bar_red_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_red_png_start);

	if (!battery_bar_green_image)
		battery_bar_green_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_green_png_start);
}
