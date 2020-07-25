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

#include "qr.h"

#include <psp2/kernel/processmgr.h>
#include <psp2/kernel/threadmgr.h> 
#include <psp2/camera.h>

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <quirc.h>
#include <vita2d.h>

#include "main.h"
#include "io_process.h"
#include "network_download.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"

static int qr_enabled;

static struct quirc *qr;
static uint32_t* qr_data;
static char *data;
static int qr_next;

static vita2d_texture *camera_tex;

static SceCameraInfo cam_info;
static SceCameraRead cam_info_read;

static char last_qr[MAX_QR_LENGTH];
static char last_download[MAX_QR_LENGTH];
static int last_qr_len;
static int qr_scanned = 0;

static SceUID thid;

int qr_thread() {
  qr = quirc_new();
  quirc_resize(qr, CAM_WIDTH, CAM_HEIGHT);
  qr_next = 1;
  while (1) {
    sceKernelDelayThread(10);
    if (qr_next == 0 && qr_scanned == 0) {
      uint8_t *image;
      int w, h;
      image = quirc_begin(qr, &w, &h);
      uint32_t colourRGBA;
      int i;
      for (i = 0; i < w*h; i++) {
        colourRGBA = qr_data[i];
        image[i] = ((colourRGBA & 0x000000FF) + ((colourRGBA & 0x0000FF00) >> 8) + ((colourRGBA & 0x00FF0000) >> 16)) / 3;
      }
      quirc_end(qr);
      int num_codes = quirc_count(qr);
      if (num_codes > 0) {
        struct quirc_code code;
        struct quirc_data data;
        quirc_decode_error_t err;
        
        quirc_extract(qr, 0, &code);
        err = quirc_decode(&code, &data);
        if (err) {
        } else {
          memcpy(last_qr, data.payload, data.payload_len);
          last_qr_len = data.payload_len;
          qr_scanned = 1;
        }
      } else {
        memset(last_qr, 0, MAX_QR_LENGTH);
      }
      qr_next = 1;
      sceKernelDelayThread(250000);
    }
  }
}

int qr_scan_thread(SceSize args, void *argp) {
  data = last_qr;
  if (last_qr_len > 4) {
    if (!(data[0] == 'h' && data[1] == 't' && data[2] == 't' && data[3] == 'p')) {
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[QR_SHOW_CONTENTS], data);
      setDialogStep(DIALOG_STEP_QR_SHOW_CONTENTS);
      return sceKernelExitDeleteThread(0);
    }
  } else {
    initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[QR_SHOW_CONTENTS], data);
    setDialogStep(DIALOG_STEP_QR_SHOW_CONTENTS);
    return sceKernelExitDeleteThread(0);
  }
  
  initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_NONE, language_container[PLEASE_WAIT]);
  
  // check for attached file
  const char *headerData;
  unsigned int headerLen;
  unsigned int fileNameLength = 0;
  int vpk = 0;
  char *fileName = "";
  uint64_t fileSize;
  char sizeString[16];
  int ret;

  ret = getDownloadFileSize(data, &fileSize);
  if (ret < 0)
    goto NETWORK_FAILURE;

  ret = getFieldFromHeader(data, "Content-Disposition", &headerData, &headerLen);
  if (ret < 0)
    goto NETWORK_FAILURE;

  getSizeString(sizeString, fileSize);
  sceMsgDialogClose();

  // Wait for it to stop loading
  while (isMessageDialogRunning()) {
    sceKernelDelayThread(10 * 1000);
  }

  if (headerLen <= 0) {
    char *next;
    fileName = data;
    while ((next = strpbrk(fileName + 1, "\\/"))) fileName = next;
    if (fileName != last_qr) fileName++;
    
    char *ext = strrchr(fileName, '.');
    if (ext) {
      vpk = getFileType(fileName) == FILE_TYPE_VPK;
    } else {
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[QR_OPEN_WEBSITE], data);
      setDialogStep(DIALOG_STEP_QR_OPEN_WEBSITE);
      return sceKernelExitDeleteThread(0);
    }
  } else {
    if (strstr(headerData, "inline") != NULL) {
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[QR_OPEN_WEBSITE], data);
      setDialogStep(DIALOG_STEP_QR_OPEN_WEBSITE);
      return sceKernelExitDeleteThread(0);
    }

    char *p = strstr(headerData, "filename=");
    if (!p)
      goto EXIT;

    fileName = p+9;

    p = strchr(fileName, '\n');
    if (p)
      *p = '\0';

    // Trim at beginning
    while (*fileName < 0x20 ||
         *fileName == ' ' || *fileName == '\\' ||
         *fileName == '/' || *fileName == ':' ||
         *fileName == '*' || *fileName == '?' ||
         *fileName == '"' || *fileName == '<' ||
         *fileName == '>' || *fileName == '|') {
      fileName++;
    }

    // Trim at end
    int i;
    for (i = strlen(fileName)-1; i >= 0; i--) {
      if (fileName[i] < 0x20 ||
        fileName[i] == ' ' || fileName[i] == '\\' ||
        fileName[i] == '/' || fileName[i] == ':' ||
        fileName[i] == '*' || fileName[i] == '?' ||
        fileName[i] == '"' || fileName[i] == '<' ||
        fileName[i] == '>' || fileName[i] == '|') {
        fileName[i] = 0;
      } else {
        break;
      }
    }

    // VPK type
    vpk = getFileType(fileName) == FILE_TYPE_VPK;
  }
  
  if (vpk)
    initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[QR_CONFIRM_INSTALL], data, fileName, sizeString);
  else
    initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[QR_CONFIRM_DOWNLOAD], data, fileName, sizeString);
  setDialogStep(DIALOG_STEP_QR_CONFIRM);
  
  // Wait for response
  while (getDialogStep() == DIALOG_STEP_QR_CONFIRM) {
    sceKernelDelayThread(10 * 1000);
  }
  
  // No
  if (getDialogStep() == DIALOG_STEP_NONE) {
    goto EXIT;
  }
  
  // Yes
  char download_path[MAX_URL_LENGTH];
  char short_name[MAX_URL_LENGTH];
  int count = 0;
  
  char *ext = strrchr(fileName, '.');
  if (ext) {
    int len = ext-fileName;
    if (len > sizeof(short_name) - 1)
      len = sizeof(short_name) - 1;
    strncpy(short_name, fileName, len);
    short_name[len] = '\0';
  } else {
    strncpy(short_name, fileName, sizeof(short_name) - 1);
    ext = "";
  }
  while (1) {
    if (count == 0)
      snprintf(download_path, sizeof(download_path) - 1, "ux0:download/%s", fileName);
    else
      snprintf(download_path, sizeof(download_path) - 1, "ux0:download/%s (%d)%s", short_name, count, ext);

    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    if (sceIoGetstat(download_path, &stat) < 0)
      break;
    count++;
  }
  
  sceIoMkdir("ux0:download", 0006);
  
  strcpy(last_download, download_path);
  if (vpk)
    return downloadFileProcess(data, download_path, DIALOG_STEP_QR_DOWNLOADED_VPK);
  else
    return downloadFileProcess(data, download_path, DIALOG_STEP_QR_DOWNLOADED);

EXIT:
  return sceKernelExitDeleteThread(0);

NETWORK_FAILURE:
  sceMsgDialogClose();
  while (isMessageDialogRunning()) {
    sceKernelDelayThread(10 * 1000);
  }

  initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[QR_OPEN_WEBSITE], data);
  setDialogStep(DIALOG_STEP_QR_OPEN_WEBSITE);
  return sceKernelExitDeleteThread(0);
}

int initQR() {
  SceKernelMemBlockType orig = vita2d_texture_get_alloc_memblock_type();
  vita2d_texture_set_alloc_memblock_type(SCE_KERNEL_MEMBLOCK_TYPE_USER_CDRAM_RW);
  camera_tex = vita2d_create_empty_texture(CAM_WIDTH, CAM_HEIGHT);
  vita2d_texture_set_alloc_memblock_type(orig);
  
  cam_info.size = sizeof(SceCameraInfo);
  cam_info.format = SCE_CAMERA_FORMAT_ABGR;
  cam_info.resolution = SCE_CAMERA_RESOLUTION_640_360;
  cam_info.pitch = vita2d_texture_get_stride(camera_tex) - (CAM_WIDTH << 2);
  cam_info.sizeIBase = (CAM_WIDTH * CAM_HEIGHT) << 2;
  cam_info.pIBase = vita2d_texture_get_datap(camera_tex);
  cam_info.framerate = 30;
  
  cam_info_read.size = sizeof(SceCameraRead);
  cam_info_read.mode = 0;
  if (sceCameraOpen(1, &cam_info) < 0) {
    qr_enabled = 0;
    vita2d_free_texture(camera_tex);
    return -1;
  }
  
  thid = sceKernelCreateThread("qr_decode_thread", qr_thread, 0x40, 0x100000, 0, 0, NULL);
  if (thid >= 0) sceKernelStartThread(thid, 0, NULL);
  qr_enabled = 1;
  return 0;
}

int finishQR() {
  sceKernelDeleteThread(thid);
  vita2d_free_texture(camera_tex);
  sceCameraClose(1);
  quirc_destroy(qr);
  return 0;
}

int startQR() {
  return sceCameraStart(1);
}

int stopQR() {
  int res = sceCameraStop(1);

  int y;
  for (y = 0; y < CAM_HEIGHT; y++) {
    int x;
    for (x = 0; x < CAM_WIDTH; x++) {
      ((uint32_t *)qr_data)[x + CAM_WIDTH * y] = 0;
    }
  }

  return res;
}

int renderCameraQR(int x, int y) {  
  sceCameraRead(1, &cam_info_read);
  vita2d_draw_texture(camera_tex, x, y);
  if (qr_next) {
    qr_data = (uint32_t *)vita2d_texture_get_datap(camera_tex);
    qr_next = 0;
  }
  return 0;
} 

char *getLastQR() {
  return data;
}

char *getLastDownloadQR() {
  return last_download;
}

int scannedQR() {
  return qr_scanned;
}

void setScannedQR(int s) {
  qr_scanned = s;
}

int enabledQR() {
  return qr_enabled;
}
