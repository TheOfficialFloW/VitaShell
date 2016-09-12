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

#include <psp2/io/fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "player.h"

int MAX_VOLUME_BOOST=15;
int MIN_VOLUME_BOOST=-15;
int MIN_PLAYING_SPEED=-119;
int MAX_PLAYING_SPEED=119;

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

short volume_boost(short *Sample, unsigned int *boost) {
	return 0;
}

int setVolume(int channel, int volume) {
	return 0;
}

int setMute(int channel, int onOff) {
	return 0;
}

void fadeOut(int channel, float seconds) {
	
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