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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

enum SettingsOptionType {
	SETTINGS_OPTION_TYPE_BOOLEAN,
	SETTINGS_OPTION_TYPE_INTEGER,
	SETTINGS_OPTION_TYPE_STRING,
};

enum SettingsMenuStatus {
	SETTINGS_MENU_CLOSED,
	SETTINGS_MENU_CLOSING,
	SETTINGS_MENU_OPENED,
	SETTINGS_MENU_OPENING,
};

void initSettingsMenu();
void openSettingsMenu();
int getSettingsMenuStatus();
void drawSettingsMenu();
void settingsMenuCtrl();

#endif