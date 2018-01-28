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
#include "network_download.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"

#define VITASHELL_USER_AGENT "VitaShell/1.00 libhttp/1.1"

int getDownloadFileSize(const char *src, uint64_t *size) {
  int res;
  int statusCode;
  int tmplId = -1, connId = -1, reqId = -1;

  res = sceHttpCreateTemplate(VITASHELL_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  tmplId = res;

  res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  connId = res;

  res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
  if (res < 0)
    goto ERROR_EXIT;

  reqId = res;

  res = sceHttpSendRequest(reqId, NULL, 0);
  if (res < 0)
    goto ERROR_EXIT;

  res = sceHttpGetStatusCode(reqId, &statusCode);
  if (res < 0)
    goto ERROR_EXIT;

  if (statusCode == 200) {
    res = sceHttpGetResponseContentLength(reqId, size);
  }

ERROR_EXIT:
  if (reqId >= 0)
    sceHttpDeleteRequest(reqId);

  if (connId >= 0)
    sceHttpDeleteConnection(connId);

  if (tmplId >= 0)
    sceHttpDeleteTemplate(tmplId);

  return res;
}

int getFieldFromHeader(const char *src, const char *field, const char **data, unsigned int *valueLen) {
  int res;
  char *header;
  unsigned int headerSize;
  int tmplId = -1, connId = -1, reqId = -1;

  res = sceHttpCreateTemplate(VITASHELL_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  tmplId = res;

  res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  connId = res;
  
  res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src,  0);
  if (res < 0)
    goto ERROR_EXIT;
  
  reqId = res;
  
  res = sceHttpSendRequest(reqId, NULL, 0);
  if (res < 0)
    goto ERROR_EXIT;

  res = sceHttpGetAllResponseHeaders(reqId, &header, &headerSize);
  if (res < 0)
    goto ERROR_EXIT;

  res = sceHttpParseResponseHeader(header, headerSize, field, data, valueLen);
  if (res < 0) {
    *data = "";
    *valueLen = 0;
    res = 0;
  }
  
ERROR_EXIT:
  if (reqId >= 0)
    sceHttpDeleteRequest(reqId);

  if (connId >= 0)
    sceHttpDeleteConnection(connId);

  if (tmplId >= 0)
    sceHttpDeleteTemplate(tmplId);

  return res;
}

int downloadFile(const char *src, const char *dst, FileProcessParam *param) {
  int res;
  int statusCode;
  int tmplId = -1, connId = -1, reqId = -1;
  SceUID fd = -1;
  int ret = 1;

  res = sceHttpCreateTemplate(VITASHELL_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  tmplId = res;

  res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
  if (res < 0)
    goto ERROR_EXIT;

  connId = res;

  res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
  if (res < 0)
    goto ERROR_EXIT;

  reqId = res;

  res = sceHttpSendRequest(reqId, NULL, 0);
  if (res < 0)
    goto ERROR_EXIT;

  res = sceHttpGetStatusCode(reqId, &statusCode);
  if (res < 0)
    goto ERROR_EXIT;

  if (statusCode == 200) {    
    res = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
    if (res < 0)
      goto ERROR_EXIT;

    fd = res;

    uint8_t buf[4096];

    while (1) {
      int read = sceHttpReadData(reqId, buf, sizeof(buf));
      
      if (read < 0) {
        res = read;
        break;
      }
      
      if (read == 0)
        break;

      int written = sceIoWrite(fd, buf, read);
      
      if (written < 0) {
        res = written;
        break;
      }

      if (param) {
        if (param->value)
          (*param->value) += read;

        if (param->SetProgress)
          param->SetProgress(param->value ? *param->value : 0, param->max);

        if (param->cancelHandler && param->cancelHandler()) {
          ret = 0;
          break;
        }
      }
    }
  }

ERROR_EXIT:
  if (fd >= 0)
    sceIoClose(fd);

  if (reqId >= 0)
    sceHttpDeleteRequest(reqId);

  if (connId >= 0)
    sceHttpDeleteConnection(connId);

  if (tmplId >= 0)
    sceHttpDeleteTemplate(tmplId);

  if (res < 0)
    return res;

  return ret;
}

int downloadFileProcess(const char *url, const char *dest, int successStep) {
  SceUID thid = -1;

  // Lock power timers
  powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  // File size
  uint64_t size = 0;
  getDownloadFileSize(url, &size);

  // Update thread
  thid = createStartUpdateThread(size, 1);

  // Download
  uint64_t value = 0;
  
  FileProcessParam param;
  param.value = &value;
  param.max = size;
  param.SetProgress = SetProgress;
  param.cancelHandler = cancelHandler;

  int res = downloadFile(url, dest, &param);
  if (res <= 0) {
    sceIoRemove(dest);
    closeWaitDialog();
    setDialogStep(DIALOG_STEP_CANCELED);
    errorDialog(res);
    goto EXIT;
  }

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);
  
  // Close
  if (successStep != 0) {
    sceMsgDialogClose();
    setDialogStep(successStep);
  }

EXIT:
  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);
  
  // Unlock power timers
  powerUnlock();

  return sceKernelExitDeleteThread(0);
}