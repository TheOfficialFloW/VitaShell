/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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
static float ctx_cur_menu_width = 0.0f;

static ContextMenu *cur_ctx = NULL;

int getContextMenuMode() {
  return ctx_menu_mode;
}

void setContextMenu(ContextMenu *ctx) {
  cur_ctx = ctx;
}

void setContextMenuMode(int mode) {
  ctx_menu_mode = mode;
}

void drawContextMenu() {
  if (!cur_ctx)
    return;

  // Closing context menu
  if (ctx_menu_mode == CONTEXT_MENU_CLOSING) {
    if (ctx_cur_menu_width > 0.0f) {
      ctx_cur_menu_width -= easeOut(0.0f, ctx_cur_menu_width, 0.375f, 0.5f);
    } else {
      ctx_menu_mode = CONTEXT_MENU_CLOSED;
    }
  }

  // Opening context menu
  if (ctx_menu_mode == CONTEXT_MENU_OPENING) {
    if (ctx_cur_menu_width < cur_ctx->max_width) {
      ctx_cur_menu_width += easeOut(ctx_cur_menu_width, cur_ctx->max_width, 0.375f, 0.5f);
    } else {
      ctx_menu_mode = CONTEXT_MENU_OPENED;
    }
  }

  if (cur_ctx->parent) {
    // Closing context menu 'More'
    if (ctx_menu_mode == CONTEXT_MENU_MORE_CLOSING) {
      if (ctx_cur_menu_width > cur_ctx->parent->max_width) {
        ctx_cur_menu_width -= easeOut(cur_ctx->parent->max_width, ctx_cur_menu_width, 0.375f, 0.5f);
      } else {
        cur_ctx = cur_ctx->parent;
        ctx_menu_mode = CONTEXT_MENU_MORE_CLOSED;
      }
    }

    // Opening context menu 'More'
    if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
      if (ctx_cur_menu_width < cur_ctx->max_width + cur_ctx->parent->max_width) {
        ctx_cur_menu_width += easeOut(ctx_cur_menu_width, cur_ctx->max_width+cur_ctx->parent->max_width, 0.375f, 0.5f);
      } else {
        ctx_menu_mode = CONTEXT_MENU_MORE_OPENED;
      }
    }
  }

  // Draw context menu
  if (ctx_menu_mode != CONTEXT_MENU_CLOSED) {
    if (!cur_ctx->parent) {
      vita2d_draw_texture_part(context_image, SCREEN_WIDTH - ctx_cur_menu_width, 0.0f, 0.0f, 0.0f, ctx_cur_menu_width, SCREEN_HEIGHT);
    } else {
      vita2d_draw_texture_part(context_image, SCREEN_WIDTH - ctx_cur_menu_width, 0.0f, 0.0f, 0.0f, cur_ctx->parent->max_width, SCREEN_HEIGHT);
      vita2d_draw_texture_part(context_more_image, SCREEN_WIDTH - ctx_cur_menu_width + cur_ctx->parent->max_width, 0.0f, 0.0f, 0.0f, cur_ctx->max_width, SCREEN_HEIGHT);
    }

    ContextMenu *ctx = cur_ctx;
    if (ctx->parent)
      ctx = ctx->parent;

    int i;
    for (i = 0; i < ctx->n_entries; i++) {
      float y = START_Y + (ctx->entries[i].pos * FONT_Y_SPACE);

      uint32_t color = CONTEXT_MENU_TEXT_COLOR;

      if (i == ctx->sel) {
        if (ctx_menu_mode != CONTEXT_MENU_MORE_OPENED && ctx_menu_mode != CONTEXT_MENU_MORE_OPENING) {
          color = CONTEXT_MENU_FOCUS_COLOR;
        }
      }

      if (ctx->entries[i].visibility == CTX_INVISIBLE)
        color = INVISIBLE_COLOR;

      // Draw entry text
      pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + CONTEXT_MENU_MARGIN, y, color, language_container[ctx->entries[i].name]);

      // Draw arrow for 'More'
      if (ctx->entries[i].flags & CTX_FLAG_MORE) {
        char *arrow = RIGHT_ARROW;
        
        if (ctx->sel == i) {
          if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED ||
            ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
            arrow = LEFT_ARROW;
          }
        }

        pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + ctx->max_width - pgf_text_width(arrow) - CONTEXT_MENU_MARGIN, y, color, arrow);
      }
    }

    if (ctx_menu_mode == CONTEXT_MENU_MORE_CLOSING ||
        ctx_menu_mode == CONTEXT_MENU_MORE_OPENED ||
        ctx_menu_mode == CONTEXT_MENU_MORE_OPENING) {
      for (i = 0; i < cur_ctx->n_entries; i++) {
        float y = START_Y + (cur_ctx->entries[i].pos * FONT_Y_SPACE);

        uint32_t color = CONTEXT_MENU_TEXT_COLOR;

        if (i == cur_ctx->sel) {
          if (ctx_menu_mode != CONTEXT_MENU_MORE_CLOSING) {
            color = CONTEXT_MENU_FOCUS_COLOR;
          }
        }

        if (cur_ctx->entries[i].visibility == CTX_INVISIBLE)
          color = INVISIBLE_COLOR;

        // Draw entry text
        pgf_draw_text(SCREEN_WIDTH - ctx_cur_menu_width + ctx->max_width + CONTEXT_MENU_MARGIN, y, color, language_container[cur_ctx->entries[i].name]);
      }
    }
  }
}

void contextMenuCtrl() {
  if (!cur_ctx)
    return;

  if (hold_pad[PAD_UP] || hold2_pad[PAD_LEFT_ANALOG_UP]) {
    if (ctx_menu_mode == CONTEXT_MENU_OPENED || ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
      int i;
      for (i = cur_ctx->n_entries - 1; i >= 0; i--) {
        if (cur_ctx->entries[i].visibility == CTX_VISIBLE) {
          if (i < cur_ctx->sel) {
            cur_ctx->sel = i;
            break;
          }
        }
      }
    }
  } else if (hold_pad[PAD_DOWN] || hold2_pad[PAD_LEFT_ANALOG_DOWN]) {
    if (ctx_menu_mode == CONTEXT_MENU_OPENED || ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
      int i;
      for (i = 0; i < cur_ctx->n_entries; i++) {
        if (cur_ctx->entries[i].visibility == CTX_VISIBLE) {
          if (i > cur_ctx->sel) {
            if (!(cur_ctx->entries[i].flags & CTX_FLAG_BARRIER) || (hold_count[PAD_DOWN] <= 1 && hold2_count[PAD_LEFT_ANALOG_DOWN] <= 1))
              cur_ctx->sel = i;
            break;
          }
        }
      }
    }
  }

  // Close
  if (pressed_pad[PAD_TRIANGLE]) {
    ctx_menu_mode = CONTEXT_MENU_CLOSING;
  }

  // Back
  if (pressed_pad[PAD_CANCEL] || pressed_pad[PAD_LEFT]) {
    if (ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
      ctx_menu_mode = CONTEXT_MENU_MORE_CLOSING;
    } else {
      ctx_menu_mode = CONTEXT_MENU_CLOSING;
    }
  }

  // Handle
  if (pressed_pad[PAD_ENTER] || pressed_pad[PAD_RIGHT]) {
    if (ctx_menu_mode == CONTEXT_MENU_OPENED || ctx_menu_mode == CONTEXT_MENU_MORE_OPENED) {
      if (cur_ctx->callback)
        ctx_menu_mode = cur_ctx->callback(cur_ctx->sel, cur_ctx->context);
    }
  }
}