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

int isHorizontal(float rad) {
	return ((int)sinf(rad) == 0) ? 1 : 0;
}

void photoMode(float *zoom, float width, float height, float rad, int mode) {
	int horizontal = isHorizontal(rad);

	switch (mode) {
		case MODE_CUSTOM:
			break;
			
		case MODE_FIT_HEIGHT:
			*zoom = horizontal ? (SCREEN_HEIGHT / height) : (SCREEN_HEIGHT / width);
			break;
			
		case MODE_FIT_WIDTH:
			*zoom = horizontal ? (SCREEN_WIDTH / width) : (SCREEN_WIDTH / height);
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

	float rad = 0;
	float zoom = 1.0f;

	// Photo mode
	int mode = MODE_FIT_HEIGHT;
	photoMode(&zoom, width, height, rad, mode);

	uint64_t time = 0;

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

			photoMode(&zoom, width, height, rad, mode);
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

		// Move. TODO: Moving by pad.lx/pad.ly
		float dx = 0.0f, dy = 0.0f;

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
			if (isHorizontal(rad)) {
				dx = -cosf(rad);
			} else {
				dy = sinf(rad);
			}
		} else if (current_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
			if (isHorizontal(rad)) {
				dx = cosf(rad);
			} else {
				dy = -sinf(rad);
			}
		}

		if (current_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			if (!isHorizontal(rad)) {
				dx = -sinf(rad);
			} else {
				dy = -cosf(rad);
			}
		} else if (current_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
			if (!isHorizontal(rad)) {
				dx = sinf(rad);
			} else {
				dy = cosf(rad);
			}
		}

		if (dx != 0.0f)
			hotspot_x += dx * MOVE_INTERVAL / zoom;

		if (dy != 0.0f)
			hotspot_y += dy * MOVE_INTERVAL / zoom;

		// Limit
		float w = isHorizontal(rad) ? SCREEN_HALF_WIDTH : SCREEN_HALF_HEIGHT;
		float h = isHorizontal(rad) ? SCREEN_HALF_HEIGHT : SCREEN_HALF_WIDTH;

		if ((zoom * width) > 2.0f * w) {
			if (hotspot_x < (w / zoom)) {
				hotspot_x = w / zoom;
			} else if (hotspot_x > (width - w / zoom)) {
				hotspot_x = width - w / zoom;
			}
		} else {
			hotspot_x = half_width;
		}

		if ((zoom * height) > 2.0f * h) {
			if (hotspot_y < (h / zoom)) {
				hotspot_y = h / zoom;
			} else if (hotspot_y > (height - h / zoom)) {
				hotspot_y = height - h / zoom;
			}
		} else {
			hotspot_y = half_height;
		}

		// Start drawing
		START_DRAWING();

		// Photo
		vita2d_draw_texture_scale_rotate_hotspot(tex, SCREEN_HALF_WIDTH, SCREEN_HALF_HEIGHT, zoom, zoom, rad, hotspot_x, hotspot_y);

		// Zoom text
		if ((sceKernelGetProcessTimeWide() - time) < ZOOM_TEXT_TIME)
			pgf_draw_textf(SHELL_MARGIN_X, SCREEN_HEIGHT - 3.0f * SHELL_MARGIN_Y, WHITE, FONT_SIZE, "%.0f%%", zoom * 100.0f);

		// End drawing
		END_DRAWING();
	}

	vita2d_free_texture(tex);

	free(buffer);

	if (hex_viewer)
		hexViewer(file);

	return 0;
}