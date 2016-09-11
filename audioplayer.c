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

#include "main.h"
#include "archive.h"
#include "audioplayer.h"
#include "file.h"
#include "theme.h"
#include "utils.h"

#include "audio/id3.h"
#include "audio/mp3player.h"

int audioPlayer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos) {
	struct ID3Tag id3tag;
	ParseID3(file, &id3tag);

	MP3_Init(0);
	MP3_Load(file);
	MP3_Play();

	while (1) {
		readPad();

		if (MP3_EndOfStream())
			break;

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		// Start drawing
		startDrawing(NULL);

		char string[32];
		MP3_GetTimeString(string);
		pgf_draw_textf(SHELL_MARGIN_X, SCREEN_HEIGHT - 3.0f * SHELL_MARGIN_Y, PHOTO_ZOOM_COLOR, FONT_SIZE, string);

		// End drawing
		endDrawing();
	}

	MP3_Stop();

	return 0;
}