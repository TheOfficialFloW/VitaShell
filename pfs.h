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

#ifndef __PFS_H__
#define __PFS_H__

char pfs_mounted_path[MAX_PATH_LENGTH];
char pfs_mount_point[MAX_MOUNT_POINT_LENGTH];
int read_only;

int known_pfs_ids[4];
int pfsMount(const char *path);
int pfsUmount();

#endif
