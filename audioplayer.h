/*
	VitaShell
	Copyright (C) 2015-2016

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

#ifndef __AUDIOPLAYER_H__
#define __AUDIOPLAYER_H__

#include <stdbool.h>

typedef struct AudioFileStream {
	char *pNameFile;
	int sizeFile;

	uint8_t *pBuffer;
	int offsetRead;
	int offsetWrite;
	int sizeBuffer;
} AudioFileStream;

typedef struct AudioOutConfig {
	int portId;
	int portType;
	int ch;
	int volume[2];
	int samplingRate;
	int param;
	int grain;
} AudioOutConfig;

extern int audioPlayer(char *file, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos, unsigned int codec_type, int showInterface);

#endif
