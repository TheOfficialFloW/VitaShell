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
#include "init.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "adhoc_dialog.h"
#include "message_dialog.h"
#include "uncommon_dialog.h"
#include "io_process.h"

#define RECEIVE_WAIT 200 * 1000

typedef struct {
  int status;
  int result;
  int sel;
  float x;
  float y;
  float width;
  float height;
  float scale;
} AdhocDialog;

#define PEERLIST_MAX 4

static AdhocDialog adhoc_dialog;

static SceNetAdhocctlPeerInfo  peer_list[PEERLIST_MAX];
static int peer_count = 0;

static SceNetAdhocctlPeerInfo server_peer_info;
static SceNetEtherAddr client_addr, server_addr;
static int client_socket = -1, server_socket = -1;
static SceUID client_waiting_thid = -1;
static int server_request_result = 0;
static char client_response[4];

#define SHARE_MAGIC 0x574F4C46
#define SHARE_TYPE_FOLDER 0
#define SHARE_TYPE_FILE 1

#define SHARE_SIZE 1024
#define SERVER_PORT 1

typedef struct {
  int magic;
  int type;
  size_t path_len;
  uint64_t file_size;
} ShareInfo;

int adhocSend(int socket, const void *buf, int size) {
  return sceNetAdhocPtpSend(socket, buf, &size, 0, 0);
}

int adhocRecv(int socket, void *buf, int *size) {
  return sceNetAdhocPtpRecv(socket, buf, size, 0, SCE_NET_ADHOC_F_NONBLOCK);
}

int sendFile(const char *src_path, FileProcessParam *param) {

  SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
  if (fdsrc < 0)
    return fdsrc;

  SceOff size = sceIoLseek(fdsrc, 0, SCE_SEEK_END);
  sceIoLseek(fdsrc, 0, SCE_SEEK_SET);

  // Send info
  ShareInfo info;
  info.magic = SHARE_MAGIC;
  info.type = SHARE_TYPE_FILE;
  info.path_len = strlen(src_path);
  info.file_size = size;
  int ret = adhocSend(client_socket, &info, sizeof(ShareInfo));
  if (ret < 0) {
    sceIoDclose(fdsrc);
    return ret;
  }
  
  // Send path
  ret = adhocSend(client_socket, src_path, info.path_len);
  if (ret < 0) {
    sceIoDclose(fdsrc);
    return ret;
  }
  
  void *buf = memalign(4096, SHARE_SIZE);
  
  while (1) {
    int read = sceIoRead(fdsrc, buf, SHARE_SIZE);

    if (read < 0) {
      free(buf);
      sceIoClose(fdsrc);
      return read;
    }

    if (read == 0)
      break;

    int sended = adhocSend(client_socket, buf, read);
    if (sended < 0) {
      free(buf);
      sceIoClose(fdsrc);
      return sended;
    }

    if (param) {
      if (param->value)
        (*param->value) += read;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        free(buf);
        sceIoClose(fdsrc);
        return 0;
      }
    }
  }

  free(buf);
  sceIoClose(fdsrc);

  return 1;
}

int sendPath(const char *src_path, FileProcessParam *param) {  
  SceUID dfd = sceIoDopen(src_path);
  if (dfd >= 0) {
    // Send info
    ShareInfo info;
    info.magic = SHARE_MAGIC;
    info.type = SHARE_TYPE_FOLDER;
    info.path_len = strlen(src_path);
    info.file_size = 0;
    int ret = adhocSend(client_socket, &info, sizeof(ShareInfo));
    if (ret < 0) {
      sceIoDclose(dfd);
      return ret;
    }
    
    // Send path
    ret = adhocSend(client_socket, src_path, info.path_len);
    if (ret < 0) {
      sceIoDclose(dfd);
      return ret;
    }
    
    if (param) {
      if (param->value)
        (*param->value) += DIRECTORY_SIZE;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        sceIoDclose(dfd);
        return 0;
      }
    }

    int res = 0;

    do {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dfd, &dir);
      if (res > 0) {
        char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
        snprintf(new_src_path, MAX_PATH_LENGTH - 1, "%s%s%s", src_path, hasEndSlash(src_path) ? "" : "/", dir.d_name);

        int ret = 0;

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
          ret = sendPath(new_src_path, param);
        } else {
          ret = sendFile(new_src_path, param);
        }

        free(new_src_path);

        if (ret <= 0) {
          sceIoDclose(dfd);
          return ret;
        }
      }
    } while (res > 0);

    sceIoDclose(dfd);
  } else {
    return sendFile(src_path, param);
  }

  return 1;
}

int send_thread(SceSize args_size, SendArguments *args) {
  int res;
  SceUID thid = -1;

  // Lock power timers
  // powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  FileListEntry *file_entry = fileListGetNthEntry(args->file_list, args->index);

  int count = 0;
  FileListEntry *head = NULL;
  FileListEntry *mark_entry_one = NULL;

  if (fileListFindEntry(args->mark_list, file_entry->name)) { // On marked entry
    count = args->mark_list->length;
    head = args->mark_list->head;
  } else {
    count = 1;
    mark_entry_one = fileListCopyEntry(file_entry);
    head = mark_entry_one;
  }

  char path[MAX_PATH_LENGTH];
  FileListEntry *mark_entry = NULL;

  // Get paths info
  uint64_t size = 0;
  uint32_t folders = 0, files = 0;

  mark_entry = head;

  int i;
  for (i = 0; i < count; i++) {
    snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", args->file_list->path, mark_entry->name);
    getPathInfo(path, &size, &folders, &files, NULL);
    mark_entry = mark_entry->next;
  }

  // TEST
  adhocSend(client_socket, &size, 1024);
  
  // Send total size
  uint64_t total_size = size + folders * DIRECTORY_SIZE;
  res = adhocSend(client_socket, &total_size, sizeof(uint64_t));
  if (res < 0) {
    closeWaitDialog();
    setDialogStep(DIALOG_STEP_CANCELLED);
    errorDialog(res);
    goto EXIT;
  }

  // Update thread
  thid = createStartUpdateThread(total_size, 1);

  // Send process
  uint64_t value = 0;

  mark_entry = head;

  for (i = 0; i < count; i++) {
    snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", args->file_list->path, mark_entry->name);

    FileProcessParam param;
    param.value = &value;
    param.max = folders + files;
    param.SetProgress = SetProgress;
    param.cancelHandler = cancelHandler;
    int res = sendPath(path, &param);
    if (res <= 0) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELLED);
      errorDialog(res);
      goto EXIT;
    }

    mark_entry = mark_entry->next;
  }

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);

  // Close
  sceMsgDialogClose();

  setDialogStep(DIALOG_STEP_ADHOC_SENDED);

EXIT:
  if (mark_entry_one)
    free(mark_entry_one);

  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);

  // Unlock power timers
  // powerUnlock();

  // Close sockets and disconnect
  adhocCloseSockets();
  sceNetCtlAdhocDisconnect();

  return sceKernelExitDeleteThread(0);
}

int receive_thread(SceSize args_size, ReceiveArguments *args) {
  SceUID thid = -1;
  uint64_t timeout;

  // Lock power timers
  // powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  // Test
  while (1) {
    // Cancel
    if (cancelHandler()) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELLED);
      goto EXIT;
    }
    
    int len = 1024;
    char buf[1024];
    int res = adhocRecv(server_socket, buf, &len);
    if (res < 0 && res != SCE_ERROR_NET_ADHOC_WOULD_BLOCK) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELLED);
      errorDialog(res);
      goto EXIT;
    }
    
    if (res == 0) {
      break;
    }

    sceKernelDelayThread(RECEIVE_WAIT);
  }
  
  // Receive total size
  uint64_t size = 0;
  while (1) {
    // Cancel
    if (cancelHandler()) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELLED);
      goto EXIT;
    }
    
    int len = sizeof(uint64_t);
    int res = adhocRecv(server_socket, &size, &len);
    if (res < 0 && res != SCE_ERROR_NET_ADHOC_WOULD_BLOCK) {
      closeWaitDialog();
      setDialogStep(DIALOG_STEP_CANCELLED);
      errorDialog(res);
      goto EXIT;
    }
    
    if (res == 0) {
      break;
    }

    sceKernelDelayThread(RECEIVE_WAIT);
  }

  // Update thread
  thid = createStartUpdateThread(size, 1);

  // Receive process
  uint64_t value = 0;

  while (value < size) {
    ShareInfo info;
    char path[MAX_PATH_LENGTH];

    // Receive share info
    while (1) {
      // Cancel
      if (cancelHandler()) {
        closeWaitDialog();
        setDialogStep(DIALOG_STEP_CANCELLED);
        goto EXIT;
      }
      
      int len = sizeof(ShareInfo);
      int res = adhocRecv(server_socket, &info, &len);
      if (res < 0 && res != SCE_ERROR_NET_ADHOC_WOULD_BLOCK) {
        closeWaitDialog();
        setDialogStep(DIALOG_STEP_CANCELLED);
        errorDialog(res);
        goto EXIT;
      }
      
      if (res == 0) {
        // Wrong magic
        if (info.magic != SHARE_MAGIC || info.path_len >= MAX_PATH_LENGTH) {
          closeWaitDialog();
          setDialogStep(DIALOG_STEP_CANCELLED);
          errorDialog(-1);
          goto EXIT;
        }
        
        break;
      }

      sceKernelDelayThread(RECEIVE_WAIT);
    }

    // Receive path
    memset(path, 0, sizeof(path));
    
    while (1) {
      // Cancel
      if (cancelHandler()) {
        closeWaitDialog();
        setDialogStep(DIALOG_STEP_CANCELLED);
        goto EXIT;
      }
      
      int res = adhocRecv(server_socket, path, (int *)&info.path_len);
      if (res < 0 && res != SCE_ERROR_NET_ADHOC_WOULD_BLOCK) {
        closeWaitDialog();
        setDialogStep(DIALOG_STEP_CANCELLED);
        errorDialog(res);
        goto EXIT;
      }
      
      if (res == 0) {
        break;
      }

      sceKernelDelayThread(RECEIVE_WAIT);
    }
    
    // Folder
    if (info.type == SHARE_TYPE_FOLDER) {
      value += DIRECTORY_SIZE;
      SetProgress(value, size);
      continue;
    }
    
    // Receive file
    if (info.type == SHARE_TYPE_FILE) {      
      void *buf = memalign(4096, SHARE_SIZE);

      uint64_t recv_offset = 0;
      while (recv_offset < info.file_size) {
        // Cancel
        if (cancelHandler()) {
          closeWaitDialog();
          setDialogStep(DIALOG_STEP_CANCELLED);
          free(buf);
          goto EXIT;
        }
        
        int len = SHARE_SIZE;
        int res = adhocRecv(server_socket, buf, &len);
        if (res < 0 && res != SCE_ERROR_NET_ADHOC_WOULD_BLOCK) {
          closeWaitDialog();
          setDialogStep(DIALOG_STEP_CANCELLED);
          errorDialog(res);
          free(buf);
          goto EXIT;
        }
        
        if (res == 0) {
          recv_offset += len;
          value += len;
          SetProgress(value, size);
        }
        
        sceKernelDelayThread(RECEIVE_WAIT);
      }
      
      free(buf);
    }
  }

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);

  // Close
  sceMsgDialogClose();

  setDialogStep(DIALOG_STEP_ADHOC_RECEIVED);

EXIT:
  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);

  // Unlock power timers
  // powerUnlock();

  // Close sockets and disconnect
  adhocCloseSockets();
  sceNetCtlAdhocDisconnect();

  return sceKernelExitDeleteThread(0);
}

int client_waiting_thread(SceSize args, void *argp) {
  // Create PTP socket and wait for connection
  sceNetAdhocctlGetEtherAddr(&client_addr);
  client_socket = sceNetAdhocPtpListen(&client_addr, SERVER_PORT, SHARE_SIZE, RECEIVE_WAIT, 300, 1, 0);
  if (client_socket < 0) {
    server_request_result = client_socket;
    goto EXIT;
  }

  // Establish PTP connection
  SceUShort16 server_port;
  server_socket = sceNetAdhocPtpAccept(client_socket, &server_addr, &server_port, 0, 0);
  if (server_socket < 0) {
    server_request_result = server_socket;
    goto EXIT;
  }
  
  // Check if we have successfully received a request from a server
  int res = sceNetAdhocctlGetPeerInfo(&server_addr, sizeof(SceNetAdhocctlPeerInfo), &server_peer_info);
  if (res < 0) {
    server_request_result = res;
    goto EXIT;
  }
  
  server_request_result = 1;

EXIT:
  return sceKernelExitDeleteThread(0);
}

void adhocCloseSockets() {
  if (client_socket >= 0)
    sceNetAdhocSetSocketAlert(client_socket, SCE_NET_ADHOC_F_ALERTALL);

  if (server_socket >= 0)
    sceNetAdhocSetSocketAlert(server_socket, SCE_NET_ADHOC_F_ALERTALL);

  if (client_waiting_thid >= 0) {
    sceKernelWaitThreadEnd(client_waiting_thid, NULL, NULL);
    client_waiting_thid = -1;
  }

  if (client_socket >= 0) {
    sceNetAdhocPtpClose(client_socket, 0);
    client_socket = -1;
  }

  if (server_socket >= 0) {
    sceNetAdhocPtpClose(server_socket, 0);
    server_socket = -1;
  }  
}

int adhocUpdatePeerList() {
  int buflen = 0;
  sceNetAdhocctlGetPeerList(&buflen, NULL);

  memset(peer_list, 0, PEERLIST_MAX * sizeof(SceNetAdhocctlPeerInfo));
  peer_count = 0;
  if (buflen > 0) {
    if (buflen > PEERLIST_MAX * sizeof(SceNetAdhocctlPeerInfo)) {
      buflen = PEERLIST_MAX * sizeof(SceNetAdhocctlPeerInfo);
    }
    
    peer_count = buflen / sizeof(SceNetAdhocctlPeerInfo);
    sceNetAdhocctlGetPeerList(&buflen, peer_list);
  }

  return peer_count;
}

int adhocSendServerResponse(char *response) {
  return adhocSend(server_socket, response, 4);  
}

char *adhocReceiveClientReponse() {
  int len = 4;
  adhocRecv(client_socket, client_response, &len);
  return client_response;
}

int adhocReceiveServerRequest() {
  return server_request_result;
}

char *adhocGetServerNickname() {
  return (char *)server_peer_info.nickname.data;
}

void adhocWaitingForServerRequest() {
  server_request_result = 0;
  
  client_waiting_thid = sceKernelCreateThread("client_waiting_thread", client_waiting_thread, 0x10000100, 0x4000, 0, 0, NULL);
  if (client_waiting_thid >= 0)
    sceKernelStartThread(client_waiting_thid, 0, NULL);
}

int getAdhocDialogStatus() {
  return adhoc_dialog.status;
}

int initAdhocDialog() {
  memset(&adhoc_dialog, 0, sizeof(AdhocDialog));
  
  // Opening status
  adhoc_dialog.result = ADHOC_DIALOG_RESULT_NONE;
  adhoc_dialog.status = ADHOC_DIALOG_OPENING;

  // Selector
  adhoc_dialog.sel = 0;

  // Width and height
  adhoc_dialog.width = 340.0f;
  adhoc_dialog.height = FONT_Y_SPACE * (PEERLIST_MAX + 1);

  // For buttons
  adhoc_dialog.height += 2.0f * FONT_Y_SPACE;

  // Margin
  adhoc_dialog.width += 2.0f * SHELL_MARGIN_X;
  adhoc_dialog.height += 2.0f * SHELL_MARGIN_Y;

  // Position
  adhoc_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, adhoc_dialog.width);
  adhoc_dialog.y = ALIGN_CENTER(SCREEN_HEIGHT, adhoc_dialog.height);

  // Align
  int y_n = (int)((float)(adhoc_dialog.y - 2.0f) / FONT_Y_SPACE);
  adhoc_dialog.y = (float)y_n * FONT_Y_SPACE + 2.0f;

  // Scale
  adhoc_dialog.scale = 0.0f;

  // Reset
  memset(peer_list, 0, PEERLIST_MAX * sizeof(SceNetAdhocctlPeerInfo));
  peer_count = 0;

  return 0;
}

void adhocDialogCtrl() {
  adhocUpdatePeerList();
  
  if (pressed_pad[PAD_CROSS]) {
    if (peer_count > 0) {
      SceNetAdhocctlPeerInfo *curr = peer_list;
      int i;
      for (i = 0; i < PEERLIST_MAX; i++) {
        if (curr) {
          if (adhoc_dialog.sel == i) {
            memcpy(&client_addr, &curr->macAddr, sizeof(SceNetEtherAddr));
            adhoc_dialog.result = ADHOC_DIALOG_RESULT_WAITING_FOR_RESPONSE;
            adhoc_dialog.status = ADHOC_DIALOG_CLOSING;
            break;
          }
          
          curr = curr->next;
        }
      }
    }
  }
  
  if (pressed_pad[PAD_CANCEL]) {
    sceNetCtlAdhocDisconnect();
    adhoc_dialog.status = ADHOC_DIALOG_CLOSING;
  }
  
  if (hold_pad[PAD_UP] || hold2_pad[PAD_LEFT_ANALOG_UP]) {
    if (adhoc_dialog.sel > 0)
      adhoc_dialog.sel--;
  } else if (hold_pad[PAD_DOWN] || hold2_pad[PAD_LEFT_ANALOG_DOWN]) {
    if (adhoc_dialog.sel < peer_count - 1)
      adhoc_dialog.sel++;
  }
  
  // Easing out
  if (adhoc_dialog.status == ADHOC_DIALOG_CLOSING) {
    if (adhoc_dialog.scale > 0.0f) {
      adhoc_dialog.scale -= easeOut(0.0f, adhoc_dialog.scale, 0.25f, 0.01f);
    } else {
      adhoc_dialog.status = ADHOC_DIALOG_CLOSED;
    }
  }

  if (adhoc_dialog.status == ADHOC_DIALOG_OPENING) {
    if (adhoc_dialog.scale < 1.0f) {
      adhoc_dialog.scale += easeOut(adhoc_dialog.scale, 1.0f, 0.25f, 0.01f);
    } else {
      adhoc_dialog.status = ADHOC_DIALOG_OPENED;
    }
  }

  // Waiting state
  if (adhoc_dialog.status == ADHOC_DIALOG_CLOSED) {
    if (adhoc_dialog.result == ADHOC_DIALOG_RESULT_WAITING_FOR_RESPONSE) {      
      // Create PTP socket and start connection
      sceNetAdhocctlGetEtherAddr(&server_addr);
      client_socket = sceNetAdhocPtpOpen(&server_addr, 0, &client_addr, SERVER_PORT, SHARE_SIZE, RECEIVE_WAIT, 300, 0);
      if (client_socket < 0) {
        sceNetCtlAdhocDisconnect();
        errorDialog(client_socket);
        return;
      }
      
      // Establish PTP connection
      // sceNetAdhocPtpConnect(client_socket, 0, 0);
      /*int res = sceNetAdhocPtpConnect(client_socket, 0, SCE_NET_ADHOC_F_NONBLOCK);
      if (res < 0) {
        sceNetAdhocPtpClose(client_socket, 0);
        sceNetCtlAdhocDisconnect();
        errorDialog(res);
        return;
      }*/
      
      memset(client_response, 0, sizeof(client_response));
      
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[PLEASE_WAIT]);
      setDialogStep(DIALOG_STEP_ADHOC_SEND_WAITING);
    }
  }
}

void drawAdhocDialog() {
  if (adhoc_dialog.status == ADHOC_DIALOG_CLOSED)
    return;

  // Dialog background
  float dialog_width = vita2d_texture_get_width(dialog_image);
  float dialog_height = vita2d_texture_get_height(dialog_image);
  vita2d_draw_texture_scale_rotate_hotspot(dialog_image, adhoc_dialog.x + adhoc_dialog.width / 2.0f,
                                           adhoc_dialog.y + adhoc_dialog.height / 2.0f,
                                           adhoc_dialog.scale * (adhoc_dialog.width/dialog_width),
                                           adhoc_dialog.scale * (adhoc_dialog.height/dialog_height),
                                           0.0f, dialog_width / 2.0f, dialog_height / 2.0f);

  if (adhoc_dialog.status == ADHOC_DIALOG_OPENED) {
    float string_y = adhoc_dialog.y + SHELL_MARGIN_Y - 2.0f;

    // Select PS Vita
    pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(language_container[ADHOC_SELECT_PSVITA])), string_y, DIALOG_COLOR, language_container[ADHOC_SELECT_PSVITA]);
    string_y += FONT_Y_SPACE;

    // Peer list
    SceNetAdhocctlPeerInfo *curr = peer_list;
    int i;
    for (i = 0; i < PEERLIST_MAX; i++) {
      if (curr) {
        char *user = (char *)curr->nickname.data;
        pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(user)), string_y, (i == adhoc_dialog.sel) ? FOCUS_COLOR : DIALOG_COLOR, user);
        curr = curr->next;
      }
      
      string_y += FONT_Y_SPACE;
    }

    // Buttons
    char button_string[128];
    sprintf(button_string, "%s %s    %s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK],
                                             enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CROSS : CIRCLE, language_container[CANCEL]);
    pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(button_string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, button_string);
  }
}