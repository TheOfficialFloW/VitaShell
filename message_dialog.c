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
#include "message_dialog.h"

static int message_dialog_running = 0;
static int message_dialog_type = -1;

static char message_string[512];

int initMessageDialog(int type, char *msg, ...) {
	if (message_dialog_running)
		return -1;

	va_list list;
	char string[512];

	va_start(list, msg);
	vsprintf(string, msg, list);
	va_end(list);

	strcpy(message_string, string);

	SceMsgDialogParam param;
	sceMsgDialogParamInit(&param);

	if (type == MESSAGE_DIALOG_PROGRESS_BAR) {
		static SceMsgDialogProgressBarParam progress_bar_param;
		memset(&progress_bar_param, 0, sizeof(SceMsgDialogProgressBarParam));
		progress_bar_param.barType = SCE_MSG_DIALOG_PROGRESSBAR_TYPE_PERCENTAGE;
		progress_bar_param.msg = (SceChar8 *)message_string;

		param.progBarParam = &progress_bar_param;
		param.mode = SCE_MSG_DIALOG_MODE_PROGRESS_BAR;
	} else {
		static SceMsgDialogUserMessageParam user_message_param;
		memset(&user_message_param, 0, sizeof(SceMsgDialogUserMessageParam));
		user_message_param.msg = (SceChar8 *)message_string;
		user_message_param.buttonType = type;

		param.userMsgParam = &user_message_param;
		param.mode = SCE_MSG_DIALOG_MODE_USER_MSG;
	}

	int res = sceMsgDialogInit(&param);
	if (res >= 0) {
		message_dialog_running = 1;
		message_dialog_type = type;
	}

	return 0;
}

int isMessageDialogRunning() {
	return message_dialog_running;
}

int updateMessageDialog() {
	if (!message_dialog_running)
		return MESSAGE_DIALOG_RESULT_NONE;

	SceCommonDialogStatus status = sceMsgDialogGetStatus();

	if (status == MESSAGE_DIALOG_RESULT_FINISHED) {
		if (message_dialog_type == SCE_MSG_DIALOG_BUTTON_TYPE_YESNO || message_dialog_type == SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL) {
			SceMsgDialogResult result;
			memset(&result, 0, sizeof(SceMsgDialogResult));
			sceMsgDialogGetResult(&result);

			if (result.buttonId == SCE_MSG_DIALOG_BUTTON_ID_NO) {
				status = MESSAGE_DIALOG_RESULT_NO;
			} else if (result.buttonId == SCE_MSG_DIALOG_BUTTON_ID_YES) {
				status = MESSAGE_DIALOG_RESULT_YES;
			}
		}

		sceMsgDialogTerm();

		message_dialog_running = 0;
	}

	return status;
}