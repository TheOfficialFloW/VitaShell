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

#include "main.h"
#include "archive.h"
#include "photo.h"
#include "file.h"
#include "theme.h"
#include "utils.h"

vita2d_texture *loadImage(char *file, int type, char *buffer) {
	vita2d_texture *tex = NULL;

	if (isInArchive()) {
		int size = 0;

        if (isInArchive()) {
			enum FileTypes archiveType = getArchiveType();
			switch(archiveType){
				case FILE_TYPE_ZIP:
					size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE);
					break;
				case FILE_TYPE_RAR:
					size = ReadArchiveRARFile(file,buffer,BIG_BUFFER_SIZE);
					break;
				default:
					size = -1;
					break;
				}
        } else {
            size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
        }

		if (size <= 0) {
			return NULL;
		}

		switch (type) {
			case FILE_TYPE_BMP:
				tex = vita2d_load_BMP_buffer(buffer);
				break;
			
			case FILE_TYPE_PNG:
				tex = vita2d_load_PNG_buffer(buffer);
				break;
				
			case FILE_TYPE_JPEG:
				tex = vita2d_load_JPEG_buffer(buffer, size);
				break;
		}
	} else {
		switch (type) {
			case FILE_TYPE_BMP:
				tex = vita2d_load_BMP_file(file);
				break;
			
			case FILE_TYPE_PNG:
				tex = vita2d_load_PNG_file(file);
				break;
				
			case FILE_TYPE_JPEG:
				tex = vita2d_load_JPEG_file(file);
				break;
		}
	}

	// Set bilinear filter
	if (tex)
		vita2d_texture_set_filters(tex, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);

	return tex;
}

int isHorizontal(float rad) {
	return ((int)sinf(rad) == 0) ? 1 : 0;
}

void photoMode(float *zoom, float width, float height, float rad, int mode) {
	int horizontal = isHorizontal(rad);
	float h = (horizontal ? height : width);
	float w = (horizontal ? width : height);

	switch (mode) {
		case MODE_CUSTOM:
			break;
			
		case MODE_PERFECT: // this is only used for showing image the first time
			if (h > SCREEN_HEIGHT) { // first priority, fit height
				*zoom = SCREEN_HEIGHT / h;
			} else if (w > SCREEN_WIDTH) { // second priority, fit screen
				*zoom = SCREEN_WIDTH / w;
			} else { // otherwise, original size
				*zoom = 1.0f;
			}

			break;
		
		case MODE_ORIGINAL:
			*zoom = 1.0f;
			break;
			
		case MODE_FIT_HEIGHT:
			*zoom = SCREEN_HEIGHT / h;
			break;
			
		case MODE_FIT_WIDTH:
			*zoom = SCREEN_WIDTH / w;
			break;
	}
}

int getNextZoomMode(float *zoom, float width, float height, float rad, int mode) {
	float next_zoom = ZOOM_MAX, smallest_zoom = ZOOM_MAX;;
	int next_mode = MODE_ORIGINAL, smallest_mode = MODE_ORIGINAL;

	int i = 0;
	while (i < 3) {
		if (mode == MODE_CUSTOM || mode == MODE_PERFECT || mode == MODE_FIT_WIDTH) {
			mode = MODE_ORIGINAL;
		} else {
			mode++;
		}

		float new_zoom = 0.0f;
		photoMode(&new_zoom, width, height, rad, mode);

		if (new_zoom < smallest_zoom) {
			smallest_zoom = new_zoom;
			smallest_mode = mode;
		}

		if (new_zoom > *zoom && new_zoom < next_zoom) {
			next_zoom = new_zoom;
			next_mode = mode;
		}

		i++;
	}

	// Get smallest then
	if (next_zoom == ZOOM_MAX) {
		next_zoom = smallest_zoom;
		next_mode = smallest_mode;
	}

	*zoom = next_zoom;
	return next_mode;
}

void resetImageInfo(vita2d_texture *tex, float *width, float *height, float *x, float *y, float *rad, float *zoom, int *mode, uint64_t *time) {
	*width = vita2d_texture_get_width(tex);
	*height = vita2d_texture_get_height(tex);

	*x = *width / 2.0f;
	*y = *height / 2.0f;

	*rad = 0;
	*zoom = 1.0f;

	*mode = MODE_PERFECT;
	photoMode(zoom, *width, *height, *rad, *mode);

	*time = 0;
}

int photoViewer(char *file, int type, FileList *list, FileListEntry *entry, int *base_pos, int *rel_pos) {
	char *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	vita2d_texture *tex = loadImage(file, type, buffer);
	if (!tex) {
		free(buffer);
		return -1;
	}

	// Variables
	float width = 0.0f, height = 0.0f, x = 0.0f, y = 0.0f, rad = 0.0f, zoom = 1.0f;
	int mode = MODE_PERFECT;
	uint64_t time = 0;

	// Reset image
	resetImageInfo(tex, &width, &height, &x, &y, &rad, &zoom, &mode, &time);

	while (1) {
		readPad();

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		// Previous/next image.
		if (pressed_buttons & SCE_CTRL_LEFT || pressed_buttons & SCE_CTRL_RIGHT) {
			int available = 0;

			int old_base_pos = *base_pos;
			int old_rel_pos = *rel_pos;
			FileListEntry *old_entry = entry;

			int previous = pressed_buttons & SCE_CTRL_LEFT;
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
					if (type == FILE_TYPE_BMP || type == FILE_TYPE_JPEG || type == FILE_TYPE_PNG) {
						vita2d_free_texture(tex);
						tex = loadImage(path, type, buffer);
						if (!tex) {
							free(buffer);
							return -1;
						}

						// Reset image
						resetImageInfo(tex, &width, &height, &x, &y, &rad, &zoom, &mode, &time);
						available = 1;
						break;
					}
				}
			}

			if (!available) {
				*base_pos = old_base_pos;
				*rel_pos = old_rel_pos;
				entry = old_entry;
			}
		}

		// Photo mode
		if (pressed_buttons & SCE_CTRL_ENTER) {
			time = sceKernelGetProcessTimeWide();

			x = width / 2.0f;
			y = height / 2.0f;

			// Find next mode
			mode = getNextZoomMode(&zoom, width, height, rad, mode);
		}

		// Rotate
		if (pressed_buttons & SCE_CTRL_LTRIGGER) {
			rad -= M_PI_2;
			if (rad < 0)
				rad += M_TWOPI;

			photoMode(&zoom, width, height, rad, mode);
		} else if (pressed_buttons & SCE_CTRL_RTRIGGER) {
			rad += M_PI_2;
			if (rad >= M_TWOPI)
				rad -= M_TWOPI;

			photoMode(&zoom, width, height, rad, mode);
		}

		// Zoom
		if (current_buttons & SCE_CTRL_RIGHT_ANALOG_DOWN) {
			time = sceKernelGetProcessTimeWide();
			mode = MODE_CUSTOM;
			zoom /= ZOOM_FACTOR;
		} else if (current_buttons & SCE_CTRL_RIGHT_ANALOG_UP) {
			time = sceKernelGetProcessTimeWide();
			mode = MODE_CUSTOM;
			zoom *= ZOOM_FACTOR;
		}

		if (zoom < ZOOM_MIN) {
			zoom = ZOOM_MIN;
		}

		if (zoom > ZOOM_MAX) {
			zoom = ZOOM_MAX;
		}

		// Move
		if (pad.lx < (ANALOG_CENTER - ANALOG_SENSITIVITY) || pad.lx > (ANALOG_CENTER + ANALOG_SENSITIVITY)) {
			float d = ((pad.lx - ANALOG_CENTER) / MOVE_DIVISION) / zoom;

			if (isHorizontal(rad)) {
				x += cosf(rad) * d;
			} else {
				y += -sinf(rad) * d;
			}
		}

		if (pad.ly < (ANALOG_CENTER - ANALOG_SENSITIVITY) || pad.ly > (ANALOG_CENTER + ANALOG_SENSITIVITY)) {
			float d = ((pad.ly - ANALOG_CENTER) / MOVE_DIVISION) / zoom;

			if (isHorizontal(rad)) {
				y += cosf(rad) * d;
			} else {
				x += sinf(rad) * d;
			}
		}

		// Limit
		int horizontal = isHorizontal(rad);
		float w = horizontal ? SCREEN_HALF_WIDTH : SCREEN_HALF_HEIGHT;
		float h = horizontal ? SCREEN_HALF_HEIGHT : SCREEN_HALF_WIDTH;

		if ((zoom * width) > 2.0f * w) {
			if (x < (w / zoom)) {
				x = w / zoom;
			} else if (x > (width - w / zoom)) {
				x = width - w / zoom;
			}
		} else {
			x = width / 2.0f;
		}

		if ((zoom * height) > 2.0f * h) {
			if (y < (h / zoom)) {
				y = h / zoom;
			} else if (y > (height - h / zoom)) {
				y = height - h / zoom;
			}
		} else {
			y = height / 2.0f;
		}

		// Start drawing
		startDrawing(bg_photo_image);

		// Photo
		vita2d_draw_texture_scale_rotate_hotspot(tex, SCREEN_HALF_WIDTH, SCREEN_HALF_HEIGHT, zoom, zoom, rad, x, y);

		// Zoom text
		if ((sceKernelGetProcessTimeWide() - time) < ZOOM_TEXT_TIME)
			pgf_draw_textf(SHELL_MARGIN_X, SCREEN_HEIGHT - 3.0f * SHELL_MARGIN_Y, PHOTO_ZOOM_COLOR, FONT_SIZE, "%.0f%%", zoom * 100.0f);

		// End drawing
		endDrawing();
	}

	vita2d_free_texture(tex);

	free(buffer);

	return 0;
}