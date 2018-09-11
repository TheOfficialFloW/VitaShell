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

#ifndef __VITASHELL_CONFIGURATION_H__
#define __VITASHELL_CONFIGURATION_H__

enum UsbDeviceModes {
  USBDEVICE_MODE_MEMORY_CARD,
  USBDEVICE_MODE_GAME_CARD,
  USBDEVICE_MODE_SD2VITA,
  USBDEVICE_MODE_PSVSD,
};

enum SelectButtonModes {
  SELECT_BUTTON_MODE_USB,
  SELECT_BUTTON_MODE_FTP,
};

typedef struct {
  int usbdevice;
  int select_button;
  int disable_autoupdate;
  int disable_warning;
} VitaShellConfig;

#endif
