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
#include "photo.h"
#include "file.h"
#include "utils.h"

int photoViewer(char *file, int type) {
	vita2d_texture *tex = NULL;

	char *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	int size = 0;

	if (isInArchive()) {
		size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE);
	} else {
		size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
	}

	if (size <= 0) {
		free(buffer);
		return size;
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

	if (!tex) {
		free(buffer);
		return -1;
	}

	// Set bilinear filter
	vita2d_texture_set_filters(tex, SCE_GXM_TEXTURE_FILTER_LINEAR, SCE_GXM_TEXTURE_FILTER_LINEAR);

	float scale = 1.0f;

	// Adjust to screen
	float width = vita2d_texture_get_width(tex);
	float height = vita2d_texture_get_height(tex);

	if (height > SCREEN_HEIGHT) { // height is first priority
		scale = SCREEN_HEIGHT / height;
	} else if (width > SCREEN_WIDTH) { // width is second priority
		scale = SCREEN_WIDTH / width;
	}

	while (1) {
		readPad();

		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		if (hold2_buttons & SCE_CTRL_LTRIGGER) {
			scale /= ZOOM_FACTOR;
		}

		if (hold2_buttons & SCE_CTRL_RTRIGGER) {
			scale *= ZOOM_FACTOR;
		}

		if (scale < ZOOM_MIN) {
			scale = ZOOM_MIN;
		}

		if (scale > ZOOM_MAX) {
			scale = ZOOM_MAX;
		}

		// TODO: move with analog

		// Start drawing
		START_DRAWING();

		vita2d_draw_texture_scale_rotate_hotspot(tex, SCREEN_WIDTH / 2.0f, SCREEN_HEIGHT / 2.0f, scale, scale, 0.0f, vita2d_texture_get_width(tex) / 2.0f, vita2d_texture_get_height(tex) / 2.0f);

		// End drawing
		END_DRAWING();
	}

	START_DRAWING();
	END_DRAWING();

	vita2d_free_texture(tex);

	free(buffer);

	return 0;
}