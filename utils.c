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
#include "message_dialog.h"
#include "uncommon_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "bm.h"

SceCtrlData pad;
uint32_t old_buttons, current_buttons, pressed_buttons, hold_buttons, hold2_buttons, released_buttons;

static int netdbg_sock = -1;
static void *net_memory = NULL;
static int net_init = -1;

static int lock_power = 0;

void startDrawing(vita2d_texture *bg) {
	vita2d_start_drawing();
	vita2d_set_clear_color(BACKGROUND_COLOR);
	vita2d_clear_screen();

	if (wallpaper_image) {
		vita2d_draw_texture(wallpaper_image, 0.0f, 0.0f);
	} else if (bg) {
		vita2d_draw_texture(bg, 0.0f, 0.0f);
	}
}

void drawingWallpaperUI2(vita2d_texture *bg,float x, float y, float tex_x, float tex_y, float tex_w, float tex_h) {
  	if (wallpaper_image) {
		vita2d_draw_texture_part(wallpaper_image, x, y, tex_x, tex_y, tex_w, tex_h);
	} else if (bg) {
		vita2d_draw_texture_part(bg, x, y, tex_x, tex_y, tex_w, tex_h);
	}
}

void endDrawing() {
	drawUncommonDialog();
	vita2d_end_drawing();
	vita2d_common_dialog_update();
	vita2d_swap_buffers();
	sceDisplayWaitVblankStart();
}

void closeWaitDialog() {
	sceMsgDialogClose();

	while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
		sceKernelDelayThread(1000);
	}
}

void errorDialog(int error) {
	if (error < 0) {
		initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[ERROR], error);
		dialog_step = DIALOG_STEP_ERROR;
	}
}

void infoDialog(char *msg, ...) {
	va_list list;
	char string[512];

	va_start(list, msg);
	vsprintf(string, msg, list);
	va_end(list);

	initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, string);
	dialog_step = DIALOG_STEP_INFO;
}

int checkMemoryCardFreeSpace(uint64_t size) {
	uint64_t free_size = 0, max_size = 0;
	sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

	if (size >= (free_size + (40 * 1024 * 1024))) {
		closeWaitDialog();

		char size_string[16];
		getSizeString(size_string, size - (free_size + (40 * 1024 * 1024)));
		infoDialog(language_container[NO_SPACE_ERROR], size_string);

		return 1;
	}

	return 0;
}

int power_tick_thread(SceSize args, void *argp) {
	while (1) {
		if (lock_power > 0) {
			sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
		}

		sceKernelDelayThread(10 * 1000 * 1000);
	}

	return 0;
}

void initPowerTickThread() {
	SceUID thid = sceKernelCreateThread("power_tick_thread", power_tick_thread, 0x10000100, 0x40000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);
}

void powerLock() {
	lock_power++;
}

void powerUnlock() {
	lock_power--;
	if (lock_power < 0) lock_power = 0;
}

void readPad() {
	static int hold_n = 0, hold2_n = 0;

	memset(&pad, 0, sizeof(SceCtrlData));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	if (pad.ly < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_UP;
	} else if (pad.ly > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_DOWN;
	}

	if (pad.lx < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_LEFT;
	} else if (pad.lx > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_RIGHT;
	}

	if (pad.ry < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_UP;
	} else if (pad.ry > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_DOWN;
	}

	if (pad.rx < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_LEFT;
	} else if (pad.rx > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_RIGHT;
	}

	current_buttons = pad.buttons;
	pressed_buttons = current_buttons & ~old_buttons;
	hold_buttons = pressed_buttons;
	hold2_buttons = pressed_buttons;
	released_buttons = ~current_buttons & old_buttons;

	if (old_buttons == current_buttons) {
		hold_n++;
		if (hold_n >= 10) {
			hold_buttons = current_buttons;
			hold_n = 6;
		}

		hold2_n++;
		if (hold2_n >= 10) {
			hold2_buttons = current_buttons;
			hold2_n = 10;
		}
	} else {
		hold_n = 0;
		hold2_n = 0;
		old_buttons = current_buttons;
	}
}

int holdButtons(SceCtrlData *pad, uint32_t buttons, uint64_t time) {
	if ((pad->buttons & buttons) == buttons) {
		uint64_t time_start = sceKernelGetProcessTimeWide();

		while ((pad->buttons & buttons) == buttons) {
			sceKernelDelayThread(10 * 1000);
			sceCtrlPeekBufferPositive(0, pad, 1);

			if ((sceKernelGetProcessTimeWide() - time_start) >= time) {
				return 1;
			}
		}
	}

	return 0;
}

int hasEndSlash(char *path) {
	return path[strlen(path) - 1] == '/';
}

int removeEndSlash(char *path) {
	int len = strlen(path);

	if (path[len - 1] == '/') {
		path[len - 1] = '\0';
		return 1;
	}

	return 0;
}

int addEndSlash(char *path) {
	int len = strlen(path);
	if (len < MAX_PATH_LENGTH - 2) {
		if (path[len - 1] != '/') {
			strcat(path, "/");
			return 1;
		}
	}

	return 0;
}

void getSizeString(char *string, uint64_t size) {
	double double_size = (double)size;

	int i = 0;
	static char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
	while (double_size >= 1024.0f) {
		double_size /= 1024.0f;
		i++;
	}

	sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, double_size, units[i]);
}

void convertUtcToLocalTime(SceDateTime *time_local, SceDateTime *time_utc) {
	SceRtcTick tick;
	sceRtcGetTick(time_utc, &tick);
	sceRtcConvertUtcToLocalTime(&tick, &tick);
	sceRtcSetTick(time_local, &tick);	
}

void convertLocalTimeToUtc(SceDateTime *time_utc, SceDateTime *time_local) {
	SceRtcTick tick;
	sceRtcGetTick(time_local, &tick);
	sceRtcConvertLocalTimeToUtc(&tick, &tick);
	sceRtcSetTick(time_utc, &tick);	
}

void getDateString(char *string, int date_format, SceDateTime *time) {
	SceDateTime time_local;
	convertUtcToLocalTime(&time_local, time);

	switch (date_format) {
		case SCE_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD:
			sprintf(string, "%04d/%02d/%02d", time_local.year, time_local.month, time_local.day);
			break;

		case SCE_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY:
			sprintf(string, "%02d/%02d/%04d", time_local.day, time_local.month, time_local.year);
			break;

		case SCE_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY:
			sprintf(string, "%02d/%02d/%04d", time_local.month, time_local.day, time_local.year);
			break;
	}
}

void getTimeString(char *string, int time_format, SceDateTime *time) {
	SceDateTime time_local;
	convertUtcToLocalTime(&time_local, time);

	switch(time_format) {
		case SCE_SYSTEM_PARAM_TIME_FORMAT_12HR:
			sprintf(string, "%02d:%02d %s", (time_local.hour > 12) ? (time_local.hour - 12) : ((time_local.hour == 0) ? 12 : time_local.hour), time_local.minute, time_local.hour >= 12 ? "PM" : "AM");
			break;

		case SCE_SYSTEM_PARAM_TIME_FORMAT_24HR:
			sprintf(string, "%02d:%02d", time_local.hour, time_local.minute);
			break;
	}
}

int randomNumber(int low, int high) {
   return rand() % (high - low + 1) + low;
}

int debugPrintf(char *text, ...) {
#ifndef RELEASE
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

#ifdef ENABLE_FILE_LOGGING
	SceUID fd = sceIoOpen("ux0:vitashell_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}
#endif

#endif
	return 0;
}

int launchAppByUriExit(char *titleid) {
	char uri[32];
	sprintf(uri, "psgm:play?titleid=%s", titleid);

	sceKernelDelayThread(10000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelDelayThread(10000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);

	sceKernelExitProcess(0);

	return 0;
}


char *strcasestr(const char *haystack, const char *needle) {
	return boyer_moore(haystack, needle); 
}
