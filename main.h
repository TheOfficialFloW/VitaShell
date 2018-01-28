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

#ifndef __MAIN_H__
#define __MAIN_H__

#include <vitasdk.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>
#include <locale.h>

#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <archive.h>
#include <archive_entry.h>

#include <zlib.h>

#include <vita2d.h>
#include <ftpvita.h>

#include <taihen.h>

#include <vitashell_user.h>

#include "file.h"
#include "vitashell_config.h"

#define INCLUDE_EXTERN_RESOURCE(name) extern unsigned char _binary_resources_##name##_start; extern unsigned char _binary_resources_##name##_size; \

// VitaShell version major.minor
#define VITASHELL_VERSION_MAJOR 0x01
#define VITASHELL_VERSION_MINOR 0x81

#define VITASHELL_VERSION ((VITASHELL_VERSION_MAJOR << 0x18) | (VITASHELL_VERSION_MINOR << 0x10))

#define VITASHELL_LASTDIR "ux0:VitaShell/internal/lastdir.txt"

#define VITASHELL_TITLEID "VITASHELL"

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define IS_DIGIT(i) (i >= '0' && i <= '9')

#define NOALPHA 0xFF

#define COLOR_ALPHA(color, alpha) (color & 0x00FFFFFF) | ((alpha & 0xFF) << 24)

// Font
#define FONT_SIZE 1.0f
#define FONT_X_SPACE 15.0f
#define FONT_Y_SPACE 23.0f

#define pgf_draw_text(x, y, color, text) \
  vita2d_pgf_draw_text(font, x, (y)+20, color, FONT_SIZE, text)

#define pgf_draw_textf(x, y, color, ...) \
  vita2d_pgf_draw_textf(font, x, (y)+20, color, FONT_SIZE, __VA_ARGS__)

#define pgf_text_width(text) \
  vita2d_pgf_text_width(font, FONT_SIZE, text)
  
// Screen
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544
#define SCREEN_HALF_WIDTH (SCREEN_WIDTH/2)
#define SCREEN_HALF_HEIGHT (SCREEN_HEIGHT/2)

// Main
#define SHELL_MARGIN_X 20.0f
#define SHELL_MARGIN_Y 18.0f

#define PATH_Y (SHELL_MARGIN_Y + 1.0f * FONT_Y_SPACE)

#define START_Y (SHELL_MARGIN_Y + 3.0f * FONT_Y_SPACE)

#define MAX_WIDTH (SCREEN_WIDTH - 2.0f * SHELL_MARGIN_X)

#define STATUS_BAR_SPACE_X 12.0f

// Hex
#define HEX_OFFSET_X 147.0f
#define HEX_CHAR_X (SCREEN_WIDTH - SHELL_MARGIN_X - 0x10 * FONT_X_SPACE)
#define HEX_OFFSET_SPACE 34.0f

// Coredump
#define COREDUMP_INFO_X (SHELL_MARGIN_X + 160.0f)
#define COREDUMP_REGISTER_NAME_SPACE 45.0f
#define COREDUMP_REGISTER_SPACE 165.0f

// Scroll bar
#define SCROLL_BAR_X 0.0f
#define SCROLL_BAR_WIDTH 8.0f
#define SCROLL_BAR_MIN_HEIGHT 4.0f

// Context menu
#define CONTEXT_MENU_MIN_WIDTH 180.0f
#define CONTEXT_MENU_MARGIN 20.0f
#define CONTEXT_MENU_VELOCITY 10.0f

// File browser
#define FILE_X (SHELL_MARGIN_X+26.0f)
#define MARK_WIDTH (SCREEN_WIDTH - 2.0f * SHELL_MARGIN_X)
#define INFORMATION_X 680.0f
#define MAX_NAME_WIDTH 500.0f

// Uncommon dialog
#define UNCOMMON_DIALOG_MAX_WIDTH 800
#define UNCOMMON_DIALOG_PROGRESS_BAR_BOX_WIDTH 420.0f
#define UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT 8.0f
#define MSG_DIALOG_MODE_QR_SCAN 10

// QR Camera
#define CAM_WIDTH 640
#define CAM_HEIGHT 360

// Max entries
#define MAX_QR_LENGTH 1024
#define MAX_POSITION 16
#define MAX_ENTRIES 17
#define MAX_URL_LENGTH 128

#define BIG_BUFFER_SIZE 16 * 1024 * 1024

enum RefreshModes {
  REFRESH_MODE_NONE,
  REFRESH_MODE_NORMAL,
  REFRESH_MODE_SETFOCUS,
};

enum DialogSteps {
  DIALOG_STEP_NONE,

  DIALOG_STEP_CANCELED,

  DIALOG_STEP_ERROR,
  DIALOG_STEP_INFO,
  DIALOG_STEP_SYSTEM,

  DIALOG_STEP_REFRESH_LIVEAREA_QUESTION,
  DIALOG_STEP_REFRESH_LICENSE_DB_QUESTION,
  DIALOG_STEP_REFRESHING,

  DIALOG_STEP_USB_ATTACH_WAIT,

  DIALOG_STEP_FTP_WAIT,
  DIALOG_STEP_FTP,

  DIALOG_STEP_USB_WAIT,
  DIALOG_STEP_USB,

  DIALOG_STEP_RENAME,
  DIALOG_STEP_NEW_FOLDER,

  DIALOG_STEP_COPYING,
  DIALOG_STEP_COPIED,
  DIALOG_STEP_MOVED,
  DIALOG_STEP_PASTE,

  DIALOG_STEP_DELETE_QUESTION,
  DIALOG_STEP_DELETE_CONFIRMED,
  DIALOG_STEP_DELETING,
  DIALOG_STEP_DELETED,

  DIALOG_STEP_COMPRESS_NAME,
  DIALOG_STEP_COMPRESS_LEVEL,
  DIALOG_STEP_COMPRESSING,
  DIALOG_STEP_COMPRESSED,

  DIALOG_STEP_EXPORT_QUESTION,
  DIALOG_STEP_EXPORT_CONFIRMED,
  DIALOG_STEP_EXPORTING,

  DIALOG_STEP_INSTALL_QUESTION,
  DIALOG_STEP_INSTALL_CONFIRMED,
  DIALOG_STEP_INSTALL_CONFIRMED_QR,
  DIALOG_STEP_INSTALL_WARNING,
  DIALOG_STEP_INSTALL_WARNING_AGREED,
  DIALOG_STEP_INSTALLING,
  DIALOG_STEP_INSTALLED,

  DIALOG_STEP_UPDATE_QUESTION,
  DIALOG_STEP_DOWNLOADING,
  DIALOG_STEP_DOWNLOADED,
  DIALOG_STEP_EXTRACTING,
  DIALOG_STEP_EXTRACTED,

  DIALOG_STEP_HASH_QUESTION,
  DIALOG_STEP_HASH_CONFIRMED,
  DIALOG_STEP_HASHING,

  DIALOG_STEP_SETTINGS_AGREEMENT,
  DIALOG_STEP_SETTINGS_STRING,
  
  DIALOG_STEP_QR,
  DIALOG_STEP_QR_DONE,
  DIALOG_STEP_QR_WAITING,
  DIALOG_STEP_QR_CONFIRM,
  DIALOG_STEP_QR_DOWNLOADING,
  DIALOG_STEP_QR_DOWNLOADED,
  DIALOG_STEP_QR_DOWNLOADED_VPK,
  DIALOG_STEP_QR_OPEN_WEBSITE,
  DIALOG_STEP_QR_SHOW_CONTENTS,
  
  DIALOG_STEP_ENTER_PASSWORD,
  
  DIALOG_STEP_ADHOC_SEND_NETCHECK,
  DIALOG_STEP_ADHOC_SEND_WAITING,
  DIALOG_STEP_ADHOC_SEND_CLIENT_DECLINED,
  DIALOG_STEP_ADHOC_SENDING,
  DIALOG_STEP_ADHOC_SENDED,
  DIALOG_STEP_ADHOC_RECEIVE_NETCHECK,
  DIALOG_STEP_ADHOC_RECEIVE_SEARCHING,
  DIALOG_STEP_ADHOC_RECEIVE_QUESTION,
  DIALOG_STEP_ADHOC_RECEIVING,
  DIALOG_STEP_ADHOC_RECEIVED,
};

extern FileList file_list, mark_list, copy_list, install_list;

extern char cur_file[MAX_PATH_LENGTH];
extern char archive_copy_path[MAX_PATH_LENGTH];
extern char archive_path[MAX_PATH_LENGTH];

extern int base_pos, rel_pos;
extern int sort_mode, copy_mode;

extern vita2d_pgf *font;
extern char font_size_cache[256];

extern VitaShellConfig vitashell_config;

extern int SCE_CTRL_ENTER, SCE_CTRL_CANCEL;

extern int use_custom_config;

void dirLevelUp();

int getDialogStep();
void setDialogStep(int step);

void drawScrollBar(int pos, int n);
void drawShellInfo(const char *path);

int isInArchive();

int refreshFileList();

void ftpvita_PROM(ftpvita_client_info_t *client);

#endif
