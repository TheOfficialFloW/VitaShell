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

INCLUDE_EXTERN_RESOURCE(colors_txt);
INCLUDE_EXTERN_RESOURCE(colors_txt_size);

INCLUDE_EXTERN_RESOURCE(vita_game_card_png);
INCLUDE_EXTERN_RESOURCE(vita_game_card_storage_png);
INCLUDE_EXTERN_RESOURCE(memory_card_png);
INCLUDE_EXTERN_RESOURCE(os0_png);
INCLUDE_EXTERN_RESOURCE(sa0_png);
INCLUDE_EXTERN_RESOURCE(ur0_png);
INCLUDE_EXTERN_RESOURCE(vd0_png);
INCLUDE_EXTERN_RESOURCE(vs0_png);
INCLUDE_EXTERN_RESOURCE(savedata0_png);
INCLUDE_EXTERN_RESOURCE(pd0_png);
INCLUDE_EXTERN_RESOURCE(app0_png);
INCLUDE_EXTERN_RESOURCE(ud0_png);

INCLUDE_EXTERN_RESOURCE(bg_wallpaper_png);
INCLUDE_EXTERN_RESOURCE(folder_png);
INCLUDE_EXTERN_RESOURCE(mark_png);
INCLUDE_EXTERN_RESOURCE(run_file_png);
INCLUDE_EXTERN_RESOURCE(image_file_png);
INCLUDE_EXTERN_RESOURCE(unknown_file_png);
INCLUDE_EXTERN_RESOURCE(music_file_png);
INCLUDE_EXTERN_RESOURCE(zip_file_png);
INCLUDE_EXTERN_RESOURCE(txt_file_png);
INCLUDE_EXTERN_RESOURCE(music_file_png);
INCLUDE_EXTERN_RESOURCE(title_bar_bg_png);
INCLUDE_EXTERN_RESOURCE(updir_png);

INCLUDE_EXTERN_RESOURCE(folder_icon_png);
INCLUDE_EXTERN_RESOURCE(file_icon_png);
INCLUDE_EXTERN_RESOURCE(archive_icon_png);
INCLUDE_EXTERN_RESOURCE(image_icon_png);
INCLUDE_EXTERN_RESOURCE(audio_icon_png);
INCLUDE_EXTERN_RESOURCE(sfo_icon_png);
INCLUDE_EXTERN_RESOURCE(text_icon_png);
INCLUDE_EXTERN_RESOURCE(wifi_png);
INCLUDE_EXTERN_RESOURCE(ftp_png);
INCLUDE_EXTERN_RESOURCE(battery_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_red_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_green_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_charge_png);

INCLUDE_EXTERN_RESOURCE(cover_png);
INCLUDE_EXTERN_RESOURCE(play_png);
INCLUDE_EXTERN_RESOURCE(pause_png);
INCLUDE_EXTERN_RESOURCE(fastforward_png);
INCLUDE_EXTERN_RESOURCE(fastrewind_png);

// Shell colors
int BACKGROUND_COLOR;
int TITLE_COLOR;
int PATH_COLOR;
int DATE_TIME_COLOR;

// Settings color
int SETTINGS_MENU_COLOR;
int SETTINGS_MENU_FOCUS_COLOR;
int SETTINGS_MENU_TITLE_COLOR;
int SETTINGS_MENU_ITEM_COLOR;
int SETTINGS_MENU_OPTION_COLOR;

// File browser colors
int FOCUS_COLOR;
int FILE_COLOR;
int SFO_COLOR;
int TXT_COLOR;
int FOLDER_COLOR;
int IMAGE_COLOR;
int ARCHIVE_COLOR;
int SCROLL_BAR_COLOR;
int SCROLL_BAR_BG_COLOR;
int MARKED_COLOR;

// Context menu colors
int CONTEXT_MENU_TEXT_COLOR;
int CONTEXT_MENU_FOCUS_COLOR;
int CONTEXT_MENU_COLOR;
int CONTEXT_MENU_MORE_COLOR;
int INVISIBLE_COLOR;

// Dialog colors
int DIALOG_COLOR;
int DIALOG_BG_COLOR;
int PROGRESS_BAR_COLOR;
int PROGRESS_BAR_BG_COLOR;

// Hex editor colors
int HEX_COLOR;
int HEX_OFFSET_COLOR;
int HEX_NIBBLE_COLOR;

// Text editor colors
int TEXT_COLOR;
int TEXT_FOCUS_COLOR;
int TEXT_LINE_NUMBER_COLOR;
int TEXT_LINE_NUMBER_COLOR_FOCUS;
int TEXT_HIGHLIGHT_COLOR;

// Photo viewer colors
int PHOTO_ZOOM_COLOR;

// Audio player colors
int AUDIO_INFO_ASSIGN;
int AUDIO_INFO;
int AUDIO_SPEED;
int AUDIO_TIME_CURRENT;
int AUDIO_TIME_SLASH;
int AUDIO_TIME_TOTAL;
int AUDIO_TIME_BAR;
int AUDIO_TIME_BAR_BG;

//UI2 colors
int ITEM_BORDER;
int ITEM_BORDER_SELECT;
int BACKGROUND_COLOR_TEXT_ITEM;
int ALPHA_TITLEBAR;

vita2d_texture *default_wallpaper = NULL, *game_card_storage_image = NULL, *game_card_image = NULL, *memory_card_image = NULL, *run_file_image = NULL, *img_file_image = NULL, *unknown_file_image = NULL, *sa0_image = NULL, *ur0_image = NULL, *vd0_image = NULL, *vs0_image = NULL, *savedata0_image = NULL, *pd0_image = NULL, *folder_image = NULL, *app0_image = NULL, *ud0_image = NULL, *mark_image = NULL, *music_image = NULL, *os0_image = NULL, *zip_file_image = NULL, *txt_file_image = NULL, *title_bar_bg_image = NULL, *updir_image = NULL ;


vita2d_texture *folder_icon = NULL, *file_icon = NULL, *archive_icon = NULL, *image_icon = NULL, *audio_icon = NULL, *sfo_icon = NULL, *text_icon = NULL,
			   *wifi_image = NULL, *ftp_image = NULL, *dialog_image = NULL, *context_image = NULL, *context_more_image = NULL, *settings_image = NULL, *battery_image = NULL,
			   *battery_bar_red_image = NULL, *battery_bar_green_image = NULL, *battery_bar_charge_image = NULL, *bg_browser_image = NULL, *bg_hex_image = NULL, *bg_text_image = NULL,
			   *bg_photo_image = NULL, *bg_audio_image = NULL, *cover_image = NULL, *play_image = NULL, *pause_image = NULL, *fastforward_image = NULL, *fastrewind_image = NULL;

vita2d_texture *wallpaper_image[MAX_WALLPAPERS];

vita2d_texture *previous_wallpaper_image = NULL, *current_wallpaper_image = NULL;

int wallpaper_count = 0;

vita2d_pgf *font = NULL;
char font_size_cache[256];

void loadTheme() {
	#define COLOR_ENTRY(name) { #name, CONFIG_TYPE_HEXDECIMAL, (void *)&name }
	ConfigEntry colors_entries[] = {
		// Shell colors
		COLOR_ENTRY(BACKGROUND_COLOR),
		COLOR_ENTRY(TITLE_COLOR),
		COLOR_ENTRY(PATH_COLOR),
		COLOR_ENTRY(DATE_TIME_COLOR),

		// Settings colors
		COLOR_ENTRY(SETTINGS_MENU_COLOR),
		COLOR_ENTRY(SETTINGS_MENU_FOCUS_COLOR),
		COLOR_ENTRY(SETTINGS_MENU_TITLE_COLOR),
		COLOR_ENTRY(SETTINGS_MENU_ITEM_COLOR),
		COLOR_ENTRY(SETTINGS_MENU_OPTION_COLOR),

		// File browser colors
		COLOR_ENTRY(FOCUS_COLOR),
		COLOR_ENTRY(FILE_COLOR),
		COLOR_ENTRY(SFO_COLOR),
		COLOR_ENTRY(TXT_COLOR),
		COLOR_ENTRY(FOLDER_COLOR),
		COLOR_ENTRY(IMAGE_COLOR),
		COLOR_ENTRY(ARCHIVE_COLOR),
		COLOR_ENTRY(SCROLL_BAR_COLOR),
		COLOR_ENTRY(SCROLL_BAR_BG_COLOR),
		COLOR_ENTRY(MARKED_COLOR),

		// Context menu colors
		COLOR_ENTRY(CONTEXT_MENU_TEXT_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_FOCUS_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_COLOR),
		COLOR_ENTRY(CONTEXT_MENU_MORE_COLOR),
		COLOR_ENTRY(INVISIBLE_COLOR),

		// Dialog colors
		COLOR_ENTRY(DIALOG_COLOR),
		COLOR_ENTRY(DIALOG_BG_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_COLOR),
		COLOR_ENTRY(PROGRESS_BAR_BG_COLOR),

		// Hex editor colors
		COLOR_ENTRY(HEX_COLOR),
		COLOR_ENTRY(HEX_OFFSET_COLOR),
		COLOR_ENTRY(HEX_NIBBLE_COLOR),

		// Text editor colors
		COLOR_ENTRY(TEXT_COLOR),
		COLOR_ENTRY(TEXT_FOCUS_COLOR),
		COLOR_ENTRY(TEXT_LINE_NUMBER_COLOR),
		COLOR_ENTRY(TEXT_LINE_NUMBER_COLOR_FOCUS),
		COLOR_ENTRY(TEXT_HIGHLIGHT_COLOR),

		// Photo viewer colors
		COLOR_ENTRY(PHOTO_ZOOM_COLOR),

		// Audio player colors
		COLOR_ENTRY(AUDIO_INFO_ASSIGN),
		COLOR_ENTRY(AUDIO_INFO),
		COLOR_ENTRY(AUDIO_SPEED),
		COLOR_ENTRY(AUDIO_TIME_CURRENT),
		COLOR_ENTRY(AUDIO_TIME_SLASH),
		COLOR_ENTRY(AUDIO_TIME_TOTAL),
		COLOR_ENTRY(AUDIO_TIME_BAR),
		COLOR_ENTRY(AUDIO_TIME_BAR_BG),

		//UI2 colors
		COLOR_ENTRY(ITEM_BORDER),
		COLOR_ENTRY(ITEM_BORDER_SELECT),
		COLOR_ENTRY(BACKGROUND_COLOR_TEXT_ITEM),
		COLOR_ENTRY(ALPHA_TITLEBAR),
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

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/wifi.png", theme_name);
			wifi_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/ftp.png", theme_name);
			ftp_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/dialog.png", theme_name);
			dialog_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/context.png", theme_name);
			context_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/context_more.png", theme_name);
			context_more_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/settings.png", theme_name);
			settings_image = vita2d_load_PNG_file(path);

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

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/bg_audioplayer.png", theme_name);
 			bg_audio_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/cover.png", theme_name);
 			cover_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/play.png", theme_name);
 			play_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/pause.png", theme_name);
 			pause_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/fastforward.png", theme_name);
 			fastforward_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/fastrewind.png", theme_name);
 			fastrewind_image = vita2d_load_PNG_file(path);

			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/vita_game_card.png", theme_name);
			game_card_storage_image = vita2d_load_PNG_file(path);
			
		        snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/vita_game_card_storage.png", theme_name);
			game_card_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/memory_card.png", theme_name);
			memory_card_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/os0.png", theme_name);
			os0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/sa0.png", theme_name);
			sa0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/ur0.png", theme_name);
			ur0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/vd0.png", theme_name);
			vd0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/vs0.png", theme_name);
			vs0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/savedata0.png", theme_name);
			savedata0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/pd0.png", theme_name);
			pd0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/app0.png", theme_name);
			app0_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/ud0.png", theme_name);
			ud0_image = vita2d_load_PNG_file(path);
		
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/folder.png", theme_name);
			folder_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/mark.png", theme_name);
			mark_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/run_file.png", theme_name);
			run_file_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/image_file.png", theme_name);
			img_file_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/unknown_file.png", theme_name);
			unknown_file_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/music_file.png", theme_name);
			music_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/zip_file.png", theme_name);
			zip_file_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/txt_file.png", theme_name);
			txt_file_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/music_file.png", theme_name);
			music_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/title_bar_bg.png", theme_name);
			title_bar_bg_image = vita2d_load_PNG_file(path);
			
			snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/updir.png", theme_name);
			updir_image = vita2d_load_PNG_file(path);

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

			// Font
			// TheFloW: I am using a modified vita2dlib that doesn't have this function yet
			//          and I'm too lazy to update mine. Will do it in the future.
			// snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/font.pgf", theme_name);
 			// font = vita2d_load_custom_pgf(path);
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

	if (!wifi_image)
		wifi_image = vita2d_load_PNG_buffer(&_binary_resources_wifi_png_start);

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

	if (!settings_image) {
		settings_image = vita2d_create_empty_texture(SCREEN_WIDTH, SCREEN_HEIGHT);
		void *data = vita2d_texture_get_datap(settings_image);

		int y;
		for (y = 0; y < SCREEN_HEIGHT; y++) {
			int x;
			for (x = 0; x < SCREEN_WIDTH; x++) {
				((uint32_t *)data)[x + SCREEN_WIDTH * y] = SETTINGS_MENU_COLOR;
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

	if (!cover_image)
		cover_image = vita2d_load_PNG_buffer(&_binary_resources_cover_png_start);

	if (!play_image)
		play_image = vita2d_load_PNG_buffer(&_binary_resources_play_png_start);

	if (!pause_image)
		pause_image = vita2d_load_PNG_buffer(&_binary_resources_pause_png_start);
	
	if (!fastforward_image)
		fastforward_image = vita2d_load_PNG_buffer(&_binary_resources_fastforward_png_start);
	
	if (!fastrewind_image)
		fastrewind_image = vita2d_load_PNG_buffer(&_binary_resources_fastrewind_png_start);
	
	if (!game_card_image)
	  game_card_image = vita2d_load_PNG_buffer(&_binary_resources_vita_game_card_png_start);

	if (!game_card_storage_image)
	  game_card_storage_image = vita2d_load_PNG_buffer(&_binary_resources_vita_game_card_storage_png_start);

	if (!memory_card_image)
	  memory_card_image = vita2d_load_PNG_buffer(&_binary_resources_memory_card_png_start);

	if (!os0_image)	  
	  os0_image = vita2d_load_PNG_buffer(&_binary_resources_os0_png_start);

	if (!sa0_image)
	  sa0_image = vita2d_load_PNG_buffer(&_binary_resources_sa0_png_start);
	
	if (!ur0_image)
	  ur0_image = vita2d_load_PNG_buffer(&_binary_resources_ur0_png_start);
	
	if (!vd0_image)
	  vd0_image = vita2d_load_PNG_buffer(&_binary_resources_vd0_png_start);
	
	if (!vs0_image)
	  vs0_image = vita2d_load_PNG_buffer(&_binary_resources_vs0_png_start);
	
	if (!savedata0_image)
	  savedata0_image = vita2d_load_PNG_buffer(&_binary_resources_savedata0_png_start);
	
	if (!pd0_image)
	  pd0_image = vita2d_load_PNG_buffer(&_binary_resources_pd0_png_start);
	
	if (!app0_image)	  
	  app0_image = vita2d_load_PNG_buffer(&_binary_resources_app0_png_start);
	
	if (!ud0_image)
	  ud0_image = vita2d_load_PNG_buffer(&_binary_resources_ud0_png_start);	

	default_wallpaper = vita2d_load_PNG_buffer(&_binary_resources_bg_wallpaper_png_start);

	if (!folder_image)
	  folder_image = vita2d_load_PNG_buffer(&_binary_resources_folder_png_start);

	if (!mark_image)
	  mark_image = vita2d_load_PNG_buffer(&_binary_resources_mark_png_start);

	if (!run_file_image)
	  run_file_image =  vita2d_load_PNG_buffer(&_binary_resources_run_file_png_start);
	
	if (!img_file_image)
	  img_file_image =  vita2d_load_PNG_buffer(&_binary_resources_image_file_png_start);
	
	if (!unknown_file_image)
	  unknown_file_image =  vita2d_load_PNG_buffer(&_binary_resources_unknown_file_png_start);
	
	if (!music_image)
	  music_image = vita2d_load_PNG_buffer(&_binary_resources_music_file_png_start);
	
	if (!zip_file_image)
	  zip_file_image = vita2d_load_PNG_buffer(&_binary_resources_zip_file_png_start);
	
	if (!txt_file_image)
	  txt_file_image = vita2d_load_PNG_buffer(&_binary_resources_txt_file_png_start);
	
	if (!title_bar_bg_image)
	  title_bar_bg_image = vita2d_load_PNG_buffer(&_binary_resources_title_bar_bg_png_start);
	
	if (!updir_image)
	  updir_image = vita2d_load_PNG_buffer(&_binary_resources_updir_png_start);

	// Load default font
	if (!font)
		font = vita2d_load_default_pgf();

	// Font size cache
	int i;
	for (i = 0; i < 256; i++) {
		char character[2];
		character[0] = i;
		character[1] = '\0';

		font_size_cache[i] = vita2d_pgf_text_width(font, FONT_SIZE, character);
	}
}
