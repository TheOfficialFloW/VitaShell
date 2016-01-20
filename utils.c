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
#include "language.h"
#include "utils.h"

uint32_t old_buttons, current_buttons, pressed_buttons, hold_buttons, hold2_buttons, released_buttons;

void errorDialog(int error) {
	initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[ERROR], error);
	dialog_step = DIALOG_STEP_ERROR;
}

void infoDialog(char *string) {
	initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, string);
	dialog_step = DIALOG_STEP_INFO;
}

int debugPrintf(char *text, ...) {
#ifndef RELEASE
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	uvl_log_write(string, strlen(string));

	SceUID fd = sceIoOpen("cache0:vitashell_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}
#endif

	return 0;
}

void disableAutoSuspend() {
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);	
}

void readPad() {
	static int hold_n = 0, hold2_n = 0;

	SceCtrlData pad;
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

	if (path[len - 1] != '/') {
		strcat(path, "/");
		return 1;
	}

	return 0;
}

void getSizeString(char *string, uint64_t size) {
	if (size >= 1024 * 1024 * 1024) {
		sprintf(string, "%.2f GB", (double)size / (double)(1024 * 1024 * 1024));
	} else if (size >= 1024 * 1024) {
		sprintf(string, "%.2f MB", (double)size / (double)(1024 * 1024));
	} else if(size >= 1024) {
		sprintf(string, "%.2f KB", (double)size / (double)(1024));
	} else {
		sprintf(string, "%.0f B", (double)size);
	}
}

void getDateString(char *string, int date_format, SceRtcTime *time) {
	switch (date_format) {
		case SCE_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD:
			sprintf(string, "%02d/%02d/%02d", time->year, time->month, time->day);
			break;
			
		case SCE_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY:
			sprintf(string, "%02d/%02d/%02d", time->day, time->month, time->year);
			break;
			
		case SCE_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY:
			sprintf(string, "%02d/%02d/%02d", time->month, time->day, time->year);
			break;
	}
}

void getTimeString(char *string, int time_format, SceRtcTime *time) {
	switch(time_format) {
		case SCE_SYSTEM_PARAM_TIME_FORMAT_12HR:
			sprintf(string, "%02d:%02d %s", time->hour >= 12 ? time->hour - 12 : time->hour, time->minutes, time->hour >= 12 ? "PM" : "AM");
			break;
			
		case SCE_SYSTEM_PARAM_TIME_FORMAT_24HR:
			sprintf(string, "%02d:%02d", time->hour, time->minutes);
			break;
	}
}