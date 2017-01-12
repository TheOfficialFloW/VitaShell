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
#include "context_menu.h"
#include "theme.h"
#include "language.h"
#include "utils.h"

static int ctx_menu_mode = CONTEXT_MENU_CLOSED;
static int ctx_menu_pos = -1;
static int ctx_menu_more_pos = -1;
static float ctx_cur_menu_width = 0.0f;

int getContextMenuPos() {
	return ctx_menu_pos;
}

void setContextMenuPos(int pos) {
	ctx_menu_pos = pos;
}

int getContextMenuMorePos() {
	return ctx_menu_more_pos;
}

void setContextMenuMorePos(int pos) {
	ctx_menu_more_pos = pos;
}

int getContextMenuMode() {
	return ctx_menu_mode;
}

void setContextMenuMode(int mode) {
	ctx_menu_mode = mode;
}

static float easeOut(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.5f) ? (dx * a) : dx;
}

void drawContextMenu(ContextMenu *ctx) {
	// Closing context menu
	if (ctx_menu_mode == CONTEXT_MENU_CLOSING) {
		if (ctx_cur_menu_width > 0.0f) {
			ctx_cur_menu_width -= easeOut(0.0f, ctx_cur_menu_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_CLOSED;
		}
	}

	// Opening context menu
	if (ctx_menu_mode == CONTEXT_MENU_OPENING) {
		if (ctx_cur_menu_width < ctx->menu_max_width) {
			ctx_cur_menu_width += easeOut(ctx_cur_menu_width, ctx->menu_max_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_OPENED;
		}
	}

	// Closing context menu 'More'
	if (ctx_menu_mode == CONTEXT_MENU_MORE_CLOSING) {
		if (ctx_cur_menu_width > ctx->menu_max_width) {
			ctx_cur_menu_width -= easeOut(ctx->menu_max_width, ctx_cur_menu_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_MORE_CLOSED;
		}
	}

	// Opening context menu 'More'
	if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
		if (ctx_cur_menu_width < ctx->menu_max_width + ctx->menu_more_max_width) {
			ctx_cur_menu_width += easeOut(ctx_cur_menu_width, ctx->menu_max_width + ctx->menu_more_max_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_MORE_OPENED;
		}
	}

	// Draw context menu
	if (ctx_menu_mode != CONTEXT_MENU_CLOSED) {
		if (ctx_cur_menu_width < ctx->menu_max_width) {
			vita2d_draw_texture_part(context_image, SCREEN_WIDTH - ctx_cur_menu_width, 0.0f, 0.0f, 0.0f, ctx_cur_menu_width, SCREEN_HEIGHT);
		} else {
			vita2d_draw_texture_part(context_image, SCREEN_WIDTH - ctx_cur_menu_width, 0.0f, 0.0f, 0.0f, ctx->menu_max_width, SCREEN_HEIGHT);
			vita2d_draw_texture_part(context_more_image, SCREEN_WIDTH - ctx_cur_menu_width + ctx->menu_max_width, 0.0f, 0.0f, 0.0f, ctx->menu_more_max_width, SCREEN_HEIGHT);
		}

		int i;
		for (i = 0; i < ctx->n_menu_entries; i++) {
			if (ctx->menu_entries[i].visibility == CTX_VISIBILITY_UNUSED)
				continue;

			float y = START_Y + (i * FONT_Y_SPACE);

			uint32_t color = CONTEXT_MENU_TEXT_COLOR;

			if (i == ctx_menu_pos) {
				if (ctx_menu_mode != CONTEXT_MENU_MORE_OPENED && ctx_menu_mode != CONTEXT_MENU_MORE_OPENING) {
					color = CONTEXT_MENU_FOCUS_COLOR;
				}
			}

			if (ctx->menu_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
				color = INVISIBLE_COLOR;

			// Draw entry text
			pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + CONTEXT_MENU_MARGIN, y, color, FONT_SIZE, language_container[ctx->menu_entries[i].name]);

			// Draw arrow for 'More'
			if (ctx->menu_entries[i].more) {
				char *arrow = NULL;
				if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED || ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
					arrow = LEFT_ARROW;
				} else {
					arrow = RIGHT_ARROW;
				}

				pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + ctx->menu_max_width - vita2d_pgf_text_width(font, FONT_SIZE, arrow) - CONTEXT_MENU_MARGIN, y, color, FONT_SIZE, arrow);
			}
		}

		if (ctx_menu_mode == CONTEXT_MENU_MORE_CLOSING || ctx_menu_mode == CONTEXT_MENU_MORE_OPENED || ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
			for (i = 0; i < ctx->n_menu_more_entries; i++) {
				if (ctx->menu_more_entries[i].visibility == CTX_VISIBILITY_UNUSED)
					continue;

				float y = START_Y + ((ctx_menu_pos + i) * FONT_Y_SPACE);

				uint32_t color = CONTEXT_MENU_TEXT_COLOR;

				if (i == ctx_menu_more_pos) {
					if (ctx_menu_mode != CONTEXT_MENU_MORE_CLOSING) {
						color = CONTEXT_MENU_FOCUS_COLOR;
					}
				}

				if (ctx->menu_more_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
					color = INVISIBLE_COLOR;

				// Draw entry text
				pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + ctx->menu_max_width + CONTEXT_MENU_MARGIN, y, color, FONT_SIZE, language_container[ctx->menu_more_entries[i].name]);
			}
		}
	}
}

void contextMenuCtrl(ContextMenu *ctx) {
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		if (ctx_menu_mode == CONTEXT_MENU_OPENED) {
			int i;
			for (i = ctx->n_menu_entries - 1; i >= 0; i--) {
				if (ctx->menu_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
					if (i < ctx_menu_pos) {
						ctx_menu_pos = i;
						break;
					}
				}
			}
		} else if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
			int i;
			for (i = ctx->n_menu_more_entries - 1; i >= 0; i--) {
				if (ctx->menu_more_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
					if (i < ctx_menu_more_pos) {
						ctx_menu_more_pos = i;
						break;
					}
				}
			}
		}
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		if (ctx_menu_mode == CONTEXT_MENU_OPENED) {
			int i;
			for (i = 0; i < ctx->n_menu_entries; i++) {
				if (ctx->menu_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
					if (i > ctx_menu_pos) {
						ctx_menu_pos = i;
						break;
					}
				}
			}
		} else if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
			int i;
			for (i = 0; i < ctx->n_menu_more_entries; i++) {
				if (ctx->menu_more_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
					if (i > ctx_menu_more_pos) {
						ctx_menu_more_pos = i;
						break;
					}
				}
			}
		}
	}

	// Close
	if (pressed_buttons & SCE_CTRL_TRIANGLE) {
		ctx_menu_mode = CONTEXT_MENU_CLOSING;
	}

	// Back
	if (pressed_buttons & SCE_CTRL_CANCEL || pressed_buttons & SCE_CTRL_LEFT) {
		if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
			ctx_menu_mode = CONTEXT_MENU_MORE_CLOSING;
		} else {
			ctx_menu_mode = CONTEXT_MENU_CLOSING;
		}
	}

	// Handle
	if (pressed_buttons & SCE_CTRL_ENTER || pressed_buttons & SCE_CTRL_RIGHT) {
		if (ctx_menu_mode == CONTEXT_MENU_OPENED) {
			if (ctx->menuEnterCallback)
				ctx_menu_mode = ctx->menuEnterCallback(ctx_menu_pos, ctx->context);
		} else if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
			if (ctx->menuMoreEnterCallback)
				ctx_menu_mode = ctx->menuMoreEnterCallback(ctx_menu_more_pos, ctx->context);
		}
	}
}