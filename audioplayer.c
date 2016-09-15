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
#include "language.h"
#include "utils.h"

#include "audio/player.h"

static char title[128], album[128], artist[128], genre[128], year[12];
static struct fileInfo *fileinfo = NULL;
static vita2d_texture *tex = NULL;

void shortenString(char *out, char *in, int width) {
	strcpy(out, in);

	int i;
	for (i = strlen(out) - 1; i > 0; i--) {
		if (vita2d_pgf_text_width(font, FONT_SIZE, out) < width)
			break;

		out[i] = '\0';
	}
}

vita2d_texture *getAlternativeCoverImage(char *file) {
	SceIoStat stat;
	char path[128];

	char *p = strrchr(file, '/');
	if (p) {
		*p = '\0';

		sprintf(path, "%s/cover.jpg", file);
		memset(&stat, 0, sizeof(SceIoStat));
		if (sceIoGetstat(path, &stat) >= 0) {
			*p = '/';
			return vita2d_load_JPEG_file(path);
		}

		sprintf(path, "%s/folder.jpg", file);
		memset(&stat, 0, sizeof(SceIoStat));
		if (sceIoGetstat(path, &stat) >= 0) {
			*p = '/';
			return vita2d_load_JPEG_file(path);
		}

		*p = '/';
	}

	return NULL;
}

void getAudioInfo(char *file) {
	char *buffer = NULL;

	fileinfo = getInfoFunct();

	shortenString(title, fileinfo->title[0] == '\0' ? "-" : fileinfo->title, 390);
	shortenString(album, fileinfo->album[0] == '\0' ? "-" : fileinfo->album, 390);
	shortenString(artist, fileinfo->artist[0] == '\0' ? "-" : fileinfo->artist, 390);
	shortenString(genre, fileinfo->genre[0] == '\0' ? "-" : fileinfo->genre, 390);
	shortenString(year, fileinfo->year[0] == '\0' ? "-" : fileinfo->year, 390);

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

					if (tex)
						vita2d_texture_set_filters(tex, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);

					free(buffer);
				}

				break;
			}
		}
	}

	if (!tex)
		tex = getAlternativeCoverImage(file);
}

int audioPlayer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos) {
	static int speed_list[] = { -7, -3, -1, 0, 1, 3, 7 };
	#define N_SPEED (sizeof(speed_list) / sizeof(int))

	sceAppMgrAcquireBgmPort();

	powerLock();

	setAudioFunctions(type);

	initFunct(0);
	loadFunct(file);
	playFunct();

	getAudioInfo(file);

	while (1) {
		char cur_time_string[12];
		getTimeStringFunct(cur_time_string);

		readPad();

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		// Display off
		if (pressed_buttons & SCE_CTRL_TRIANGLE) {
			scePowerRequestDisplayOff();
		}

		// Toggle play/pause
		if (pressed_buttons & SCE_CTRL_ENTER) {
			if (isPlayingFunct() && getPlayingSpeedFunct() == 0) {
				pauseFunct();
			} else {
				setPlayingSpeedFunct(0);
				playFunct();
			}
		}

		if (pressed_buttons & SCE_CTRL_LEFT || pressed_buttons & SCE_CTRL_RIGHT) {
			int speed = getPlayingSpeedFunct();

			if (pressed_buttons & SCE_CTRL_LEFT) {
				int i;
				for (i = 0; i < N_SPEED; i++) {
					if (speed_list[i] == speed) {
						if (i > 0)
							speed = speed_list[i - 1];
						break;
					}
				}
			}

			if (pressed_buttons & SCE_CTRL_RIGHT) {
				int i;
				for (i = 0; i < N_SPEED; i++) {
					if (speed_list[i] == speed) {
						if (i < N_SPEED - 1)
							speed = speed_list[i + 1];
						break;
					}
				}
			}

			setPlayingSpeedFunct(speed);

			playFunct();
		}

		// Previous/next song.
		if (getPercentageFunct() == 100.0f || endOfStreamFunct() || pressed_buttons & SCE_CTRL_LTRIGGER || pressed_buttons & SCE_CTRL_RTRIGGER) {
			int previous = pressed_buttons & SCE_CTRL_LTRIGGER;
			if (previous && strcmp(cur_time_string, "00:00:00") != 0) {
				endFunct();
				initFunct(0);
				loadFunct(file);
				playFunct();

				getAudioInfo(file);
			} else {
				int available = 0;

				int old_base_pos = *base_pos;
				int old_rel_pos = *rel_pos;
				FileListEntry *old_entry = entry;

				if (getPercentageFunct() == 100.0f && !endOfStreamFunct())
					previous = 1;

				if (endOfStreamFunct())
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
						if (type == FILE_TYPE_MP3 || type == FILE_TYPE_OGG) {
							file = path;

							endFunct();

							setAudioFunctions(type);

							initFunct(0);
							loadFunct(file);
							playFunct();

							getAudioInfo(file);

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
		}

		// Start drawing
		startDrawing(bg_audio_image);

		// Draw shell info
		drawShellInfo(file);

		float cover_size = MAX_ENTRIES * FONT_Y_SPACE;

		// Cover
		if (tex) {
			vita2d_draw_texture_scale(tex, SHELL_MARGIN_X, START_Y, cover_size / vita2d_texture_get_width(tex), cover_size / vita2d_texture_get_height(tex));
		} else {
			vita2d_draw_texture(cover_image, SHELL_MARGIN_X, START_Y);
		}

		// Info
		float x = 2.0f * SHELL_MARGIN_X + cover_size;

		pgf_draw_text(x, START_Y + (0 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, FONT_SIZE, language_container[TITLE]);
		pgf_draw_text(x, START_Y + (1 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, FONT_SIZE, language_container[ALBUM]);
		pgf_draw_text(x, START_Y + (2 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, FONT_SIZE, language_container[ARTIST]);
		pgf_draw_text(x, START_Y + (3 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, FONT_SIZE, language_container[GENRE]);
		pgf_draw_text(x, START_Y + (4 * FONT_Y_SPACE), AUDIO_INFO_ASSIGN, FONT_SIZE, language_container[YEAR]);

		x += 120.0f;

		pgf_draw_text(x, START_Y + (0 * FONT_Y_SPACE), AUDIO_INFO, FONT_SIZE, title);
		pgf_draw_text(x, START_Y + (1 * FONT_Y_SPACE), AUDIO_INFO, FONT_SIZE, album);
		pgf_draw_text(x, START_Y + (2 * FONT_Y_SPACE), AUDIO_INFO, FONT_SIZE, artist);
		pgf_draw_text(x, START_Y + (3 * FONT_Y_SPACE), AUDIO_INFO, FONT_SIZE, genre);
		pgf_draw_text(x, START_Y + (4 * FONT_Y_SPACE), AUDIO_INFO, FONT_SIZE, year);

		x -= 120.0f;

		float y = SCREEN_HEIGHT - 6.0f * SHELL_MARGIN_Y;

		// Icon
		vita2d_texture *icon = NULL;

		if (getPlayingSpeedFunct() != 0) {
			if (getPlayingSpeedFunct() < 0) {
				icon = fastrewind_image;
			} else {
				icon = fastforward_image;
			}

			pgf_draw_textf(x + 45.0f, y, AUDIO_SPEED, FONT_SIZE, "%dx", abs(getPlayingSpeedFunct() + (getPlayingSpeedFunct() < 0 ? -1 : 1)));
		} else {
			if (isPlayingFunct()) {
				icon = pause_image;
			} else {
				icon = play_image;
			}
		}

		vita2d_draw_texture(icon, x, y + 3.0f);

		// Time
		char string[32];
		sprintf(string, "%s / %s", cur_time_string, fileinfo->strLength);
		float time_x = ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, string));

		int w = pgf_draw_text(time_x, y, AUDIO_TIME_CURRENT, FONT_SIZE, cur_time_string);
		pgf_draw_text(time_x + (float)w, y, AUDIO_TIME_SLASH, FONT_SIZE, " /");
		pgf_draw_text(ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, fileinfo->strLength)), y, AUDIO_TIME_TOTAL, FONT_SIZE, fileinfo->strLength);

		float width = SCREEN_WIDTH - 3.0f * SHELL_MARGIN_X - cover_size;
		vita2d_draw_rectangle(x, (y) + FONT_Y_SPACE + 10.0f, width, 8, AUDIO_TIME_BAR_BG);
		vita2d_draw_rectangle(x, (y) + FONT_Y_SPACE + 10.0f, getPercentageFunct() * width / 100.0f, 8, AUDIO_TIME_BAR);

		// End drawing
		endDrawing();
	}

	if (tex) {
		vita2d_free_texture(tex);
		tex = NULL;
	}

	endFunct();

	powerUnlock();

	sceAppMgrReleaseBgmPort();

	return 0;
}