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

#ifndef __VITASHELL_USER_H__
#define __VITASHELL_USER_H__

#include "vitashell_kernel.h"

int shellUserIsUx0Redirected(const char *blkdev, const char *blkdev2);
int shellUserRedirectUx0(const char *blkdev, const char *blkdev2);
int shellUserMountById(ShellMountIdArgs *args);
int shellUserGetRifVitaKey(const void *license_buf, void *klicensee);

#endif
