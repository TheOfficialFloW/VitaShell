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

#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/sysmodule.h>
#include <psp2/promoterutil.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define PACKAGE_DIR "ux0:data/pkg"
#define HEAD_BIN PACKAGE_DIR "/sce_sys/package/head.bin"

typedef struct SfoHeader {
  uint32_t magic;
  uint32_t version;
  uint32_t keyofs;
  uint32_t valofs;
  uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
  uint16_t nameofs;
  uint8_t  alignment;
  uint8_t  type;
  uint32_t valsize;
  uint32_t totalsize;
  uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

int launchAppByUriExit(char *titleid) {
  char uri[32];
  sprintf(uri, "psgm:play?titleid=%s", titleid);

  sceAppMgrLaunchAppByUri(0xFFFFF, uri);
  sceKernelExitProcess(0);

  return 0;
}

static int loadScePaf() {
  static uint32_t argp[] = { 0x180000, -1, -1, 1, -1, -1 };

  int result = -1;

  uint32_t buf[4];
  buf[0] = sizeof(buf);
  buf[1] = (uint32_t)&result;
  buf[2] = -1;
  buf[3] = -1;

  return sceSysmoduleLoadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, sizeof(argp), argp, buf);
}

static int unloadScePaf() {
  uint32_t buf = 0;
  return sceSysmoduleUnloadModuleInternalWithArg(SCE_SYSMODULE_INTERNAL_PAF, 0, NULL, &buf);
}

int promoteApp(const char *path) {
  int res;

  res = loadScePaf();
  if (res < 0)
    return res;

  res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = scePromoterUtilityInit();
  if (res < 0)
    return res;

  res = scePromoterUtilityPromotePkgWithRif(path, 1);
  if (res < 0)
    return res;

  res = scePromoterUtilityExit();
  if (res < 0)
    return res;

  res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = unloadScePaf();
  if (res < 0)
    return res;

  return res;
}

char *get_title_id(const char *filename) {
  char *res = NULL;
  long size = 0;
  FILE *fin = NULL;
  char *buf = NULL;
  int i;

  SfoHeader *header;
  SfoEntry *entry;
  
  fin = fopen(filename, "rb");
  if (!fin)
    goto cleanup;
  if (fseek(fin, 0, SEEK_END) != 0)
    goto cleanup;
  if ((size = ftell(fin)) == -1)
    goto cleanup;
  if (fseek(fin, 0, SEEK_SET) != 0)
    goto cleanup;
  buf = calloc(1, size + 1);
  if (!buf)
    goto cleanup;
  if (fread(buf, size, 1, fin) != 1)
    goto cleanup;

  header = (SfoHeader*)buf;
  entry = (SfoEntry*)(buf + sizeof(SfoHeader));
  for (i = 0; i < header->count; ++i, ++entry) {
    const char *name = buf + header->keyofs + entry->nameofs;
    const char *value = buf + header->valofs + entry->dataofs;
    if (name >= buf + size || value >= buf + size)
      break;
    if (strcmp(name, "TITLE_ID") == 0)
      res = strdup(value);
  }

cleanup:
  if (buf)
    free(buf);
  if (fin)
    fclose(fin);

  return res;
}

int main(int argc, const char *argv[]) {
  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  char *titleid = get_title_id(PACKAGE_DIR "/sce_sys/param.sfo");
  if (titleid && strcmp(titleid, "VITASHELL") == 0) {
    promoteApp(PACKAGE_DIR);
  }

  launchAppByUriExit("VITASHELL");

  return 0;
}
