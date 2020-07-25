/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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
#include "file.h"
#include "config.h"
#include "theme.h"
#include "utils.h"

INCLUDE_EXTERN_RESOURCE(default_colors_txt);
INCLUDE_EXTERN_RESOURCE(default_archive_icon_png);
INCLUDE_EXTERN_RESOURCE(default_audio_icon_png);
INCLUDE_EXTERN_RESOURCE(default_battery_bar_charge_png);
INCLUDE_EXTERN_RESOURCE(default_battery_bar_green_png);
INCLUDE_EXTERN_RESOURCE(default_battery_bar_red_png);
INCLUDE_EXTERN_RESOURCE(default_battery_png);
INCLUDE_EXTERN_RESOURCE(default_cover_png);
INCLUDE_EXTERN_RESOURCE(default_fastforward_png);
INCLUDE_EXTERN_RESOURCE(default_fastrewind_png);
INCLUDE_EXTERN_RESOURCE(default_file_icon_png);
INCLUDE_EXTERN_RESOURCE(default_file_symlink_icon_png);
INCLUDE_EXTERN_RESOURCE(default_folder_icon_png);
INCLUDE_EXTERN_RESOURCE(default_folder_symlink_icon_png);
INCLUDE_EXTERN_RESOURCE(default_ftp_png);
INCLUDE_EXTERN_RESOURCE(default_image_icon_png);
INCLUDE_EXTERN_RESOURCE(default_pause_png);
INCLUDE_EXTERN_RESOURCE(default_play_png);
INCLUDE_EXTERN_RESOURCE(default_sfo_icon_png);
INCLUDE_EXTERN_RESOURCE(default_text_icon_png);

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
int FILE_SYMLINK_COLOR;
int FOLDER_SYMLINK_COLOR;
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

vita2d_texture *folder_icon = NULL, *folder_symlink_icon = NULL, *file_icon = NULL,*file_symlink_icon = NULL, *archive_icon =NULL, *image_icon = NULL,
    *audio_icon = NULL, *sfo_icon = NULL, *text_icon = NULL,
         *ftp_image = NULL, *dialog_image = NULL, *context_image = NULL, *context_more_image = NULL, *settings_image = NULL, *battery_image = NULL,
         *battery_bar_red_image = NULL, *battery_bar_green_image = NULL, *battery_bar_charge_image = NULL, *bg_browser_image = NULL, *bg_hex_image = NULL, *bg_text_image = NULL,
         *bg_photo_image = NULL, *bg_audio_image = NULL, *cover_image = NULL, *play_image = NULL, *pause_image = NULL, *fastforward_image = NULL, *fastrewind_image = NULL;

vita2d_texture *wallpaper_image;

vita2d_texture *previous_wallpaper_image = NULL, *current_wallpaper_image = NULL;

vita2d_pgf *font = NULL;
char font_size_cache[256];

typedef struct {
  char *name;
  void *default_buf;
  vita2d_texture **texture;
} ThemeImage;

ThemeImage theme_images[] = {
  { "archive_icon.png", &_binary_resources_default_archive_icon_png_start, &archive_icon },
  { "audio_icon.png", &_binary_resources_default_audio_icon_png_start, &audio_icon },
  { "battery.png", &_binary_resources_default_battery_png_start, &battery_image },
  { "battery_bar_charge.png", &_binary_resources_default_battery_bar_charge_png_start, &battery_bar_charge_image },
  { "battery_bar_green.png", &_binary_resources_default_battery_bar_green_png_start, &battery_bar_green_image },
  { "battery_bar_red.png", &_binary_resources_default_battery_bar_red_png_start, &battery_bar_red_image },
  { "bg_audioplayer.png", NULL, &bg_audio_image },
  { "bg_browser.png", NULL, &bg_browser_image },
  { "bg_hexeditor.png", NULL, &bg_hex_image },
  { "bg_photoviewer.png", NULL, &bg_photo_image },
  { "bg_texteditor.png", NULL, &bg_text_image },
  { "context.png", NULL, &context_image },
  { "context_more.png", NULL, &context_more_image },
  { "cover.png", &_binary_resources_default_cover_png_start, &cover_image },
  { "dialog.png", NULL, &dialog_image },
  { "fastforward.png", &_binary_resources_default_fastforward_png_start, &fastforward_image },
  { "fastrewind.png", &_binary_resources_default_fastrewind_png_start, &fastrewind_image },
  { "file_icon.png", &_binary_resources_default_file_icon_png_start, &file_icon },
  { "file_symlink_icon.png", &_binary_resources_default_file_symlink_icon_png_start,
      &file_symlink_icon },
  { "folder_icon.png", &_binary_resources_default_folder_icon_png_start, &folder_icon },
  { "folder_symlink_icon.png", &_binary_resources_default_folder_symlink_icon_png_start,
      &folder_symlink_icon },
  { "ftp.png", &_binary_resources_default_ftp_png_start, &ftp_image },
  { "image_icon.png", &_binary_resources_default_image_icon_png_start, &image_icon },
  { "pause.png", &_binary_resources_default_pause_png_start, &pause_image },
  { "play.png", &_binary_resources_default_play_png_start, &play_image },
  { "settings.png", NULL, &settings_image },
  { "sfo_icon.png", &_binary_resources_default_sfo_icon_png_start, &sfo_icon },
  { "text_icon.png", &_binary_resources_default_text_icon_png_start, &text_icon },
  { "wallpaper.png", NULL, &wallpaper_image },
};

#define N_THEME_IMAGES (sizeof(theme_images) / sizeof(ThemeImage))

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
    COLOR_ENTRY(FILE_SYMLINK_COLOR),
    COLOR_ENTRY(FOLDER_SYMLINK_COLOR),
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
  };

  int i;

  // Load default config file
  readConfigBuffer(&_binary_resources_default_colors_txt_start, (int)&_binary_resources_default_colors_txt_size,
                   colors_entries, sizeof(colors_entries) / sizeof(ConfigEntry));

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
      
      // Font
      snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/font.pgf", theme_name);
      font = vita2d_load_custom_pgf(path);
      
      // Load theme
      for (i = 0; i < N_THEME_IMAGES; i++) {
        snprintf(path, MAX_PATH_LENGTH, "ux0:VitaShell/theme/%s/%s", theme_name, theme_images[i].name);
        if (theme_images[i].texture && *(theme_images[i].texture) == NULL)
          *(theme_images[i].texture) = vita2d_load_PNG_file(path);
      }
    }
  }

  // Load default theme
  for (i = 0; i < N_THEME_IMAGES; i++) {
    if (theme_images[i].texture && *(theme_images[i].texture) == NULL && theme_images[i].default_buf)
      *(theme_images[i].texture) = vita2d_load_PNG_buffer(theme_images[i].default_buf);
  }

  // Load default pngs
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

  // Load system fonts
  if (!font)
    font = loadSystemFonts();

  // Font size cache
  for (i = 0; i < 256; i++) {
    char character[2];
    character[0] = i;
    character[1] = '\0';

    font_size_cache[i] = pgf_text_width(character);
  }
}
