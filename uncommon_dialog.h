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

#ifndef __UNCOMMON_DIALOG_H__
#define __UNCOMMON_DIALOG_H__

#define CROSS "\xE2\x95\xB3"
#define SQUARE "\xE2\x96\xA1"
#define TRIANGLE "\xE2\x96\xB3"
#define CIRCLE "\xE2\x97\x8B"

enum UncommonDialogStatus {
	UNCOMMON_DIALOG_CLOSED,
	UNCOMMON_DIALOG_CLOSING,
	UNCOMMON_DIALOG_OPENED,
	UNCOMMON_DIALOG_OPENING,
};

int drawUncommonDialog();

#endif