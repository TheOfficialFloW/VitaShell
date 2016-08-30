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

#ifndef __MAIN_H__
#define __MAIN_H__

#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/audioout.h>
#include <psp2/audiodec.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/ime_dialog.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>
#include <psp2/message_dialog.h>
#include <psp2/moduleinfo.h>
#include <psp2/pgf.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/sysmodule.h>
#include <psp2/system_param.h>
#include <psp2/touch.h>
#include <psp2/types.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vita2d.h>
#include <ftpvita.h>

#include "functions.h"


#define ENABLE_DEBUGNET_LOGGING 1
#define ENABLE_FILE_LOGGING 1

// VitaShell version major.minor
#define VITASHELL_VERSION_MAJOR 0
#define VITASHELL_VERSION_MINOR 85

#define ALIGN(x, align) (((x) + ((align) - 1)) & ~((align) - 1))

#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))


enum Colors {
	// Primary colors
	RED				= 0xFF0000FF,
	GREEN			= 0xFF00FF00,
	BLUE			= 0xFFFF0000,
	// Secondary colors
	CYAN			= 0xFFFFFF00,
	MAGENTA			= 0xFFFF00FF,
	YELLOW			= 0xFF00FFFF,
	// Tertiary colors
	AZURE			= 0xFFFF7F00,
	VIOLET			= 0xFFFF007F,
	ROSE			= 0xFF7F00FF,
	ORANGE			= 0xFF007FFF,
	CHARTREUSE		= 0xFF00FF7F,
	SPRING_GREEN	= 0xFF7FFF00,
	// Grayscale
	WHITE			= 0xFFFFFFFF,
	LITEGRAY		= 0xFFBFBFBF,
	GRAY			= 0xFF7F7F7F,
	DARKGRAY		= 0xFF3F3F3F,
	BLACK			= 0xFF000000
};

#define NOALPHA 0xFF

#define COLOR_ALPHA(color, alpha) (color & 0x00FFFFFF) | ((alpha & 0xFF) << 24)

// Font
#define FONT_SIZE 1.0f
#define FONT_X_SPACE 15.0f
#define FONT_Y_SPACE 28.0f

#define pgf_draw_text(x, y, color, scale, text) \
	vita2d_pgf_draw_text(font, x, (y) + 20, color, scale, text)

#define pgf_draw_textf(x, y, color, scale, ...) \
	vita2d_pgf_draw_textf(font, x, (y) + 20, color, scale, __VA_ARGS__)

// Screen
#define SCREEN_WIDTH 960
#define SCREEN_HEIGHT 544
#define SCREEN_HALF_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT (SCREEN_HEIGHT / 2)

// Main
#define SHELL_MARGIN_X 20.0f
#define SHELL_MARGIN_Y 18.0f

#define PATH_Y (SHELL_MARGIN_Y + 1.0f * FONT_Y_SPACE)

#define START_Y (SHELL_MARGIN_Y + 3.0f * FONT_Y_SPACE)

#define MAX_WIDTH (SCREEN_WIDTH - (2.0f * SHELL_MARGIN_X))

// Hex
#define HEX_OFFSET_X 147.0f
#define HEX_CHAR_X (SCREEN_WIDTH - SHELL_MARGIN_X - 0x10 * FONT_X_SPACE)
#define HEX_OFFSET_SPACE 34.0f

// Scroll bar
#define SCROLL_BAR_X 5.0f
#define SCROLL_BAR_WIDTH 8.0f
#define SCROLL_BAR_MIN_HEIGHT 4.0f

// Context menu
#define CONTEXT_MENU_MIN_WIDTH 180.0f
#define CONTEXT_MENU_MARGIN 20.0f
#define CONTEXT_MENU_VELOCITY 10.0f

// File browser
#define MARK_WIDTH (SCREEN_WIDTH - 2.0f * SHELL_MARGIN_X)
#define INFORMATION_X 680.0f
#define MAX_NAME_WIDTH 530.0f
#define MAX_NAME_WIDTH_TILE 145.0f

// Uncommon dialog
#define UNCOMMON_DIALOG_PROGRESS_BAR_BOX_WIDTH 420.0f
#define UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT 8.0f

// Max entries
#define MAX_POSITION 999 //16
#define MAX_ENTRIES 999 //17

#define BIG_BUFFER_SIZE 16 * 1024 * 1024

enum ContextMenuModes {
	CONTEXT_MENU_CLOSED,
	CONTEXT_MENU_CLOSING,
	CONTEXT_MENU_OPENED,
	CONTEXT_MENU_OPENING,
};

enum DialogSteps {
	DIALOG_STEP_NONE,

	DIALOG_STEP_CANCELLED,

	DIALOG_STEP_ERROR,
	DIALOG_STEP_INFO,
	DIALOG_STEP_SYSTEM,

	DIALOG_STEP_FTP,

	DIALOG_STEP_NEW_FOLDER,

	DIALOG_STEP_COPYING,
	DIALOG_STEP_COPIED,
	DIALOG_STEP_MOVED,
	DIALOG_STEP_PASTE,

	DIALOG_STEP_DELETE_QUESTION,
	DIALOG_STEP_DELETE_CONFIRMED,
	DIALOG_STEP_INSTALL_WARNING,
 	DIALOG_STEP_INSTALL_WARNING_AGREED,
	DIALOG_STEP_DELETING,
	DIALOG_STEP_DELETED,

	DIALOG_STEP_INSTALL_QUESTION,
	DIALOG_STEP_INSTALL_CONFIRMED,
	DIALOG_STEP_INSTALLING,
	DIALOG_STEP_INSTALLED,

	DIALOG_STEP_RENAME,
};

extern vita2d_pgf *font;
extern char font_size_cache[256];

extern vita2d_texture *headphone_image, *audio_previous_image, *audio_pause_image, *audio_play_image, *audio_next_image;
extern vita2d_texture *game_card_storage_image, *game_card_image, *memory_card_image, *os0_image ;
extern vita2d_texture *run_file_image, *img_file_image, *unknown_file_image, *sa0_image, *ur0_image, *vd0_image, *vs0_image;
extern vita2d_texture *savedata0_image, *folder_image, *pd0_image, *app0_image, *ud0_image, *mark_image, *music_image;
extern vita2d_texture *title_bar_bg_image, *default_wallpaper;

extern int SCE_CTRL_ENTER, SCE_CTRL_CANCEL;

extern int dialog_step;

extern int use_custom_config;

void drawScrollBar(int pos, int n);

void drawScrollBar2(int file_list_length);

void drawShellInfo(char *path);

int isInArchive();


int TOUCH_FRONT();

#endif

