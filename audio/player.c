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

#include <psp2/io/fcntl.h>
#include <psp2/kernel/threadmgr.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "player.h"
#include "mp3player.h"
#include "oggplayer.h"
#include "vita_audio.h"

#include "../file.h"

int MUTED_VOLUME = 800;
int MAX_VOLUME_BOOST = 15;
int MIN_VOLUME_BOOST = -15;
int MIN_PLAYING_SPEED = -119;
int MAX_PLAYING_SPEED = 119;
int currentVolume = 0;

void (* initFunct)(int);
int (* isPlayingFunct)();
int (* loadFunct)(char *);
int (* playFunct)();
void (* pauseFunct)();
void (* endFunct)();
void (* setVolumeBoostTypeFunct)(char*);
void (* setVolumeBoostFunct)(int);
struct fileInfo *(* getInfoFunct)();
struct fileInfo (* getTagInfoFunct)();
void (* getTimeStringFunct)();
float (* getPercentageFunct)();
int (* getPlayingSpeedFunct)();
int (* setPlayingSpeedFunct)(int);
int (* endOfStreamFunct)();

int (* setMuteFunct)(int);
int (* setFilterFunct)(double[32], int copyFilter);
void (* enableFilterFunct)();
void (* disableFilterFunct)();
int (* isFilterEnabledFunct)();
int (* isFilterSupportedFunct)();
int (* suspendFunct)();
int (* resumeFunct)();
void (* fadeOutFunct)(float seconds);

double (* getFilePositionFunct)();
void (* setFilePositionFunct)(double positionInSecs);

//Seek next valid frame
//NOTE: this function comes from Music prx 0.55 source
//      all credits goes to joek2100.
int SeekNextFrameMP3(SceUID fd)
{
    int offset = 0;
    unsigned char buf[1024];
    unsigned char *pBuffer;
    int i;
    int size = 0;

    offset = sceIoLseek32(fd, 0, SCE_SEEK_CUR);
    sceIoRead(fd, buf, sizeof(buf));
    if (!strncmp((char*)buf, "ID3", 3) || !strncmp((char*)buf, "ea3", 3)) //skip past id3v2 header, which can cause a false sync to be found
    {
        //get the real size from the syncsafe int
        size = buf[6];
        size = (size<<7) | buf[7];
        size = (size<<7) | buf[8];
        size = (size<<7) | buf[9];

        size += 10;

        if (buf[5] & 0x10) //has footer
            size += 10;
    }

    sceIoLseek32(fd, offset, SCE_SEEK_SET); //now seek for a sync
    while(1)
    {
        offset = sceIoLseek32(fd, 0, SCE_SEEK_CUR);
        size = sceIoRead(fd, buf, sizeof(buf));

        if (size <= 2)//at end of file
            return -1;

        if (!strncmp((char*)buf, "EA3", 3))//oma mp3 files have non-safe ints in the EA3 header
        {
            sceIoLseek32(fd, (buf[4]<<8)+buf[5], SCE_SEEK_CUR);
            continue;
        }

        pBuffer = buf;
        for( i = 0; i < size; i++)
        {
            //if this is a valid frame sync (0xe0 is for mpeg version 2.5,2+1)
            if ( (pBuffer[i] == 0xff) && ((pBuffer[i+1] & 0xE0) == 0xE0))
            {
                offset += i;
                sceIoLseek32(fd, offset, SCE_SEEK_SET);
                return offset;
            }
        }
       //go back two bytes to catch any syncs that on the boundary
        sceIoLseek32(fd, -2, SCE_SEEK_CUR);
    }
}

int setAudioFunctions(int type) {
	if (type == FILE_TYPE_OGG) {
        //OGG Vorbis
		initFunct = OGG_Init;
		loadFunct = OGG_Load;
		isPlayingFunct = OGG_IsPlaying;
		playFunct = OGG_Play;
		pauseFunct = OGG_Pause;
		endFunct = OGG_End;
        setVolumeBoostTypeFunct = OGG_setVolumeBoostType;
        setVolumeBoostFunct = OGG_setVolumeBoost;
        getInfoFunct = OGG_GetInfo;
        getTagInfoFunct = OGG_GetTagInfoOnly;
        getTimeStringFunct = OGG_GetTimeString;
        getPercentageFunct = OGG_GetPercentage;
        getPlayingSpeedFunct = OGG_getPlayingSpeed;
        setPlayingSpeedFunct = OGG_setPlayingSpeed;
        endOfStreamFunct = OGG_EndOfStream;

        setMuteFunct = OGG_setMute;
        setFilterFunct = OGG_setFilter;
        enableFilterFunct = OGG_enableFilter;
        disableFilterFunct = OGG_disableFilter;
        isFilterEnabledFunct = OGG_isFilterEnabled;
        isFilterSupportedFunct = OGG_isFilterSupported;

        suspendFunct = OGG_suspend;
        resumeFunct = OGG_resume;
        fadeOutFunct = OGG_fadeOut;

        getFilePositionFunct = OGG_getFilePosition;
        setFilePositionFunct = OGG_setFilePosition;
		return 0;
    } else if (type == FILE_TYPE_MP3) {
		initFunct = MP3_Init;
		loadFunct = MP3_Load;
		isPlayingFunct = MP3_IsPlaying;
		playFunct = MP3_Play;
		pauseFunct = MP3_Pause;
		endFunct = MP3_End;
		setVolumeBoostTypeFunct = MP3_setVolumeBoostType;
		setVolumeBoostFunct = MP3_setVolumeBoost;
		getInfoFunct = MP3_GetInfo;
		getTagInfoFunct = MP3_GetTagInfoOnly;
		getTimeStringFunct = MP3_GetTimeString;
		getPercentageFunct = MP3_GetPercentage;
		getPlayingSpeedFunct = MP3_getPlayingSpeed;
		setPlayingSpeedFunct = MP3_setPlayingSpeed;
		endOfStreamFunct = MP3_EndOfStream;

		setMuteFunct = MP3_setMute;
		setFilterFunct = MP3_setFilter;
		enableFilterFunct = MP3_enableFilter;
		disableFilterFunct = MP3_disableFilter;
		isFilterEnabledFunct = MP3_isFilterEnabled;
		isFilterSupportedFunct = MP3_isFilterSupported;

		suspendFunct = MP3_suspend;
		resumeFunct = MP3_resume;
		fadeOutFunct = MP3_fadeOut;

		getFilePositionFunct = MP3_getFilePosition;
		setFilePositionFunct = MP3_setFilePosition;

		return 0;
    }

	return -1;
}

void unsetAudioFunctions() {
    initFunct = NULL;
    loadFunct = NULL;
    playFunct = NULL;
    pauseFunct = NULL;
    endFunct = NULL;
    setVolumeBoostTypeFunct = NULL;
    setVolumeBoostFunct = NULL;
    getInfoFunct = NULL;
    getTagInfoFunct = NULL;
    getTimeStringFunct = NULL;
    getPercentageFunct = NULL;
    getPlayingSpeedFunct = NULL;
    setPlayingSpeedFunct = NULL;
    endOfStreamFunct = NULL;

    setMuteFunct = NULL;
    setFilterFunct = NULL;
    enableFilterFunct = NULL;
    disableFilterFunct = NULL;
    isFilterEnabledFunct = NULL;
    isFilterSupportedFunct = NULL;

    suspendFunct = NULL;
    resumeFunct = NULL;

    getFilePositionFunct = NULL;
    setFilePositionFunct = NULL;
}

short volume_boost(short *Sample, unsigned int *boost) {
	int intSample = *Sample * (*boost + 1);
	if (intSample > 32767)
		return 32767;
	else if (intSample < -32768)
		return -32768;
	else
    	return intSample;
}

int setVolume(int channel, int volume) {
	vitaAudioSetVolume(channel, volume, volume);
	return 0;
}

int setMute(int channel, int onOff) {
	if (onOff)
        setVolume(channel, MUTED_VOLUME);
	else
        setVolume(channel, VITA_VOLUME_MAX);
	return 0;
}

void fadeOut(int channel, float seconds) {
    int i = 0;
    long timeToWait = (long)((seconds * 1000.0) / (float)currentVolume);
    for (i=currentVolume; i>=0; i--){
        vitaAudioSetVolume(channel, i, i);
        sceKernelDelayThread(timeToWait);
    }
}

int initAudioLib() {
	return 0;
}

int endAudioLib() {
	return 0;
}

void initFileInfo(struct fileInfo *info){
    info->fileType = -1;
    info->defaultCPUClock = 0;
    info->needsME = 0;
    info->fileSize = 0;
    strcpy(info->layer, "");
    info->kbit = 0;
    info->instantBitrate = 0;
    info->hz = 0;
    strcpy(info->mode, "");
    strcpy(info->emphasis, "");
    info->length = 0;
    strcpy(info->strLength, "");
    info->frames = 0;
    info->framesDecoded = 0;
    info->encapsulatedPictureType = 0;
    info->encapsulatedPictureOffset = 0;
    info->encapsulatedPictureLength = 0;

    strcpy(info->album, "");
    strcpy(info->title, "");
    strcpy(info->artist, "");
    strcpy(info->genre, "");
    strcpy(info->year, "");
    strcpy(info->trackNumber, "");
    strcpy(info->coverArtImageName, "");
}