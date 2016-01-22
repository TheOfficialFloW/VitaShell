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

	float scale_x = 1.0f;
	float scale_y = 1.0f;
	float rad = 0.0f;

	// Screen center
	float x = SCREEN_WIDTH / 2.0f;
	float y = SCREEN_HEIGHT / 2.0f;

	// Adjust to screen
	float width = vita2d_texture_get_width(tex);
	float height = vita2d_texture_get_height(tex);
	float width2 = width / 2.0f;
	float height2 = height / 2.0f;

	if (height > SCREEN_HEIGHT) { // height is first priority
		scale_y = SCREEN_HEIGHT / height;
		scale_x = scale_y * width;
	} else if (width > SCREEN_WIDTH) { // width is second priority
		scale_x = SCREEN_WIDTH / width;
		scale_y = scale_x * height;
	}

	while (1) {
		readPad();

		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;
		}

		if (current_buttons & SCE_CTRL_UP || current_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			y -= MOVE_SPEED;
		} else if (current_buttons & SCE_CTRL_DOWN || current_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
			y += MOVE_SPEED;
		}

		if (current_buttons & SCE_CTRL_LEFT || current_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
			x -= MOVE_SPEED;
		} else if (current_buttons & SCE_CTRL_RIGHT || current_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
			x += MOVE_SPEED;
		}

		if ((x - scale_x * width2) > SCREEN_WIDTH) {
			x = SCREEN_WIDTH + scale_x * width2;
		} else if ((x + scale_x * width2) < 0) {
			x = -scale_x * width2;
		}

		if ((y - scale_y * height2) > SCREEN_HEIGHT) {
			y = SCREEN_HEIGHT + scale_y * height2;
		} else if ((y + scale_y * height2) < 0) {
			y = -scale_y * height2;
		}

		if (hold2_buttons & SCE_CTRL_LTRIGGER) {
			rad -= ROTATION_SPEED;
		}

		if (hold2_buttons & SCE_CTRL_RTRIGGER) {
			rad += ROTATION_SPEED;
		}

		if (current_buttons & (SCE_CTRL_RIGHT_ANALOG_UP | SCE_CTRL_RIGHT_ANALOG_DOWN)) {
			scale_y -= ZOOM_SPEED * ((pad.ry - ANALOG_CENTER) / 128.0f);
		}

		if (current_buttons & (SCE_CTRL_RIGHT_ANALOG_RIGHT | SCE_CTRL_RIGHT_ANALOG_LEFT)) {
			scale_x += ZOOM_SPEED * ((pad.rx - ANALOG_CENTER) / 128.0f);
		}

		if (scale_x < ZOOM_MIN) {
			scale_x = ZOOM_MIN;
		} else if (scale_x > ZOOM_MAX) {
			scale_x = ZOOM_MAX;
		}

		if (scale_y < ZOOM_MIN) {
			scale_y = ZOOM_MIN;
		} else if (scale_y > ZOOM_MAX) {
			scale_y = ZOOM_MAX;
		}

		if (current_buttons & SCE_CTRL_TRIANGLE) {
			if (height > SCREEN_HEIGHT) { // height is first priority
				scale_y = SCREEN_HEIGHT / height;
				scale_x = scale_y * width;
			} else if (width > SCREEN_WIDTH) { // width is second priority
				scale_x = SCREEN_WIDTH / width;
				scale_y = scale_x * height;
			}
			scale_x = 1.0f;
			scale_y = 1.0f;
			rad = 0.0f;
			x = SCREEN_WIDTH / 2.0f;
			y = SCREEN_HEIGHT / 2.0f;
		}

		// Start drawing
		START_DRAWING();

		vita2d_draw_texture_scale_rotate_hotspot(tex, x, y, scale_x, scale_y, rad, vita2d_texture_get_width(tex) / 2.0f, vita2d_texture_get_height(tex) / 2.0f);

		// End drawing
		END_DRAWING();
	}

	START_DRAWING();
	END_DRAWING();

	vita2d_free_texture(tex);

	free(buffer);

	return 0;
}
