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

#ifndef __MESSAGE_DIALOG_H__
#define __MESSAGE_DIALOG_H__

#define MESSAGE_DIALOG_RESULT_NONE 0
#define MESSAGE_DIALOG_RESULT_RUNNING 1
#define MESSAGE_DIALOG_RESULT_FINISHED 2
#define MESSAGE_DIALOG_RESULT_NO 3
#define MESSAGE_DIALOG_RESULT_YES 4

#define MESSAGE_DIALOG_PROGRESS_BAR 5

int initMessageDialog(int type, char *msg, ...);
int isMessageDialogRunning();
int updateMessageDialog();

#endif