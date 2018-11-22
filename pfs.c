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

#include "main.h"
#include "pfs.h"

/*
  SceAppMgr mount IDs:
  0x64: ux0:picture
  0x65: ur0:user/00/psnfriend
  0x66: ur0:user/00/psnmsg
  0x69: ux0:music
  0x6E: ux0:appmeta
  0xC8: ur0:temp/sqlite
  0xCD: ux0:cache
  0x12E: ur0:user/00/trophy/data/sce_trop
  0x12F: ur0:user/00/trophy/data
  0x3E8: ux0:app, vs0:app, gro0:app
  0x3E9: ux0:patch
  0x3EB: ?
  0x3EA: ux0:addcont
  0x3EC: ux0:theme
  0x3ED: ux0:user/00/savedata
  0x3EE: ur0:user/00/savedata
  0x3EF: vs0:sys/external
  0x3F0: vs0:data/external
*/

int known_pfs_ids[] = {
  0x6E,
  0x12E,
  0x12F,
  0x3ED,
};

int pfsMount(const char *path) {
  int res;
  char work_path[MAX_PATH_LENGTH];
  char klicensee[0x10];
  char license_buf[0x200];
  ShellMountIdArgs args;

  memset(klicensee, 0, sizeof(klicensee));

/*
  snprintf(work_path, MAX_PATH_LENGTH, "%ssce_sys/package/work.bin", path);
  if (ReadFile(work_path, license_buf, sizeof(license_buf)) == sizeof(license_buf)) {
    int res = shellUserGetRifVitaKey(license_buf, klicensee);
    debugPrintf("read license: 0x%08X\n", res);
  }
*/
  args.process_titleid = VITASHELL_TITLEID;
  args.path = path;
  args.desired_mount_point = NULL;
  args.klicensee = klicensee;
  args.mount_point = pfs_mount_point;

  read_only = 0;

  int i;
  for (i = 0; i < sizeof(known_pfs_ids) / sizeof(int); i++) {
    args.id = known_pfs_ids[i];

    res = shellUserMountById(&args);
    if (res >= 0)
      return res;
  }

  read_only = 1;
  return sceAppMgrGameDataMount(path, 0, 0, pfs_mount_point);
}

int pfsUmount() {
  if (pfs_mount_point[0] == 0)
    return -1;

  int res = sceAppMgrUmount(pfs_mount_point);
  if (res >= 0) {
    memset(pfs_mount_point, 0, sizeof(pfs_mount_point));
    memset(pfs_mounted_path, 0, sizeof(pfs_mounted_path));
  }

  return res;
}