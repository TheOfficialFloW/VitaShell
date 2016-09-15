//    LightMP3
//    Copyright (C) 2007 Sakya
//    sakya_tg@yahoo.it
//
//    This program is free software; you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation; either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program; if not, write to the Free Software
//    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
#include "../libmad/mad.h"

extern int MP3_defaultCPUClock;

//private functions
void MP3_Init(int channel);
int MP3_IsPlaying();
int MP3_Play();
void MP3_Pause();
int MP3_Stop();
void MP3_End();
void MP3_FreeTune();
int MP3_Load(char *filename);
void MP3_GetTimeString(char *dest);
int MP3_EndOfStream();
struct fileInfo *MP3_GetInfo();
struct fileInfo MP3_GetTagInfoOnly(char *filename);
int MP3_GetStatus();
float MP3_GetPercentage();
void MP3_setVolumeBoostType(char *boostType);
void MP3_setVolumeBoost(int boost);
int MP3_getVolumeBoost();
int MP3_getPlayingSpeed();
int MP3_setPlayingSpeed(int playingSpeed);
int MP3_setMute(int onOff);
void MP3_fadeOut(float seconds);

//Functions for filter (equalizer):
int MP3_setFilter(double tFilter[32], int copyFilter);
void MP3_enableFilter();
void MP3_disableFilter();
int MP3_isFilterEnabled();
int MP3_isFilterSupported();

//Manage suspend:
int MP3_suspend();
int MP3_resume();

double MP3_getFilePosition();
void MP3_setFilePosition(double position);
