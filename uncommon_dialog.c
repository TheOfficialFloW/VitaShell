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
#include "uncommon_dialog.h"
#include "qr.h"

typedef struct {
  int dialog_status;
  int status;
  int mode;
  int buttonType;
  int buttonId;
  char msg[512];
  char info[64];
  float x;
  float y;
  float width;
  float height;
  float scale;
  int progress;
} UncommonDialog;

static UncommonDialog uncommon_dialog;

static void calculateDialogBoxSize() {
  int len = strlen(uncommon_dialog.msg);
  char *string = uncommon_dialog.msg;
  float text_width = 0;
  
  // Get width and height
  uncommon_dialog.width = 0.0f;
  uncommon_dialog.height = 0.0f;

  int i;
  for (i = 0; i < len + 1; i++) {
    if (uncommon_dialog.msg[i] == '\n') {
      uncommon_dialog.msg[i] = '\0';

      float width = pgf_text_width(string);
      if (width > uncommon_dialog.width)
        uncommon_dialog.width = width;
      
      uncommon_dialog.msg[i] = '\n';

      string = uncommon_dialog.msg + i;
    }

    if (uncommon_dialog.msg[i] == '\0') {
      float width = pgf_text_width(string);
      if (width > uncommon_dialog.width)
        uncommon_dialog.width = width;
    }

    char tmp = uncommon_dialog.msg[i + 1];
    uncommon_dialog.msg[i + 1] = '\0';
    text_width = pgf_text_width(uncommon_dialog.msg);
    uncommon_dialog.msg[i + 1] = tmp;

    if (text_width > UNCOMMON_DIALOG_MAX_WIDTH) {
      int lastSpace = i - 1;
      while (uncommon_dialog.msg[lastSpace] != ' ' && uncommon_dialog.msg[lastSpace] != '\n' && lastSpace > 0)
        lastSpace--;
      
      if (lastSpace == 0 || uncommon_dialog.msg[lastSpace] == '\n') {
        memmove(uncommon_dialog.msg + i + 1, uncommon_dialog.msg + i, strlen(uncommon_dialog.msg) - (i + 1));
        uncommon_dialog.msg[i] = '\n';
      } else {
        uncommon_dialog.msg[lastSpace] = '\n';
      }
      
      uncommon_dialog.width = text_width;
      string = uncommon_dialog.msg + i;
    }    
  }

  uncommon_dialog.height += FONT_Y_SPACE;
  for (i = 0; uncommon_dialog.msg[i]; i++)
    uncommon_dialog.height += FONT_Y_SPACE * (uncommon_dialog.msg[i] == '\n');
  
  // Margin
  uncommon_dialog.width += 2.0f * SHELL_MARGIN_X;
  uncommon_dialog.height += 2.0f * SHELL_MARGIN_Y;

  // Progress bar box width
  if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
    uncommon_dialog.width = UNCOMMON_DIALOG_PROGRESS_BAR_BOX_WIDTH;
    uncommon_dialog.height += 2.0f * FONT_Y_SPACE;
  }
  
  if (uncommon_dialog.mode == MSG_DIALOG_MODE_QR_SCAN) {
    uncommon_dialog.width = CAM_WIDTH + 30;
    uncommon_dialog.height += CAM_HEIGHT;
  }
  
  // More space for buttons
  if (uncommon_dialog.buttonType != SCE_MSG_DIALOG_BUTTON_TYPE_NONE)
    uncommon_dialog.height += 2.0f * FONT_Y_SPACE;

  // Position
  uncommon_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, uncommon_dialog.width);
  uncommon_dialog.y = ALIGN_CENTER(SCREEN_HEIGHT, uncommon_dialog.height);

  // Align
  int y_n = (int)((float)(uncommon_dialog.y - 2.0f) / FONT_Y_SPACE);
  uncommon_dialog.y = (float)y_n * FONT_Y_SPACE + 2.0f;

  // Scale
  uncommon_dialog.scale = 0;
}

int sceMsgDialogInit(const SceMsgDialogParam *param) {
  if (!param)
    return -1;

  memset(&uncommon_dialog, 0, sizeof(UncommonDialog));

  switch (param->mode) {
    case SCE_MSG_DIALOG_MODE_USER_MSG:
    {
      if (!param->userMsgParam || !param->userMsgParam->msg)
        return -1;

      strncpy(uncommon_dialog.msg, (char *)param->userMsgParam->msg, sizeof(uncommon_dialog.msg) - 1);
      uncommon_dialog.buttonType = param->userMsgParam->buttonType;
      break;
    }
    
    case SCE_MSG_DIALOG_MODE_PROGRESS_BAR:
    {
      if (!param->progBarParam || !param->progBarParam->msg)
        return -1;

      strncpy(uncommon_dialog.msg, (char *)param->progBarParam->msg, sizeof(uncommon_dialog.msg) - 1);
      uncommon_dialog.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL;
      break;
    }
    
    case MSG_DIALOG_MODE_QR_SCAN:
    {
      strncpy(uncommon_dialog.msg, (char *)param->userMsgParam->msg, sizeof(uncommon_dialog.msg) - 1);
       uncommon_dialog.buttonType = SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL;
      break;
    }
    
    default:
      return -1;
  }

  uncommon_dialog.dialog_status = UNCOMMON_DIALOG_OPENING;
  uncommon_dialog.mode = param->mode;
  uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_RUNNING;

  calculateDialogBoxSize();

  return 0;
}

SceCommonDialogStatus sceMsgDialogGetStatus(void) {
  if (uncommon_dialog.status == SCE_COMMON_DIALOG_STATUS_RUNNING) {
    switch (uncommon_dialog.buttonType) {
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
      {
        if (pressed_pad[PAD_ENTER]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
          uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_OK;
        }

        break;
      }
      
      case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
      {
        if (pressed_pad[PAD_ENTER]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
          uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
        }

        if (pressed_pad[PAD_CANCEL]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
          uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
        }

        break;
      }
      
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
      {
        if (pressed_pad[PAD_ENTER]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
          uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
        }

        if (pressed_pad[PAD_CANCEL]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
          uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
        }

        break;
      }
      
      case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
      {
        if (pressed_pad[PAD_CANCEL]) {
          uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
        }

        break;
      }
    }
  }

  return uncommon_dialog.status;
}

int sceMsgDialogClose(void) {
  if (uncommon_dialog.status != SCE_COMMON_DIALOG_STATUS_RUNNING)
    return -1;

  uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSING;
  return 0;
}

int sceMsgDialogTerm(void) {
  if (uncommon_dialog.status == SCE_COMMON_DIALOG_STATUS_NONE)
    return -1;

  uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_NONE;
  return 0;
}

int sceMsgDialogGetResult(SceMsgDialogResult *result) {
  if (!result)
    return -1;

  result->buttonId = uncommon_dialog.buttonId;
  return 0;
}

int sceMsgDialogProgressBarSetInfo(SceMsgDialogProgressBarTarget target, const SceChar8 *barInfo) {
  strncpy(uncommon_dialog.info, (char *)barInfo, sizeof(uncommon_dialog.info) - 1);
  return 0;
}

int sceMsgDialogProgressBarSetMsg(SceMsgDialogProgressBarTarget target, const SceChar8 *barMsg) {
  strncpy(uncommon_dialog.msg, (char *)barMsg, sizeof(uncommon_dialog.msg) - 1);
  return 0;
}

int sceMsgDialogProgressBarSetValue(SceMsgDialogProgressBarTarget target, SceUInt32 rate) {
  if (rate > 100)
    return -1;

  uncommon_dialog.progress = rate;
  return 0;
}

int drawUncommonDialog() {
  if (uncommon_dialog.status == SCE_COMMON_DIALOG_STATUS_NONE)
    return 0;

  // Dialog background
  vita2d_draw_texture_scale_rotate_hotspot(dialog_image, uncommon_dialog.x + uncommon_dialog.width / 2.0f,
                                           uncommon_dialog.y + uncommon_dialog.height / 2.0f,
                                           uncommon_dialog.scale * (uncommon_dialog.width/vita2d_texture_get_width(dialog_image)),
                                           uncommon_dialog.scale * (uncommon_dialog.height/vita2d_texture_get_height(dialog_image)),
                                           0.0f, vita2d_texture_get_width(dialog_image) / 2.0f, vita2d_texture_get_height(dialog_image) / 2.0f);

  // Easing out
  if (uncommon_dialog.dialog_status == UNCOMMON_DIALOG_CLOSING) {
    if (uncommon_dialog.scale > 0.0f) {
      uncommon_dialog.scale -= easeOut(0.0f, uncommon_dialog.scale, 0.25f, 0.01f);
    } else {
      uncommon_dialog.dialog_status = UNCOMMON_DIALOG_CLOSED;
      uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
    }
  }

  if (uncommon_dialog.dialog_status == UNCOMMON_DIALOG_OPENING) {
    if (uncommon_dialog.scale < 1.0f) {
      uncommon_dialog.scale += easeOut(uncommon_dialog.scale, 1.0f, 0.25f, 0.01f);
    } else {
      uncommon_dialog.dialog_status = UNCOMMON_DIALOG_OPENED;
    }
  }

  if (uncommon_dialog.dialog_status == UNCOMMON_DIALOG_OPENED) {
    float string_y = uncommon_dialog.y + SHELL_MARGIN_Y - 2.0f;

    // Draw info
    if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
      if (uncommon_dialog.info[0] != '\0') {
        float x = ALIGN_RIGHT(uncommon_dialog.x + uncommon_dialog.width - SHELL_MARGIN_X, pgf_text_width(uncommon_dialog.info));
        pgf_draw_text(x, string_y, DIALOG_COLOR, uncommon_dialog.info);
      }
    }

    // Draw message
    int len = strlen(uncommon_dialog.msg);
    char *string = uncommon_dialog.msg;

    int i;
    for (i = 0; i < len + 1; i++) {
      if (uncommon_dialog.msg[i] == '\n') {
        uncommon_dialog.msg[i] = '\0';
        pgf_draw_text(uncommon_dialog.x + SHELL_MARGIN_X, string_y, DIALOG_COLOR, string);
        uncommon_dialog.msg[i] = '\n';

        string = uncommon_dialog.msg+i + 1;
        string_y += FONT_Y_SPACE;
      }

      if (uncommon_dialog.msg[i] == '\0') {
        pgf_draw_text(uncommon_dialog.x + SHELL_MARGIN_X, string_y, DIALOG_COLOR, string);
        string_y += FONT_Y_SPACE;
      }
    }
    
    // Dialog type
    char button_string[128];

    switch (uncommon_dialog.buttonType) {
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
        sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK]);
        break;
      
      case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
        sprintf(button_string, "%s %s    %s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[YES],
                                                 enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[NO]);
        break;
        
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
        sprintf(button_string, "%s %s    %s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK],
                                                 enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[CANCEL]);
        break;
        
      case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
        sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[CANCEL]);
        break;
    }

    // Progress bar
    if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
      float width = uncommon_dialog.width - 2.0f * SHELL_MARGIN_X;
      float x = uncommon_dialog.x + SHELL_MARGIN_X;
      vita2d_draw_rectangle(x, string_y + 10.0f, width, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_BG_COLOR);
      vita2d_draw_rectangle(x, string_y + 10.0f, uncommon_dialog.progress * width / 100.0f, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_COLOR);

      char string[8];
      sprintf(string, "%d%%", uncommon_dialog.progress);
      pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, string);

      string_y += 2.0f * FONT_Y_SPACE;
    }
    
    if (uncommon_dialog.mode == MSG_DIALOG_MODE_QR_SCAN) {
      renderCameraQR(uncommon_dialog.x + 15, string_y + 10);
      string_y += CAM_HEIGHT;
    }

    switch (uncommon_dialog.buttonType) {
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
      case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
      case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
      case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
        pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(button_string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, button_string);
        break;
    }
  }

  return 0;
}