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

#ifndef __LRCPARSE_H__
#define __LRCPARSE_H__

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <psp2/types.h>
#include <onigmoposix.h>
#include "../file.h"

#define MAX_LYRICSLINE 512

/** struct for a line lyrics */
typedef struct {
    uint64_t totalms;///< total millisecond
    uint32_t m; ///< minute
    uint8_t s; ///< second
    uint8_t ms; ///< millisecond
    char* word; ///< lyrics words
}Lyricsline;

/** struct for lyrics */
typedef struct{
    size_t lyricscount;///< lyrics line count
    Lyricsline* lrclines;///< lyrics line
}Lyrics;



/**
* init from lrc file path
* @param[in] lrcfilepath lrc file path
* @return lyrics struct
* @ref file.h
* @see
* @note
*/
Lyrics* lrcParseLoadWithFile(char* lrcfilepath);

/**
* init from buffer
* @param[in] buffer lrcbuffer
* @return lyrics struct
* @see
* @note
*/
Lyrics* lrcParseLoadWithBuffer(char* buffer);

/**
* close lrcparse
* @param[in] lyrics
* @see
* @note You must use this function to free lrcparse malloc memory!
*/
void lrcParseClose(Lyrics* lyrics);


#endif
