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
#include "netcheck_dialog.h"

static int netcheck_dialog_running = 0;

int initNetCheckDialog(int mode, int timeoutUs) {
  if (netcheck_dialog_running)
    return -1;

  SceNetCheckDialogParam param;
  sceNetCheckDialogParamInit(&param);

  SceNetAdhocctlGroupName groupName;
  memset(groupName.data, 0, SCE_NET_ADHOCCTL_GROUPNAME_LEN);
  param.groupName = &groupName;
  memcpy(&param.npCommunicationId.data, VITASHELL_TITLEID, 9);
  param.npCommunicationId.term = '\0';
  param.npCommunicationId.num = 0;
  param.mode = mode;
  param.timeoutUs = timeoutUs;

  int res = sceNetCheckDialogInit(&param);
  if (res >= 0) {
    netcheck_dialog_running = 1;
  }

  return res;
}

int isNetCheckDialogRunning() {
  return netcheck_dialog_running;  
}

int updateNetCheckDialog() {
  if (!netcheck_dialog_running)
    return NETCHECK_DIALOG_RESULT_NONE;

  SceCommonDialogStatus status = sceNetCheckDialogGetStatus();
  if (status == NETCHECK_DIALOG_RESULT_FINISHED) {
    SceNetCheckDialogResult result;
    memset(&result, 0, sizeof(SceNetCheckDialogResult));
    sceNetCheckDialogGetResult(&result);

    if (result.result == SCE_COMMON_DIALOG_RESULT_OK) {
      status = NETCHECK_DIALOG_RESULT_CONNECTED;
    } else {
      status = NETCHECK_DIALOG_RESULT_NOT_CONNECTED;
    }

    sceNetCheckDialogTerm();

    netcheck_dialog_running = 0;
  }

  return status;
}