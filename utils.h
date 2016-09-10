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

#ifndef __UTILS_H__
#define __UTILS_H__

#include "main.h"

#define ALIGN_CENTER(a, b) ((a - b) / 2)
#define ALIGN_LEFT(x, w) (x - w)

#define ANALOG_CENTER 128
#define ANALOG_THRESHOLD 64
#define ANALOG_SENSITIVITY 16

enum {
	SCE_CTRL_RIGHT_ANALOG_UP	= 0x0020000,
	SCE_CTRL_RIGHT_ANALOG_RIGHT	= 0x0040000,
	SCE_CTRL_RIGHT_ANALOG_DOWN	= 0x0080000,
	SCE_CTRL_RIGHT_ANALOG_LEFT	= 0x0100000,

	SCE_CTRL_LEFT_ANALOG_UP		= 0x0200000,
	SCE_CTRL_LEFT_ANALOG_RIGHT	= 0x0400000,
	SCE_CTRL_LEFT_ANALOG_DOWN	= 0x0800000,
	SCE_CTRL_LEFT_ANALOG_LEFT	= 0x1000000,
/*
	SCE_CTRL_ENTER				= 0x2000000,
	SCE_CTRL_CANCEL				= 0x4000000,
*/
};

extern SceCtrlData pad;
extern uint32_t old_buttons, current_buttons, pressed_buttons, hold_buttons, hold2_buttons, released_buttons;

void startDrawing(vita2d_texture *bg);
void endDrawing();

void errorDialog(int error);
void infoDialog(char *msg, ...);

void initPowerTickThread();
void powerLock();
void powerUnlock();

void readPad();
int holdButtons(SceCtrlData *pad, uint32_t buttons, uint64_t time);

int hasEndSlash(char *path);
int removeEndSlash(char *path);
int addEndSlash(char *path);

void getSizeString(char *string, uint64_t size);
void getDateString(char *string, int date_format, SceDateTime *time);
void getTimeString(char *string, int time_format, SceDateTime *time);

int randomNumber(int low, int high);

int debugPrintf(char *text, ...);

int launchAppByUriExit(char *titleid);

char *strcasestr(const char *haystack, const char *needle);

#endif
