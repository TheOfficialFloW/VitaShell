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

#ifndef __QR_H__
#define __QR_H__

#include <psp2/kernel/sysmem.h>

int qr_scan_thread(SceSize args, void *argp);

int initQR();
int finishQR();
int startQR();
int stopQR();
int enabledQR();
int scannedQR();
int renderCameraQR(int x, int y);
char *getLastQR();
char *getLastDownloadQR();
void setScannedQR(int scanned);

#endif
