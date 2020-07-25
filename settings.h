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

#ifndef __SETTINGS_H__
#define __SETTINGS_H__

#define MAX_THEMES 64
#define MAX_THEME_LENGTH 64

enum SettingsAgreement {
  SETTINGS_AGREEMENT_NONE,
  SETTINGS_AGREEMENT_AGREE,
  SETTINGS_AGREEMENT_DISAGREE,
};

enum SettingsOptionType {
  SETTINGS_OPTION_TYPE_BOOLEAN,
  SETTINGS_OPTION_TYPE_INTEGER,
  SETTINGS_OPTION_TYPE_STRING,
  SETTINGS_OPTION_TYPE_CALLBACK,
  SETTINGS_OPTION_TYPE_OPTIONS,
};

enum SettingsMenuStatus {
  SETTINGS_MENU_CLOSED,
  SETTINGS_MENU_CLOSING,
  SETTINGS_MENU_OPENED,
  SETTINGS_MENU_OPENING,
};

typedef struct {
  int status;
  float cur_pos;
  int entry_sel;
  int option_sel;
  int n_options;
} SettingsMenu;

typedef struct {
  int name;
  int type;
  int (* callback)();
  char *string;
  int size_string;
  char **options;
  int n_options;
  int *value;
} SettingsMenuOption;

typedef struct {
  int name;
  SettingsMenuOption *options;
  int n_options;
} SettingsMenuEntry;

void loadSettingsConfig();
void saveSettingsConfig();

void initSettingsMenu();
void openSettingsMenu();
void closeSettingsMenu();
int getSettingsMenuStatus();
void drawSettingsMenu();
void settingsMenuCtrl();

void settingsAgree();
void settingsDisagree();

#endif
