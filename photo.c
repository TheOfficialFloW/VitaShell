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

void photoMode(float *scale, float width, float height, int angle, int mode) {
	int horizontal = (angle == 0 || angle == 180);

	switch (mode) {
		case MODE_CUSTOM:
			break;
			
		case MODE_FIT_HEIGHT:
			*scale = horizontal ? (SCREEN_HEIGHT / height) : SCREEN_HEIGHT / width;
			break;
			
		case MODE_FIT_WIDTH:
			*scale = horizontal ? (SCREEN_WIDTH / width) : SCREEN_WIDTH / height;
			break;
	}
}

int photoViewer(char *file, int type) {
	int hex_viewer = 0;

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

	// Variables
	float width = vita2d_texture_get_width(tex);
	float height = vita2d_texture_get_height(tex);

	float half_width = width / 2.0f;
	float half_height = height / 2.0f;

	float hotspot_x = half_width;
	float hotspot_y = half_height;

	int angle = 0;
	float scale = 1.0f;

	// Photo mode
	int mode = MODE_FIT_HEIGHT;
	photoMode(&scale, width, height, angle, mode);

	while (1) {
		readPad();

		// Cancel
		if (pressed_buttons & SCE_CTRL_CANCEL) {
			hex_viewer = 0;
			break;
		}
/*
		// Switch to hex viewer
		if (pressed_buttons & SCE_CTRL_SQUARE) {
			hex_viewer = 1;
			break;
		}
*/
		// Photo mode
		if (pressed_buttons & SCE_CTRL_ENTER) {
			switch (mode) {
				case MODE_CUSTOM:
					mode = MODE_FIT_HEIGHT;
					break;
					
				case MODE_FIT_HEIGHT:
					mode = MODE_FIT_WIDTH;
					break;
					
				case MODE_FIT_WIDTH:
					mode = MODE_FIT_HEIGHT;
					break;
			}

			hotspot_x = half_width;
			hotspot_y = half_height;

			photoMode(&scale, width, height, angle, mode);
		}

		// Rotate
		if (pressed_buttons & SCE_CTRL_LTRIGGER) {
			angle -= 90;
			if (angle < 0)
				angle += 360;

			photoMode(&scale, width, height, angle, mode);
		}

		if (pressed_buttons & SCE_CTRL_RTRIGGER) {
			angle += 90;
			if (angle >= 360)
				angle -= 360;

			photoMode(&scale, width, height, angle, mode);
		}

		// Scale
		if (current_buttons & SCE_CTRL_RIGHT_ANALOG_DOWN) {
			mode = MODE_CUSTOM;
			scale /= ZOOM_FACTOR;
		}

		if (current_buttons & SCE_CTRL_RIGHT_ANALOG_UP) {
			mode = MODE_CUSTOM;
			scale *= ZOOM_FACTOR;
		}

		if (scale < ZOOM_MIN) {
			scale = ZOOM_MIN;
		}

		if (scale > ZOOM_MAX) {
			scale = ZOOM_MAX;
		}

		// Move
		int dx = 0, dy = 0;

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
			dx = -cos(DEG_TO_RAD(angle));
			dy = sin(DEG_TO_RAD(angle));
		}

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
			dx = cos(DEG_TO_RAD(angle));
			dy = -sin(DEG_TO_RAD(angle));
		}

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			dx = -sin(DEG_TO_RAD(angle));
			dy = -cos(DEG_TO_RAD(angle));
		}

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
			dx = sin(DEG_TO_RAD(angle));
			dy = cos(DEG_TO_RAD(angle));
		}

		if (dx < 0)
			hotspot_x -= MOVE_INTERVAL / scale;

		if (dx > 0)
			hotspot_x += MOVE_INTERVAL / scale;

		if (dy < 0)
			hotspot_y -= MOVE_INTERVAL / scale;
		
		if (dy > 0)
			hotspot_y += MOVE_INTERVAL / scale;

		int w = (angle == 0 || angle == 180) ? SCREEN_HALF_WIDTH : SCREEN_HALF_HEIGHT;
		int h = (angle == 0 || angle == 180) ? SCREEN_HALF_HEIGHT : SCREEN_HALF_WIDTH;

		if ((scale * width) > 2 * w) {
			if (hotspot_x < w / scale) {
				hotspot_x = w / scale;
			} else if (hotspot_x > width - w / scale) {
				hotspot_x = width - w / scale;
			}
		} else {
			hotspot_x = half_width;
		}

		if ((scale * height) > 2 * h) {
			if (hotspot_y < h / scale) {
				hotspot_y = h / scale;
			} else if (hotspot_y > height - h / scale) {
				hotspot_y = height - h / scale;
			}
		} else {
			hotspot_y = half_height;
		}

		// Start drawing
		START_DRAWING();

		vita2d_draw_texture_scale_rotate_hotspot(tex, SCREEN_HALF_WIDTH, SCREEN_HALF_HEIGHT, scale, scale, DEG_TO_RAD(angle), hotspot_x, hotspot_y);

		// End drawing
		END_DRAWING();
	}

	vita2d_free_texture(tex);

	free(buffer);

	if (hex_viewer)
		hexViewer(file);

	return 0;
}