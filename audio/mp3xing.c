//    LightMP3
//    Copyright (C) 2009 Sakya
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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "mp3xing.h"
/*
4-x		    Not used till string "Xing" (58 69 6E 67). This string is used as a main VBR file identifier. If it is not found, file is supposed to be CBR. This string can be placed at different locations according to values of MPEG and CHANNEL (ya, these from a few lines upwards):
36-39		"Xing" for MPEG1 and CHANNEL != mono (mostly used)
21-24		"Xing" for MPEG1 and CHANNEL == mono
21-24		"Xing" for MPEG2 and CHANNEL != mono
13-16		"Xing" for MPEG2 and CHANNEL == mono

After "Xing" string there are placed flags, number of frames in file and a size of file in Bytes. Each of these items has 4 Bytes and it is stored as 'int' number in memory. The first is the most significant Byte and the last is the least.
Following schema is for MPEG1 and CHANNEL != mono:
40-43		Flags
            Value	  	Name	  	Description
            00 00 00 01		Frames Flag		set if value for number of frames in file is stored
            00 00 00 02		Bytes Flag		set if value for filesize in Bytes is stored
            00 00 00 04		TOC Flag		set if values for TOC (see below) are stored
            00 00 00 08		VBR Scale Flag		set if values for VBR scale are stored
            All these values can be stored simultaneously.

44-47		Frames
Number of frames in file (including the first info one)

48-51		Bytes
            File length in Bytes

52-151		TOC (Table of Contents)
            Contains of 100 indexes (one Byte length) for easier lookup in file. Approximately solves problem with moving inside file.
            Each Byte has a value according this formula:
            (TOC[i] / 256) * fileLenInBytes
            So if song lasts eg. 240 sec. and you want to jump to 60. sec. (and file is 5 000 000 Bytes length) you can use:
            TOC[(60/240)*100] = TOC[25]
            and corresponding Byte in file is then approximately at:
            (TOC[25]/256) * 5000000

If you want to trim VBR file you should also reconstruct Frames, Bytes and TOC properly.

152-155		VBR Scale
            I dont know exactly system of storing of this values but this item probably doesnt have deeper meaning.
*/

int xingSearchFrame(unsigned char *buffer, int startPos, int maxSearch)
{
    int i = 0;
    for (i=0; i<maxSearch; i++)
    {
        if(!memcmp(buffer, XING_GUID, 4))
            return startPos;
        startPos++;
        buffer++;
    }
    return -1;
}

int xingGetFlags(unsigned char *buffer, int startPos)
{
    buffer += startPos + 4;
    return buffer[0] + buffer[1] + buffer[2] + buffer[3];
}

int xingGetFrameNumber(unsigned char *buffer, int startPos)
{
    buffer += startPos + 8;
    char hexStr[9] = "";
    sprintf(hexStr, "%2.2X%2.2X%2.2X%2.2X", buffer[0], buffer[1], buffer[2], buffer[3]);
    return strtol(hexStr, NULL, 16);
}

int xingGetFileSize(unsigned char *buffer, int startPos)
{
    buffer += startPos + 12;
    char hexStr[9] = "";
    sprintf(hexStr, "%2.2X%2.2X%2.2X%2.2X", buffer[0], buffer[1], buffer[2], buffer[3]);
    return strtol(hexStr, NULL, 16);
}


int parse_xing(unsigned char *buffer, int startPos, struct xing *xing)
{
    int  pos = xingSearchFrame(buffer, startPos, XING_BUFFER_SIZE - 4);
    if (pos < 0)
        return 0;

    xing->flags = xingGetFlags(buffer, pos);
    if (xing->flags & 1)
        xing->frames = xingGetFrameNumber(buffer, pos);
    if (xing->flags & 2)
        xing->bytes = xingGetFileSize(buffer, pos);

    /*if (xing->flags & 4)
    if (xing->flags & 8)*/
    return 1;
}
