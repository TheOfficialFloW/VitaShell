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

#ifndef __PLAYER_H__
#define __PLAYER_H__

#include "id3.h"
#include "info.h"
#include "vita_audio.h"

#define OPENING_OK 0
#define ERROR_OPENING -1
#define ERROR_INVALID_SAMPLE_RATE -2
#define ERROR_MEMORY -3
#define ERROR_CREATE_THREAD -4

#define MP3_TYPE 0
#define OGG_TYPE 1
#define UNK_TYPE -1

#define FASTFORWARD_VOLUME 0 // 0x2200

extern int MAX_VOLUME_BOOST;
extern int MIN_VOLUME_BOOST;
extern int MIN_PLAYING_SPEED;
extern int MAX_PLAYING_SPEED;

int SeekNextFrameMP3(SceUID fd);

short volume_boost(short *Sample, unsigned int *boost);
int setVolume(int channel, int volume);
int setMute(int channel, int onOff);
void fadeOut(int channel, float seconds);

int initAudioLib();
int endAudioLib();

void initFileInfo(struct fileInfo *info);

extern void (* initFunct)(int);
extern int (* loadFunct)(char *);
extern int (* isPlayingFunct)();
extern int (* playFunct)();
extern void (* pauseFunct)();
extern void (* endFunct)();
extern void (* setVolumeBoostTypeFunct)(char*);
extern void (* setVolumeBoostFunct)(int);
extern struct fileInfo *(* getInfoFunct)();
extern struct fileInfo (* getTagInfoFunct)();
extern void (* getTimeStringFunct)();
extern float (* getPercentageFunct)();
extern int (* getPlayingSpeedFunct)();
extern int (* setPlayingSpeedFunct)(int);
extern int (* endOfStreamFunct)();

extern int (* setMuteFunct)(int);
extern int (* setFilterFunct)(double[32], int copyFilter);
extern void (* enableFilterFunct)();
extern void (* disableFilterFunct)();
extern int (* isFilterEnabledFunct)();
extern int (* isFilterSupportedFunct)();
extern int (* suspendFunct)();
extern int (* resumeFunct)();
extern void (* fadeOutFunct)(float seconds);

extern double (* getFilePositionFunct)();                     //Gets current file position in bytes
extern void (* setFilePositionFunct)(double position);          //Set current file position in butes

extern int setAudioFunctions(int type);
extern void unsetAudioFunctions();

#endif