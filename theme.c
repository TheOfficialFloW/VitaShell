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
#include "utils.h"

extern unsigned char _binary_resources_folder_icon_png_start;
extern unsigned char _binary_resources_file_icon_png_start;
extern unsigned char _binary_resources_archive_icon_png_start;
extern unsigned char _binary_resources_image_icon_png_start;
extern unsigned char _binary_resources_audio_icon_png_start;
extern unsigned char _binary_resources_sfo_icon_png_start;
extern unsigned char _binary_resources_text_icon_png_start;
extern unsigned char _binary_resources_ftp_png_start;
extern unsigned char _binary_resources_battery_png_start;
extern unsigned char _binary_resources_battery_bar_red_png_start;
extern unsigned char _binary_resources_battery_bar_green_png_start;
extern unsigned char _binary_resources_battery_bar_charge_png_start;

extern unsigned char _binary_resources_colors_txt_start;
extern unsigned char _binary_resources_colors_txt_size;

int BACKGROUND_COLOR;
int GENERAL_COLOR;
int TITLE_COLOR;
int PATH_COLOR;
int DATE_TIME_COLOR;
int FOCUS_COLOR;
int FILE_COLOR;
int FOLDER_COLOR;
int IMAGE_COLOR;
int ARCHIVE_COLOR;
int SCROLL_BAR_COLOR;
int SCROLL_BAR_BG_COLOR;
int MARKED_COLOR;
int INVISIBLE_COLOR;
int DIALOG_BG_COLOR;
int CONTEXT_MENU_COLOR;
int CONTEXT_MENU_MORE_COLOR;
int PROGRESS_BAR_COLOR;
int PROGRESS_BAR_BG_COLOR;
int HEX_OFFSET_COLOR;
int HEX_NIBBLE_COLOR;
int TEXT_LINE_NUMBER_COLOR;
int TEXT_LINE_NUMBER_COLOR_FOCUS;

vita2d_texture *folder_icon = NULL, *file_icon = NULL, *archive_icon = NULL, *image_icon = NULL, *audio_icon = NULL, *sfo_icon = NULL, *text_icon = NULL,
			   *ftp_image = NULL, *dialog_image = NULL, *context_image = NULL, *context_more_image = NULL, *battery_image = NULL, *battery_bar_red_image = NULL,
			   *battery_bar_green_image = NULL, *battery_bar_charge_image = NULL, *bg_browser_image = NULL, *bg_hex_image = NULL,
			   *bg_text_image = NULL, *bg_photo_image = NULL;

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
		COLOR_ENTRY(FILE_COLOR),
		COLOR_ENTRY(FOLDER_COLOR),
		COLOR_ENTRY(IMAGE_COLOR),
		COLOR_ENTRY(ARCHIVE_COLOR),
		COLOR_ENTRY(SCROLL_BAR_COLOR),
		COLOR_ENTRY(SCROLL_BAR_BG_COLOR),
		COLOR_ENTRY(MARKED_COLOR),
		COLOR_ENTRY(INVISIBLE_COLOR),
		COLOR_ENTRY(DIALOG_BG_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_MORE_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_BG_COLOR),
		COLOR_ENTRY(HEX_OFFSET_COLOR),
		COLOR_ENTRY(HEX_NIBBLE_COLOR),
		COLOR_ENTRY(TEXT_LINE_NUMBER_COLOR),
		COLOR_ENTRY(TEXT_LINE_NUMBER_COLOR_FOCUS),
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
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/folder_icon.png", theme_name);
			folder_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/file_icon.png", theme_name);
			file_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/archive_icon.png", theme_name);
			archive_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/image_icon.png", theme_name);
			image_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/audio_icon.png", theme_name);
			audio_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/sfo_icon.png", theme_name);
			sfo_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/text_icon.png", theme_name);
			text_icon = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/ftp.png", theme_name);
			ftp_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/dialog.png", theme_name);
			dialog_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/context.png", theme_name);
			context_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/context_more.png", theme_name);
			context_more_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery.png", theme_name);
			battery_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery_bar_red.png", theme_name);
			battery_bar_red_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery_bar_green.png", theme_name);
			battery_bar_green_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/battery_bar_charge.png", theme_name);
 			battery_bar_charge_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/bg_browser.png", theme_name);
 			bg_browser_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/bg_hexeditor.png", theme_name);
 			bg_hex_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/bg_texteditor.png", theme_name);
 			bg_text_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/bg_photoviewer.png", theme_name);
 			bg_photo_image = vita2d_load_PNG_file(path);

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
			if (wallpaper_count > 0) {
				int random_num = randomNumber(0, wallpaper_count - 1);
				current_wallpaper_image = wallpaper_image[random_num];
			}
		}
	}

	// Load default pngs
	if (!folder_icon)
		folder_icon = vita2d_load_PNG_buffer(&_binary_resources_folder_icon_png_start);

	if (!file_icon)
		file_icon = vita2d_load_PNG_buffer(&_binary_resources_file_icon_png_start);

	if (!archive_icon)
		archive_icon = vita2d_load_PNG_buffer(&_binary_resources_archive_icon_png_start);

	if (!image_icon)
		image_icon = vita2d_load_PNG_buffer(&_binary_resources_image_icon_png_start);

	if (!audio_icon)
		audio_icon = vita2d_load_PNG_buffer(&_binary_resources_audio_icon_png_start);

	if (!sfo_icon)
		sfo_icon = vita2d_load_PNG_buffer(&_binary_resources_sfo_icon_png_start);

	if (!text_icon)
		text_icon = vita2d_load_PNG_buffer(&_binary_resources_text_icon_png_start);

	if (!ftp_image)
		ftp_image = vita2d_load_PNG_buffer(&_binary_resources_ftp_png_start);

	if (!dialog_image) {
		dialog_image = vita2d_create_empty_texture(SCREEN_WIDTH, SCREEN_HEIGHT);
		void *data = vita2d_texture_get_datap(dialog_image);

		int y;
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			int x;
			for (x = 0; x < SCREEN_WIDTH; x++) {
				((uint32_t *)data)[x + SCREEN_WIDTH * y] = DIALOG_BG_COLOR;
			}
		}
	}

	if (!context_image) {
		context_image = vita2d_create_empty_texture(SCREEN_WIDTH, SCREEN_HEIGHT);
		void *data = vita2d_texture_get_datap(context_image);

		int y;
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			int x;
			for (x = 0; x < SCREEN_WIDTH; x++) {
				((uint32_t *)data)[x + SCREEN_WIDTH * y] = CONTEXT_MENU_COLOR;
			}
		}
	}

	if (!context_more_image) {
		context_more_image = vita2d_create_empty_texture(SCREEN_WIDTH, SCREEN_HEIGHT);
		void *data = vita2d_texture_get_datap(context_more_image);

		int y;
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			int x;
			for (x = 0; x < SCREEN_WIDTH; x++) {
				((uint32_t *)data)[x + SCREEN_WIDTH * y] = CONTEXT_MENU_MORE_COLOR;
			}
		}
	}

	if (!battery_image)
		battery_image = vita2d_load_PNG_buffer(&_binary_resources_battery_png_start);

	if (!battery_bar_red_image)
		battery_bar_red_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_red_png_start);

	if (!battery_bar_green_image)
		battery_bar_green_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_green_png_start);
	
	if (!battery_bar_charge_image)
		battery_bar_charge_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_charge_png_start);
}