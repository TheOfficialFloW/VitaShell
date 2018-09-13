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

#ifndef __USB_H__
#define __USB_H__

int mountGamecardUx0();
int umountGamecardUx0();

int mountUsbUx0();
int umountUsbUx0();

SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type);
int stopUsb(SceUID modid);

#endif
