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
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "uncommon_dialog.h"
#include <stdbool.h> 

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

typedef struct {
	int animation_mode;
	int status;
	int mode;
	int buttonType;
	int buttonId;
	char msg[512];
	float x;
	float y;
	float width;
	float height;
	float scale;
	int progress;
} UncommonDialog;

static UncommonDialog uncommon_dialog;

bool lock = false;
SceTouchData touch;	

// State_Touch is global value to certain function TOUCH_FRONT_DIALOG alway return absolute one value for one time.
int Touch_Up = -1;

int AREA_TOUCH_DIALOG = 100.0f;
int type_touch = 0; 

float x_d_start = 0;
float x_d_end = 0;
float y_d = 0;

// Output:
//	1 : Touch up select
int TOUCH_FRONT_DIALOG() {	
	sceTouchPeek(0, &touch, 1);
		
	if(lock & (touch.reportNum == 0) ) {							
			Touch_Up = 1;						
	}

	if (touch.reportNum > 0) {
		lock = true;							
	}
	else lock = false;
	return Touch_Up;
}

void calculateDialogBoxSize() {
	int len = strlen(uncommon_dialog.msg);
	char *string = uncommon_dialog.msg;

	// Get width and height
	uncommon_dialog.width = 0.0f;
	uncommon_dialog.height = 0.0f;

	int i;
	for (i = 0; i < len + 1; i++) {
		if (uncommon_dialog.msg[i] == '\n') {
			uncommon_dialog.msg[i] = '\0';

			float width = vita2d_pgf_text_width(font, FONT_SIZE, string);
			if (width > uncommon_dialog.width)
				uncommon_dialog.width = width;

			uncommon_dialog.msg[i] = '\n';

			string = uncommon_dialog.msg + i;
			uncommon_dialog.height += FONT_Y_SPACE;
		}

		if (uncommon_dialog.msg[i] == '\0') {
			float width = vita2d_pgf_text_width(font, FONT_SIZE, string);
			if (width > uncommon_dialog.width)
				uncommon_dialog.width = width;

			uncommon_dialog.height += FONT_Y_SPACE;
		}
	}

	// Margin
	uncommon_dialog.width += 2.0f * SHELL_MARGIN_X;
	uncommon_dialog.height += 2.0f * SHELL_MARGIN_Y;

	// Progress bar box width
	if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
		uncommon_dialog.width = UNCOMMON_DIALOG_PROGRESS_BAR_BOX_WIDTH;
		uncommon_dialog.height += 2.0f * FONT_Y_SPACE;
	}

	// More space for buttons
	if (uncommon_dialog.buttonType != SCE_MSG_DIALOG_BUTTON_TYPE_NONE)
		uncommon_dialog.height += 2.0f * FONT_Y_SPACE;

	// Position
	uncommon_dialog.x = CENTER(SCREEN_WIDTH, uncommon_dialog.width);
	uncommon_dialog.y = CENTER(SCREEN_HEIGHT, uncommon_dialog.height);

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
		
		default:
			return -1;
	}

	uncommon_dialog.animation_mode = UNCOMMON_DIALOG_OPENING;
	uncommon_dialog.mode = param->mode;
	uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_RUNNING;

	calculateDialogBoxSize();

	return 0;
}

SceCommonDialogStatus sceMsgDialogGetStatus(void) {
	if (uncommon_dialog.status == SCE_COMMON_DIALOG_STATUS_RUNNING) {

		if (TOUCH_FRONT_DIALOG() == 1) {
						Touch_Up = -1;
						float t_x = touch.report[0].x;						 		  
						float t_y = touch.report[0].y;					
						if ((t_x < x_d_end) & (t_x > x_d_start) & (t_y < y_d + FONT_Y_SPACE) & (t_y > y_d - FONT_Y_SPACE)) {						
								type_touch = 1;
						}
						else {
							type_touch = 2;
						}																
					}

		switch (uncommon_dialog.buttonType) {
			case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
			{
				if ((pressed_buttons & SCE_CTRL_ENTER) || (type_touch == 1)) {
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_OK;
				}
				type_touch = 0;
				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
			{
				

				if ((pressed_buttons & SCE_CTRL_ENTER) || (type_touch == 1)) {
					
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
					
				}

				if ((pressed_buttons & SCE_CTRL_CANCEL) || (type_touch == 2)) {
					
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
				}
				type_touch = 0;
				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
			{
				if ((pressed_buttons & SCE_CTRL_ENTER) || (type_touch == 1))  {
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_YES;
				}

				if ((pressed_buttons & SCE_CTRL_CANCEL) || (type_touch == 2))  {
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
					uncommon_dialog.buttonId = SCE_MSG_DIALOG_BUTTON_ID_NO;
				}
				type_touch = 0;
				break;
			}
			
			case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
			{
				if ((pressed_buttons & SCE_CTRL_CANCEL) || (type_touch == 1))  {
					uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
				}
				type_touch = 0;
				break;
			}
		}
	}

	return uncommon_dialog.status;
}

int sceMsgDialogClose(void) {
	if (uncommon_dialog.status != SCE_COMMON_DIALOG_STATUS_RUNNING)
		return -1;

	uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSING;
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

float easeOut2(float x0, float x1, float a) {
	float dx = (x1 - x0);
	return ((dx * a) > 0.01f) ? (dx * a) : dx;
}

int drawUncommonDialog() {
	if (uncommon_dialog.status == SCE_COMMON_DIALOG_STATUS_NONE)
		return 0;

	// Dialog background
	vita2d_draw_texture_scale_rotate_hotspot(dialog_image, uncommon_dialog.x + uncommon_dialog.width / 2.0f,
														uncommon_dialog.y + uncommon_dialog.height / 2.0f,
														uncommon_dialog.scale * (uncommon_dialog.width / vita2d_texture_get_width(dialog_image)),
														uncommon_dialog.scale * (uncommon_dialog.height / vita2d_texture_get_height(dialog_image)),
														0.0f, vita2d_texture_get_width(dialog_image) / 2.0f, vita2d_texture_get_height(dialog_image) / 2.0f);

	// Easing out
	if (uncommon_dialog.animation_mode == UNCOMMON_DIALOG_CLOSING) {
		if (uncommon_dialog.scale > 0.0f) {
			uncommon_dialog.scale -= easeOut2(0.0f, uncommon_dialog.scale, 0.25f);
		} else {
			uncommon_dialog.animation_mode = UNCOMMON_DIALOG_CLOSED;
			uncommon_dialog.status = SCE_COMMON_DIALOG_STATUS_FINISHED;
		}
	}

	if (uncommon_dialog.animation_mode == UNCOMMON_DIALOG_OPENING) {
		if (uncommon_dialog.scale < 1.0f) {
			uncommon_dialog.scale += easeOut2(uncommon_dialog.scale, 1.0f, 0.25f);
		} else {
			uncommon_dialog.animation_mode = UNCOMMON_DIALOG_OPENED;
		}
	}

	if (uncommon_dialog.animation_mode == UNCOMMON_DIALOG_OPENED) {
		// Draw message
		float string_y = uncommon_dialog.y + SHELL_MARGIN_Y - 2.0f;

		int len = strlen(uncommon_dialog.msg);
		char *string = uncommon_dialog.msg;

		int i;
		for (i = 0; i < len + 1; i++) {
			if (uncommon_dialog.msg[i] == '\n') {
				uncommon_dialog.msg[i] = '\0';
				pgf_draw_text(uncommon_dialog.x + SHELL_MARGIN_X, string_y, GENERAL_COLOR, FONT_SIZE, string);
				uncommon_dialog.msg[i] = '\n';

				string = uncommon_dialog.msg + i + 1;
				string_y += FONT_Y_SPACE;
			}

			if (uncommon_dialog.msg[i] == '\0') {
				pgf_draw_text(uncommon_dialog.x + SHELL_MARGIN_X, string_y, GENERAL_COLOR, FONT_SIZE, string);
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
		//vita2d_draw_rectangle(x_d_start , y_d , x_d_end - x_d_start, y_d +  FONT_Y_SPACE, WHITE);

		// Progress bar
		if (uncommon_dialog.mode == SCE_MSG_DIALOG_MODE_PROGRESS_BAR) {
			float width = uncommon_dialog.width - 2.0f * SHELL_MARGIN_X;
			vita2d_draw_rectangle(uncommon_dialog.x + SHELL_MARGIN_X, string_y + 10.0f, width, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_BG_COLOR);
			vita2d_draw_rectangle(uncommon_dialog.x + SHELL_MARGIN_X, string_y + 10.0f, uncommon_dialog.progress * width / 100.0f, UNCOMMON_DIALOG_PROGRESS_BAR_HEIGHT, PROGRESS_BAR_COLOR);

			char string[8];
			sprintf(string, "%d%%", uncommon_dialog.progress);
			pgf_draw_text(CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, string)), string_y + FONT_Y_SPACE, GENERAL_COLOR, FONT_SIZE, string);

			string_y += 2.0f * FONT_Y_SPACE;
		}

		switch (uncommon_dialog.buttonType) {
			case SCE_MSG_DIALOG_BUTTON_TYPE_OK:
			case SCE_MSG_DIALOG_BUTTON_TYPE_YESNO:
			case SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL:
			case SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL:
				pgf_draw_text(CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, button_string)), string_y + FONT_Y_SPACE, GENERAL_COLOR, FONT_SIZE, button_string);
				break;
		}
		x_d_start = uncommon_dialog.x + uncommon_dialog.width;
		x_d_end = uncommon_dialog.x + uncommon_dialog.width + CENTER(SCREEN_WIDTH, vita2d_pgf_text_width(font, FONT_SIZE, string));
		y_d = uncommon_dialog.y + uncommon_dialog.height + string_y + FONT_Y_SPACE;
	
				
				
	}

	return 0;
}