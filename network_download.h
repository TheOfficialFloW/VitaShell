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

/*
 *  network_download
 *  same as network_update.h // network_update.c 
 *  just modified to download any file from url, destination and successStep (0 for no step)
 */

#ifndef __NETWORK_DOWNLOAD_H__
#define __NETWORK_DOWNLOAD_H__

int getDownloadFileSize(const char *src, uint64_t *size);
int getFieldFromHeader(const char *src, const char *field, const char **data, unsigned int *valueLen);
int downloadFile(const char *src, const char *dst, FileProcessParam *param);
int downloadFileProcess(const char *url, const char *dest, int successStep);

#endif
