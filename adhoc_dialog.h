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

#ifndef __ADHOC_DIALOG_H__
#define __ADHOC_DIALOG_H__

#define ADHOC_DIALOG_RESULT_NONE 0
#define ADHOC_DIALOG_RESULT_WAITING_FOR_RESPONSE 1

enum AdhocDialogStatus {
  ADHOC_DIALOG_CLOSED,
  ADHOC_DIALOG_CLOSING,
  ADHOC_DIALOG_OPENED,
  ADHOC_DIALOG_OPENING,
};

typedef struct {
  FileList *file_list;
  FileList *mark_list;
  int index;
} SendArguments;

typedef struct {
  FileList *file_list;
  FileList *mark_list;
  int index;
} ReceiveArguments;

int send_thread(SceSize args_size, SendArguments *args);
int receive_thread(SceSize args_size, ReceiveArguments *args);

void adhocAlertSockets();
void adhocCloseSockets();
int adhocUpdatePeerList();
int adhocSendServerResponse(char *response);
char *adhocReceiveClientReponse();
int adhocReceiveServerRequest();
void adhocWaitingForServerRequest();
char *adhocGetServerNickname();

int getAdhocDialogStatus();
int initAdhocDialog();
void adhocDialogCtrl();
void drawAdhocDialog();

#endif