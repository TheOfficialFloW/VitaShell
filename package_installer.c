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
#include "io_process.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"
#include "sha1.h"

INCLUDE_EXTERN_RESOURCE(head_bin);

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

int promotePsm(const char *path, const char *titleid) {
  int res;
  
  ScePromoterUtilityImportParams promoteArgs;
  memset(&promoteArgs,0x00,sizeof(ScePromoterUtilityImportParams));
  strncpy(promoteArgs.path,path,0x7F);
  strncpy(promoteArgs.titleid,titleid,0xB);
  promoteArgs.type = SCE_PKG_TYPE_PSM;
  promoteArgs.attribute = 0x1;

  res = loadScePaf();
  if (res < 0)
    return res;

  res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = scePromoterUtilityInit();
  if (res < 0)
    return res;

  res = scePromoterUtilityPromoteImport(&promoteArgs);
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

int deleteApp(const char *titleid) {
  int res;

  sceAppMgrDestroyOtherApp();

  res = loadScePaf();
  if (res < 0)
    return res;

  res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = scePromoterUtilityInit();
  if (res < 0)
    return res;

  res = scePromoterUtilityDeletePkg(titleid);
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

int checkAppExist(const char *titleid) {
  int res;
  int ret;

  res = loadScePaf();
  if (res < 0)
    return res;

  res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_INTERNAL_PROMOTER_UTIL);
  if (res < 0)
    return res;

  res = scePromoterUtilityInit();
  if (res < 0)
    return res;

  ret = scePromoterUtilityCheckExist(titleid, &res);
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

  return ret >= 0;
}

static void fpkg_hmac(const uint8_t *data, unsigned int len, uint8_t hmac[16]) {
  SHA1_CTX ctx;
  uint8_t sha1[20];
  uint8_t buf[64];

  sha1_init(&ctx);
  sha1_update(&ctx, data, len);
  sha1_final(&ctx, sha1);

  memset(buf, 0, 64);
  memcpy(&buf[0], &sha1[4], 8);
  memcpy(&buf[8], &sha1[4], 8);
  memcpy(&buf[16], &sha1[12], 4);
  buf[20] = sha1[16];
  buf[21] = sha1[1];
  buf[22] = sha1[2];
  buf[23] = sha1[3];
  memcpy(&buf[24], &buf[16], 8);

  sha1_init(&ctx);
  sha1_update(&ctx, buf, 64);
  sha1_final(&ctx, sha1);
  memcpy(hmac, sha1, 16);
}

int makeHeadBin() {
  uint8_t hmac[16];
  uint32_t off;
  uint32_t len;
  uint32_t out;

  SceIoStat stat;
  memset(&stat, 0, sizeof(SceIoStat));

  if (checkFileExist(HEAD_BIN))
    return 0;

  // Read param.sfo
  void *sfo_buffer = NULL;
  int res = allocateReadFile(PACKAGE_DIR "/sce_sys/param.sfo", &sfo_buffer);
  if (res < 0)
    return res;

  // Get title id
  char titleid[12];
  memset(titleid, 0, sizeof(titleid));
  getSfoString(sfo_buffer, "TITLE_ID", titleid, sizeof(titleid));

  // Enforce TITLE_ID format
  if ((strlen(titleid) != 9) || (strncmp(titleid, strupr(titleid), 9) != 0))
    return VITASHELL_ERROR_INVALID_TITLEID;

  // Get content id
  char contentid[48];
  memset(contentid, 0, sizeof(contentid));
  getSfoString(sfo_buffer, "CONTENT_ID", contentid, sizeof(contentid));

  // Free sfo buffer
  free(sfo_buffer);

  // Allocate head.bin buffer
  uint8_t *head_bin = malloc((int)&_binary_resources_head_bin_size);
  memcpy(head_bin, (void *)&_binary_resources_head_bin_start, (int)&_binary_resources_head_bin_size);

  // Write full title id
  char full_title_id[48];
  snprintf(full_title_id, sizeof(full_title_id), "EP9000-%s_00-0000000000000000", titleid);
  strncpy((char *)&head_bin[0x30], strlen(contentid) > 0 ? contentid : full_title_id, 48);

  // hmac of pkg header
  len = ntohl(*(uint32_t *)&head_bin[0xD0]);
  fpkg_hmac(&head_bin[0], len, hmac);
  memcpy(&head_bin[len], hmac, 16);

  // hmac of pkg info
  off = ntohl(*(uint32_t *)&head_bin[0x8]);
  len = ntohl(*(uint32_t *)&head_bin[0x10]);
  out = ntohl(*(uint32_t *)&head_bin[0xD4]);
  fpkg_hmac(&head_bin[off], len-64, hmac);
  memcpy(&head_bin[out], hmac, 16);

  // hmac of everything
  len = ntohl(*(uint32_t *)&head_bin[0xE8]);
  fpkg_hmac(&head_bin[0], len, hmac);
  memcpy(&head_bin[len], hmac, 16);

  // Make dir
  sceIoMkdir(PACKAGE_DIR "/sce_sys/package", 0777);

  // Write head.bin
  WriteFile(HEAD_BIN, head_bin, (int)&_binary_resources_head_bin_size);

  free(head_bin);

  return 0;
}

int installPackage(const char *file) {
  int res;

  // Recursively clean up pkg directory
  removePath(PACKAGE_DIR, NULL);

  // Open archive
  archiveClearPassword();
  res = archiveOpen(file);
  if (res < 0)
    return res;

  // Src path
  char src_path[MAX_PATH_LENGTH];
  strcpy(src_path, file);
  addEndSlash(src_path);

  // Extract process
  res = extractArchivePath(src_path, PACKAGE_DIR "/", NULL);
  if (res < 0)
    return res;

  // Close archive
  res = archiveClose();
  if (res < 0)
    return res;

  // Make head.bin
  res = makeHeadBin();
  if (res < 0)
    return res;

  // Promote app
  res = promoteApp(PACKAGE_DIR);
  if (res < 0)
    return res;

  return 0;
}

int install_thread(SceSize args_size, InstallArguments *args) {
  int res;
  SceUID thid = -1;
  char path[MAX_PATH_LENGTH];
  SceIoStat stat;
  int isFolder = 0;

  // Lock power timers
  powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  // Recursively clean up pkg directory
  removePath(PACKAGE_DIR, NULL);

  res = sceIoGetstat(args->file, &stat);
  if (res < 0) {
    closeWaitDialog();
    errorDialog(res);
    goto EXIT;
  }

  if (SCE_S_ISDIR(stat.st_mode)) {
    // Check for param.sfo
    snprintf(path, MAX_PATH_LENGTH, "%s/sce_sys/param.sfo", args->file);
    if (sceIoGetstat(path, &stat) < 0 || SCE_S_ISDIR(stat.st_mode)) {
      closeWaitDialog();
      errorDialog(-2);
      goto EXIT;
    }

    // Check permissions
    snprintf(path, MAX_PATH_LENGTH, "%s/eboot.bin", args->file);
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
    if (fd >= 0) {
      char buffer[0x88];
      sceIoRead(fd, buffer, sizeof(buffer));
      sceIoClose(fd);

      // Team molecule's request: Full permission access warning
      uint64_t authid = *(uint64_t *)(buffer + 0x80);
      if (!vitashell_config.disable_warning && authid != 0x2F00000000000002) {
        closeWaitDialog();

        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_WARNING]);
        setDialogStep(DIALOG_STEP_INSTALL_WARNING);

        // Wait for response
        while (getDialogStep() == DIALOG_STEP_INSTALL_WARNING) {
          sceKernelDelayThread(10 * 1000);
        }

        // Canceled
        if (getDialogStep() == DIALOG_STEP_CANCELED) {
          closeWaitDialog();
          goto EXIT;
        }

        // Init again
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
        setDialogStep(DIALOG_STEP_INSTALLING);
      }
    }

    res = sceIoRename(args->file, PACKAGE_DIR);
    if (res < 0) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELED);
      errorDialog(res);
      goto EXIT;
    }
    sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 50);
    sceKernelDelayThread(COUNTUP_WAIT);

    isFolder = 1;
  } else {
    // Open archive
    archiveClearPassword();
    res = archiveOpen(args->file);
    if (res < 0) {
      closeWaitDialog();
      errorDialog(res);
      goto EXIT;
    }

    // If you canceled at the time archiveOpen was working,
    // it would still open the full permission dialog instead of termiating.
    // So terminate now
    if (cancelHandler()) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELED);
      goto EXIT;
    }

    // Check for param.sfo
    snprintf(path, MAX_PATH_LENGTH, "%s/sce_sys/param.sfo", args->file);
    if (archiveFileGetstat(path, NULL) < 0) {
      closeWaitDialog();
      errorDialog(-2);
      goto EXIT;
    }

    // Team molecule's request: Full permission access warning
    int unsafe = archiveCheckFilesForUnsafeFself(); // 0: Safe, 1: Unsafe, 2: Dangerous
    if (!vitashell_config.disable_warning && unsafe) {
      closeWaitDialog();

      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[unsafe == 2 ? INSTALL_BRICK_WARNING : INSTALL_WARNING]);
      setDialogStep(DIALOG_STEP_INSTALL_WARNING);

      // Wait for response
      while (getDialogStep() == DIALOG_STEP_INSTALL_WARNING) {
        sceKernelDelayThread(10 * 1000);
      }

      // Canceled
      if (getDialogStep() == DIALOG_STEP_CANCELED) {
        closeWaitDialog();
        goto EXIT;
      }

      // Init again
      initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
      setDialogStep(DIALOG_STEP_INSTALLING);
    }

    // Src path
    char src_path[MAX_PATH_LENGTH];
    strcpy(src_path, args->file);
    addEndSlash(src_path);

    // Get archive path info
    uint64_t size = 0;
    uint32_t folders = 0, files = 0;
    getArchivePathInfo(src_path, &size, &folders, &files, NULL);

    // Check memory card free space
    if (checkMemoryCardFreeSpace(PACKAGE_DIR, size))
      goto EXIT;

    // Update thread
    thid = createStartUpdateThread(size + folders*DIRECTORY_SIZE, 1);

    // Extract process
    uint64_t value = 0;

    FileProcessParam param;
    param.value = &value;
    param.max = size + folders * DIRECTORY_SIZE;
    param.SetProgress = SetProgress;
    param.cancelHandler = cancelHandler;

    res = extractArchivePath(src_path, PACKAGE_DIR "/", &param);
    if (res <= 0) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELED);
      errorDialog(res);
      goto EXIT;
    }

    // Close archive
    res = archiveClose();
    if (res < 0) {
      closeWaitDialog();
      errorDialog(res);
      goto EXIT;
    }
  }

  // Make head.bin
  res = makeHeadBin();
  if (res < 0) {
    closeWaitDialog();
    errorDialog(res);
    // If failed, move package folder back
    if (isFolder) sceIoRename(PACKAGE_DIR, args->file);
    goto EXIT;
  }

  // Promote app
  res = promoteApp(PACKAGE_DIR);
  if (res < 0) {
    closeWaitDialog();
    errorDialog(res);
    // If failed, move package folder back
    if (isFolder) sceIoRename(PACKAGE_DIR, args->file);
    goto EXIT;
  }

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);

  // Close
  sceMsgDialogClose();

  setDialogStep(DIALOG_STEP_INSTALLED);

EXIT:
  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);

  // Recursively clean up package_temp directory
  removePath(PACKAGE_DIR, NULL);

  // Unlock power timers
  powerUnlock();

  return sceKernelExitDeleteThread(0);
}
