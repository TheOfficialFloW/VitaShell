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
#include "init.h"
#include "language.h"
#include "utils.h"

typedef struct {
	int status;
	int mode;
	int buttonType;
	int buttonId;
	char msg[512];
	float max_width;
	float max_height;
	int progress;
} UncommonDialog;

static UncommonDialog uncommon_dialog;

void calculateDialogBoxSize() {
	int len = strlen(uncommon_dialog.msg);
	char *string = uncommon_dialog.msg;

	// Get max width and height
	uncommon_dialog.max_width = 0.0f;
	uncommon_dialog.max_height = 0.0f;

	int i;
	for (i = 0; i < len + 1; i++) {
		if (uncommon_dialog.msg[i] == '\n') {
			uncommon_dialog.msg[i] = '\0';

			float width = vita2d_pgf_text_width(font, FONT_SIZE, string);
			if (width > uncommon_dialog.max_width)
				uncommon_dialog.max_width = width;

			uncommon_dialog.msg[i] = '\n';

			string = uncommon_dialog.msg + i;
			uncommon_dialog.max_height += FONT_Y_SPACE;
		}

		if (uncommon_dialog.msg[i] == '\0') {
			float width = vita2d_pgf_text_width(font, FONT_SIZE, string);
			if (width > uncommon_dialog.max_width)
				uncommon_dialog.max_width = width;

			uncommon_dialog.max_height += FONT_Y_SPACE;
		}
	}

	// Margin
	uncommon_dialog.max_width += 2.0f * SHELL_MARGIN_X;
	uncommon_dialog.max_height += 2.0f * SHELL_MARGIN_Y;

	// Progress bar box width
	if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
		uncommon_dialog.max_width = UNCOMMON_DIALOG_PROGRESS_BAR_BOX_WIDTH;
		uncommon_dialog.max_height += 2.0f * FONT_Y_SPACE;
	}

	// More space for buttons
	if (uncommon_dialog.buttonType != SCE_MSG_DIALOG_BUTTON_TYPE_NONE)
		uncommon_dialog.max_height += 2.0f * FONT_Y_SPACE;
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
		
		default:
			return -1;
	}

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
				if (pressed_buttons & SCE_CTRL_ENTER) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_OK;
				}

				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
			{
				if (pressed_buttons & SCE_CTRL_ENTER) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
				}

				if (pressed_buttons & SCE_CTRL_CANCEL) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
				}

				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
			{
				if (pressed_buttons & SCE_CTRL_ENTER) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
				}

				if (pressed_buttons & SCE_CTRL_CANCEL) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
				}

				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
			{
				if (pressed_buttons & SCE_CTRL_CANCEL) {
					uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
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

	uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
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
	float x = CENTER(SCREEN_WIDTH, uncommon_dialog.max_width);
	float y = CENTER(SCREEN_HEIGHT, uncommon_dialog.max_height);
	vita2d_draw_rectangle(x, y, uncommon_dialog.max_width, uncommon_dialog.max_height, 0xFF2F2F2F);

	// Draw message
	float string_y = y + SHELL_MARGIN_Y - 2.0f;

	int len = strlen(uncommon_dialog.msg);
	char *string = uncommon_dialog.msg;

	int i;
	for (i = 0; i < len + 1; i++) {
		if (uncommon_dialog.msg[i] == '\n') {
			uncommon_dialog.msg[i] = '\0';
			pgf_draw_text(x + SHELL_MARGIN_X, string_y, WHITE, FONT_SIZE, string);
			uncommon_dialog.msg[i] = '\n';

			string = uncommon_dialog.msg + i;
			string_y += FONT_Y_SPACE;
		}

		if (uncommon_dialog.msg[i] == '\0') {
			pgf_draw_text(x + SHELL_MARGIN_X, string_y, WHITE, FONT_SIZE, string);
			string_y += FONT_Y_SPACE;
		}
	}

	// Dialog type
	char button_string[32];

	switch (uncommon_dialog.buttonType) {
		case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
			sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK]);
			break;
		
		case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
			sprintf(button_string, "%s %s      %s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[YES], enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[NO]);
			break;
			
		case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
			sprintf(button_string, "%s %s      %s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK], enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[CANCEL]);
			break;
			
		case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
			sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[CANCEL]);
			break;
	}

	// Progress bar
	if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
		float width = uncommon_dialog.max_width - 2.0f * SHELL_MARGIN_X;
		vita2d_draw_rectangle(x + SHELL_MARGIN_X, string_y + 10.0f, width, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, GRAY);
		vita2d_draw_rectangle(x + SHELL_MARGIN_X, string_y + 10.0f, uncommon_dialog.progress * width / 100.0f, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, CHARTREUSE);

		char string[8];
		sprintf(string, "%d%%", uncommon_dialog.progress);
		pgf_draw_text(CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, string)), string_y + FONT_Y_SPACE, WHITE, FONT_SIZE, string);

		string_y += 2.0f * FONT_Y_SPACE;
	}

	switch (uncommon_dialog.buttonType) {
		case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
		case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
		case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
		case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
			pgf_draw_text(CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, button_string)), string_y + FONT_Y_SPACE, WHITE, FONT_SIZE, button_string);
			break;
	}

	return 0;
}