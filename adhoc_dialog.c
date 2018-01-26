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
#include "init.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "adhoc_dialog.h"
#include "uncommon_dialog.h"

typedef struct {
  int status;
  float x;
  float y;
  float width;
  float height;
  float scale;
} AdhocDialog;

static AdhocDialog adhoc_dialog;

int getAdhocDialogStatus() {
  
}

int updateAdhocDialog() {
  
}

int initAdhocDialog() {
}

void adhocDialogCtrl() {
}

void drawAdhocDialog() {
  if (adhoc_dialog.status == ADHOC_DIALOG_CLOSED)
    return;

  // Dialog background
  float dialog_width = vita2d_texture_get_width(dialog_image);
  float dialog_height = vita2d_texture_get_height(dialog_image);
  vita2d_draw_texture_scale_rotate_hotspot(dialog_image, adhoc_dialog.x + adhoc_dialog.width / 2.0f,
                                           adhoc_dialog.y + adhoc_dialog.height / 2.0f,
                                           adhoc_dialog.scale * (adhoc_dialog.width/dialog_width),
                                           adhoc_dialog.scale * (adhoc_dialog.height/dialog_height),
                                           0.0f, dialog_width / 2.0f, dialog_height / 2.0f);

  // Easing out
  if (adhoc_dialog.status == ADHOC_DIALOG_CLOSING) {
    if (adhoc_dialog.scale > 0.0f) {
      adhoc_dialog.scale -= easeOut(0.0f, adhoc_dialog.scale, 0.25f, 0.01f);
    } else {
      adhoc_dialog.status = ADHOC_DIALOG_CLOSED;
    }
  }

  if (adhoc_dialog.status == ADHOC_DIALOG_OPENING) {
    if (adhoc_dialog.scale < 1.0f) {
      adhoc_dialog.scale += easeOut(adhoc_dialog.scale, 1.0f, 0.25f, 0.01f);
    } else {
      adhoc_dialog.status = ADHOC_DIALOG_OPENED;
    }
  }

  if (adhoc_dialog.status == ADHOC_DIALOG_OPENED) {
    float string_y = adhoc_dialog.y + SHELL_MARGIN_Y - 2.0f;

    char button_string[32];
    sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK]);
    pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(button_string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, button_string);
  }
}