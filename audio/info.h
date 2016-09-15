//    LightMP3
//    Copyright (C) 2007,2008 Sakya
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
#ifndef __info_h
#define __info_h (1)

struct fileInfo{
    int fileType;
    int defaultCPUClock;
    int needsME;
	double fileSize;
	char layer[12];
	int kbit;
	long instantBitrate;
	long hz;
	char mode[52];
	char emphasis[20];
	long length;
	char strLength[12];
	long frames;
	long framesDecoded;
	int  encapsulatedPictureType;
	int  encapsulatedPictureOffset;
	int  encapsulatedPictureLength;
	char coverArtImageName[264];

    //Tag/comments:
	char album[260];
	char title[260];
	char artist[260];
	char genre[260];
    char year[12];
    char trackNumber[8];
};

#endif
