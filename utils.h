/*
  VitaShell
  Copyright (C) 2015-2017, TheFloW

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

#define ALIGN_CENTER(a, b) (((a)-(b)) / 2)
#define ALIGN_RIGHT(x, w) ((x)-(w))

#define ANALOG_CENTER 128
#define ANALOG_THRESHOLD 64
#define ANALOG_SENSITIVITY 16

enum {
  SCE_CTRL_RIGHT_ANALOG_UP    = 0x00200000,
  SCE_CTRL_RIGHT_ANALOG_RIGHT = 0x00400000,
  SCE_CTRL_RIGHT_ANALOG_DOWN  = 0x00800000,
  SCE_CTRL_RIGHT_ANALOG_LEFT  = 0x01000000,

  SCE_CTRL_LEFT_ANALOG_UP     = 0x02000000,
  SCE_CTRL_LEFT_ANALOG_RIGHT  = 0x04000000,
  SCE_CTRL_LEFT_ANALOG_DOWN   = 0x08000000,
  SCE_CTRL_LEFT_ANALOG_LEFT   = 0x10000000,
/*
  SCE_CTRL_ENTER              = 0x20000000,
  SCE_CTRL_CANCEL             = 0x40000000,
*/
};

extern SceCtrlData pad;
extern uint32_t old_buttons, current_buttons, pressed_buttons, hold_buttons, hold2_buttons, released_buttons;

float easeOut(float x0, float x1, float a, float b);

void startDrawing(vita2d_texture *bg);
void endDrawing();

void closeWaitDialog();

void errorDialog(int error);
void infoDialog(const char *msg, ...);

int checkMemoryCardFreeSpace(const char *path, uint64_t size);

void initPowerTickThread();
void powerLock();
void powerUnlock();

void readPad();
int holdButtons(SceCtrlData *pad, uint32_t buttons, uint64_t time);

int hasEndSlash(const char *path);
int removeEndSlash(char *path);
int addEndSlash(char *path);

void getSizeString(char string[16], uint64_t size);

void convertUtcToLocalTime(SceDateTime *time_local, SceDateTime *time_utc);
void convertLocalTimeToUtc(SceDateTime *time_utc, SceDateTime *time_local);

void getDateString(char string[24], int date_format, SceDateTime *time);
void getTimeString(char string[16], int time_format, SceDateTime *time);

int debugPrintf(const char *text, ...);

int launchAppByUriExit(const char *titleid);

char *strcasestr(const char *haystack, const char *needle);

int vshIoUmount(int id, int a2, int a3, int a4);
int _vshIoMount(int id, const char *path, int permission, void *buf);
int vshIoMount(int id, const char *path, int permission, int a4, int a5, int a6);

void remount(int id);

#endif