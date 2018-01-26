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

#ifndef __UTILS_H__
#define __UTILS_H__

#include "main.h"

#define ALIGN_CENTER(a, b) (((a) - (b)) / 2)
#define ALIGN_RIGHT(x, w) ((x) - (w))

#define ANALOG_CENTER 128
#define ANALOG_THRESHOLD 64
#define ANALOG_SENSITIVITY 16

enum PadButtons {
  PAD_UP,
  PAD_DOWN,
  PAD_LEFT,
  PAD_RIGHT,
  PAD_LTRIGGER,
  PAD_RTRIGGER,
  PAD_TRIANGLE,
  PAD_CIRCLE,
  PAD_CROSS,
  PAD_SQUARE,
  PAD_START,
  PAD_SELECT,
  PAD_ENTER,
  PAD_CANCEL,
  PAD_LEFT_ANALOG_UP,
  PAD_LEFT_ANALOG_DOWN,
  PAD_LEFT_ANALOG_LEFT,
  PAD_LEFT_ANALOG_RIGHT,
  PAD_RIGHT_ANALOG_UP,
  PAD_RIGHT_ANALOG_DOWN,
  PAD_RIGHT_ANALOG_LEFT,
  PAD_RIGHT_ANALOG_RIGHT,
  PAD_N_BUTTONS
};

typedef uint8_t Pad[PAD_N_BUTTONS];

extern SceCtrlData pad;
extern Pad old_pad, current_pad, pressed_pad, released_pad, hold_pad, hold2_pad;
extern Pad hold_count, hold2_count;

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

void setEnterButton(int circle);
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