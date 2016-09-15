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
extern int OGG_defaultCPUClock;

//private functions
void OGG_Init(int channel);
int OGG_IsPlaying();
int OGG_Play();
void OGG_Pause();
int OGG_Stop();
void OGG_End();
void OGG_FreeTune();
int OGG_Load(char *filename);
void OGG_GetTimeString(char *dest);
int OGG_EndOfStream();
struct fileInfo *OGG_GetInfo();
struct fileInfo OGG_GetTagInfoOnly(char *filename);
int OGG_GetStatus();
float OGG_GetPercentage();
void OGG_setVolumeBoostType(char *boostType);
void OGG_setVolumeBoost(int boost);
int OGG_getVolumeBoost();
int OGG_getPlayingSpeed();
int OGG_setPlayingSpeed(int playingSpeed);
int OGG_setMute(int onOff);
void OGG_fadeOut(float seconds);

//Functions for filter (equalizer):
int OGG_setFilter(double tFilter[32], int copyFilter);
void OGG_enableFilter();
void OGG_disableFilter();
int OGG_isFilterEnabled();
int OGG_isFilterSupported();

//Manage suspend:
int OGG_suspend();
int OGG_resume();

double OGG_getFilePosition();
void OGG_setFilePosition(double position);
