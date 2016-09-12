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
#include "audio/info.h"
#include "audio/mp3player.h"

struct fileInfo *fileinfo = NULL;
vita2d_texture *tex = NULL;

void getMp3Info(char *file) {
	char *buffer = NULL;

	fileinfo = MP3_GetInfo();

	if (tex) {
		vita2d_free_texture(tex);
		tex = NULL;
	}

	switch (fileinfo->encapsulatedPictureType) {
		case JPEG_IMAGE:
		case PNG_IMAGE:
		{
			SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
			if (fd >= 0) {
				char *buffer = malloc(fileinfo->encapsulatedPictureLength);
				if (buffer) {
					sceIoLseek32(fd, fileinfo->encapsulatedPictureOffset, SCE_SEEK_SET);
					sceIoRead(fd, buffer, fileinfo->encapsulatedPictureLength);
					sceIoClose(fd);

					if (fileinfo->encapsulatedPictureType == JPEG_IMAGE)
						tex = vita2d_load_JPEG_buffer(buffer, fileinfo->encapsulatedPictureLength);

					if (fileinfo->encapsulatedPictureType == PNG_IMAGE)
						tex = vita2d_load_PNG_buffer(buffer);

					free(buffer);
				}

				break;
			}
		}
	}
}

int audioPlayer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos) {
	MP3_Init(0);
	MP3_Load(file);
	MP3_Play();

	getMp3Info(file);

	while (1) {
		readPad();

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		// Previous/next song.
		if (MP3_EndOfStream() || pressed_buttons & SCE_CTRL_LTRIGGER || pressed_buttons & SCE_CTRL_RTRIGGER) {
			int available = 0;

			int old_base_pos = *base_pos;
			int old_rel_pos = *rel_pos;
			FileListEntry *old_entry = entry;

			int previous = pressed_buttons & SCE_CTRL_LTRIGGER;
			if (MP3_EndOfStream())
				previous = 0;

			while (previous ? entry->previous : entry->next) {
				entry = previous ? entry->previous : entry->next;

				if (previous) {
					if (*rel_pos > 0) {
						(*rel_pos)--;
					} else {
						if (*base_pos > 0) {
							(*base_pos)--;
						}
					}
				} else {
					if ((*rel_pos + 1) < list->length) {
						if ((*rel_pos + 1) < MAX_POSITION) {
							(*rel_pos)++;
						} else {
							if ((*base_pos + *rel_pos + 1) < list->length) {
								(*base_pos)++;
							}
						}
					}
				}

				if (!entry->is_folder) {
					char path[MAX_PATH_LENGTH];
					snprintf(path, MAX_PATH_LENGTH, "%s%s", list->path, entry->name);
					int type = getFileType(path);
					if (type == FILE_TYPE_MP3) {
						MP3_End();
						MP3_Init(0);
						MP3_Load(path);
						MP3_Play();
						getMp3Info(path);
						available = 1;
						break;
					}
				}
			}

			if (!available) {
				*base_pos = old_base_pos;
				*rel_pos = old_rel_pos;
				entry = old_entry;
				break;
			}
		}

		// Start drawing
		startDrawing(NULL);

		// Draw shell info
		drawShellInfo(file);

		pgf_draw_textf(SHELL_MARGIN_X, START_Y + (0 * FONT_Y_SPACE), 0xFFFFFFFF, FONT_SIZE, fileinfo->artist);
		pgf_draw_textf(SHELL_MARGIN_X, START_Y + (1 * FONT_Y_SPACE), 0xFFFFFFFF, FONT_SIZE, fileinfo->title);
		pgf_draw_textf(SHELL_MARGIN_X, START_Y + (2 * FONT_Y_SPACE), 0xFFFFFFFF, FONT_SIZE, fileinfo->album);

		// Picture
		if (tex)
			vita2d_draw_texture_scale(tex, SHELL_MARGIN_X, 200.0f, 1.0f, 1.0f);

		// Time
		char string[12];
		MP3_GetTimeString(string);

		pgf_draw_textf(SHELL_MARGIN_X, SCREEN_HEIGHT - 3.0f * SHELL_MARGIN_Y, PHOTO_ZOOM_COLOR, FONT_SIZE, "%s/%s", string, fileinfo->strLength);
/*
		//float percent = MP3_GetPercentage();
		float width = uncommon_dialog.width - 2.0f * SHELL_MARGIN_X;
		vita2d_draw_rectangle(uncommon_dialog.x + SHELL_MARGIN_X, string_y + 10.0f, width, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_BG_COLOR);
		vita2d_draw_rectangle(uncommon_dialog.x + SHELL_MARGIN_X, string_y + 10.0f, uncommon_dialog.progress * width / 100.0f, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_COLOR);
*/
		// End drawing
		endDrawing();
	}

	if (tex) {
		vita2d_free_texture(tex);
		tex = NULL;
	}

	MP3_End();

	return 0;
}