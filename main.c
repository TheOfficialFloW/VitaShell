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
#include "main_context.h"
#include "init.h"
#include "io_process.h"
#include "refresh.h"
#include "makezip.h"
#include "package_installer.h"
#include "network_update.h"
#include "network_download.h"
#include "context_menu.h"
#include "archive.h"
#include "photo.h"
#include "audioplayer.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "settings.h"
#include "adhoc_dialog.h"
#include "property_dialog.h"
#include "message_dialog.h"
#include "netcheck_dialog.h"
#include "ime_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"
#include "coredump.h"
#include "usb.h"
#include "qr.h"

int _newlib_heap_size_user = 128 * 1024 * 1024;

// Dialog step
static volatile int dialog_step = DIALOG_STEP_NONE;

// File lists
FileList file_list, mark_list, copy_list, install_list;

// Paths
char cur_file[MAX_PATH_LENGTH];
char archive_copy_path[MAX_PATH_LENGTH];
char archive_path[MAX_PATH_LENGTH];

static char install_path[MAX_PATH_LENGTH];
static char focus_name[MAX_NAME_LENGTH], compress_name[MAX_NAME_LENGTH];

// Position
int base_pos = 0, rel_pos = 0;
static int base_pos_list[MAX_DIR_LEVELS];
static int rel_pos_list[MAX_DIR_LEVELS];
static int dir_level = 0;

// Modes
int sort_mode = SORT_BY_NAME;
int copy_mode = COPY_MODE_NORMAL;
int file_type = FILE_TYPE_UNKNOWN;

// Archive
static int is_in_archive = 0;
static char dir_level_archive = -1;

// FTP
static char vita_ip[16];
static unsigned short int vita_port;

static SceUID usbdevice_modid = -1;

static SceKernelLwMutexWork dialog_mutex;

// Scrolling filename
static int scroll_count = 0;
static float scroll_x = FILE_X;

VitaShellConfig vitashell_config;

// Enter and cancel buttons
int SCE_CTRL_ENTER = SCE_CTRL_CROSS, SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;

// Use custom config
int use_custom_config = 1;

int getDialogStep() {
  sceKernelLockLwMutex(&dialog_mutex, 1, NULL);
  volatile int step = dialog_step;
  sceKernelUnlockLwMutex(&dialog_mutex, 1);
  return step;
}

void setDialogStep(int step) {
  sceKernelLockLwMutex(&dialog_mutex, 1, NULL);
  dialog_step = step;
  sceKernelUnlockLwMutex(&dialog_mutex, 1);
}

void dirLevelUp() {
  base_pos_list[dir_level] = base_pos;
  rel_pos_list[dir_level] = rel_pos;
  dir_level++;
  base_pos_list[dir_level] = 0;
  rel_pos_list[dir_level] = 0;
  base_pos = 0;
  rel_pos = 0;
}

int isInArchive() {
  return is_in_archive;
}

void dirUpCloseArchive() {
  if (isInArchive() && dir_level_archive >= dir_level) {
    is_in_archive = 0;
    archiveClose();
    dir_level_archive = -1;
  }
}

static void dirUp() {
  if (pfs_mounted_path[0] &&
      strcmp(file_list.path, pfs_mounted_path) == 0 && // we're about to leave the pfs path
      !strstr(copy_list.path, pfs_mounted_path)) { // nothing has been copied from pfs path
    // Then umount
    gameDataUmount();
  }

  removeEndSlash(file_list.path);

  char *p;

  p = strrchr(file_list.path, '/');
  if (p) {
    p[1] = '\0';
    dir_level--;
    goto DIR_UP_RETURN;
  }

  p = strrchr(file_list.path, ':');
  if (p) {
    if (strlen(file_list.path) - ((p + 1) - file_list.path) > 0) {
      p[1] = '\0';
      dir_level--;
      goto DIR_UP_RETURN;
    }
  }

  strcpy(file_list.path, HOME_PATH);
  dir_level = 0;

DIR_UP_RETURN:
  base_pos = (int)base_pos_list[dir_level];
  rel_pos = (int)rel_pos_list[dir_level];
  dirUpCloseArchive();
}

static void setFocusOnFilename(const char *name) {
  int name_pos = fileListGetNumberByName(&file_list, name);
  if (name_pos < 0 || name_pos >= file_list.length)
    return;
  
  if (name_pos >= base_pos && name_pos < (base_pos + MAX_POSITION)) {
    rel_pos = name_pos - base_pos;
  } else if (name_pos < base_pos) {
    base_pos = name_pos;
    rel_pos = 0;
  } else if (name_pos >= (base_pos + MAX_POSITION)) {
    rel_pos = MAX_POSITION - 1;
    base_pos = name_pos - rel_pos;
  }
}

int refreshFileList() {
  int ret = 0, res = 0;

  do {
    fileListEmpty(&file_list);

    res = fileListGetEntries(&file_list, file_list.path, sort_mode);

    if (res < 0) {
      ret = res;
      dirUp();
    }
  } while (res < 0);
    
  // Position correction
  if (file_list.length >= MAX_POSITION) {
    if ((base_pos + rel_pos) >= file_list.length) {
      rel_pos = MAX_POSITION - 1;
    }
    
    if ((base_pos + MAX_POSITION - 1) >= file_list.length) {
      base_pos = file_list.length - MAX_POSITION;
    }
  } else {
    if ((base_pos + rel_pos) >= file_list.length) {
      rel_pos = file_list.length - 1;
    }
    
    base_pos = 0;
  }
  
  return ret;
}

static void refreshMarkList() {
  if (isInArchive())
    return;
  
  FileListEntry *entry = mark_list.head;

  int length = mark_list.length;

  int i;
  for (i = 0; i < length; i++) {
    // Get next entry already now to prevent crash after entry is removed
    FileListEntry *next = entry->next;

    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, entry->name);

    // Check if the entry still exits. If not, remove it from list
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    if (sceIoGetstat(path, &stat) < 0)
      fileListRemoveEntry(&mark_list, entry);

    // Next
    entry = next;
  }
}

static void refreshCopyList() {
  if (copy_list.is_in_archive)
    return;
  
  FileListEntry *entry = copy_list.head;

  int length = copy_list.length;

  int i;
  for (i = 0; i < length; i++) {
    // Get next entry already now to prevent crash after entry is removed
    FileListEntry *next = entry->next;

    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", copy_list.path, entry->name);

    // Check if the entry still exits. If not, remove it from list
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    if (sceIoGetstat(path, &stat) < 0)
      fileListRemoveEntry(&copy_list, entry);

    // Next
    entry = next;
  }
}

static int handleFile(const char *file, FileListEntry *entry) {
  int res = 0;

  int type = getFileType(file);

  switch (type) {
    case FILE_TYPE_PSP2DMP:
    case FILE_TYPE_MP3:
    case FILE_TYPE_OGG:
    case FILE_TYPE_VPK:
    case FILE_TYPE_ARCHIVE:
      if (isInArchive())
        type = FILE_TYPE_UNKNOWN;

      break;
  }

  switch (type) {
    case FILE_TYPE_PSP2DMP:
      res = coredumpViewer(file);
      break;
      
    case FILE_TYPE_INI:
    case FILE_TYPE_TXT:
    case FILE_TYPE_XML:
    case FILE_TYPE_UNKNOWN:
      res = textViewer(file);
      break;
      
    case FILE_TYPE_BMP:
    case FILE_TYPE_PNG:
    case FILE_TYPE_JPEG:
      res = photoViewer(file, type, &file_list, entry, &base_pos, &rel_pos);
      break;

    case FILE_TYPE_MP3:
    case FILE_TYPE_OGG:
      res = audioPlayer(file, type, &file_list, entry, &base_pos, &rel_pos);
      break;
      
    case FILE_TYPE_SFO:
      res = SFOReader(file);
      break;
      
    case FILE_TYPE_VPK:
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_QUESTION]);
      setDialogStep(DIALOG_STEP_INSTALL_QUESTION);
      break;
      
    case FILE_TYPE_ARCHIVE:
      archiveClearPassword();
      res = archiveOpen(file);
      if (res >= 0 && archiveNeedPassword()) {
        initImeDialog(language_container[ENTER_PASSWORD], "", 128, SCE_IME_TYPE_BASIC_LATIN, 0, 1);
        setDialogStep(DIALOG_STEP_ENTER_PASSWORD);
      }
      break;
      
    default:
      res = textViewer(file);
      break;
  }

  if (res < 0) {
    errorDialog(res);
    return res;
  }

  return type;
}

void drawScrollBar(int pos, int n) {
  if (n > MAX_POSITION) {
    vita2d_draw_rectangle(SCROLL_BAR_X, START_Y, SCROLL_BAR_WIDTH, MAX_ENTRIES * FONT_Y_SPACE, SCROLL_BAR_BG_COLOR);

    float y = START_Y + ((pos * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);
    float height = ((MAX_POSITION * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);

    float scroll_bar_y = MIN(y, (START_Y + MAX_ENTRIES * FONT_Y_SPACE - height));
    vita2d_draw_rectangle(SCROLL_BAR_X, scroll_bar_y, SCROLL_BAR_WIDTH, MAX(height, SCROLL_BAR_MIN_HEIGHT), SCROLL_BAR_COLOR);
  }
}

void drawShellInfo(const char *path) {
  // Title
  char version[8];
  sprintf(version, "%X.%02X", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR);
  if (version[3] == '0')
    version[3] = '\0';

  pgf_draw_textf(SHELL_MARGIN_X, SHELL_MARGIN_Y, TITLE_COLOR, "VitaShell %s", version);

  // Status bar
  float x = SCREEN_WIDTH - SHELL_MARGIN_X;

  // Battery
  if (sceKernelGetModel() == SCE_KERNEL_MODEL_VITA) {
    float battery_x = ALIGN_RIGHT(x, vita2d_texture_get_width(battery_image));
    vita2d_draw_texture(battery_image, battery_x, SHELL_MARGIN_Y + 3.0f);

    vita2d_texture *battery_bar_image = battery_bar_green_image;

    if (scePowerIsLowBattery() && !scePowerIsBatteryCharging()) {
      battery_bar_image = battery_bar_red_image;
    } 

    float percent = scePowerGetBatteryLifePercent() / 100.0f;

    float width = vita2d_texture_get_width(battery_bar_image);
    vita2d_draw_texture_part(battery_bar_image, battery_x + 3.0f + (1.0f - percent) * width,
                             SHELL_MARGIN_Y + 5.0f, (1.0f - percent) * width, 0.0f, percent * width,
                             vita2d_texture_get_height(battery_bar_image));

    if (scePowerIsBatteryCharging()) {
      vita2d_draw_texture(battery_bar_charge_image, battery_x + 3.0f, SHELL_MARGIN_Y + 5.0f);
    }

    x = battery_x - STATUS_BAR_SPACE_X;
  }

  // Date & time
  SceDateTime time;
  sceRtcGetCurrentClock(&time, 0);

  char date_string[16];
  getDateString(date_string, date_format, &time);

  char time_string[24];
  getTimeString(time_string, time_format, &time);

  char string[64];
  snprintf(string, sizeof(string), "%s  %s", date_string, time_string);
  float date_time_x = ALIGN_RIGHT(x, pgf_text_width(string));
  pgf_draw_text(date_time_x, SHELL_MARGIN_Y, DATE_TIME_COLOR, string);

  x = date_time_x - STATUS_BAR_SPACE_X;

  // FTP
  if (ftpvita_is_initialized()) {
    float ftp_x = ALIGN_RIGHT(x, vita2d_texture_get_width(ftp_image));
    vita2d_draw_texture(ftp_image, ftp_x, SHELL_MARGIN_Y + 3.0f);
    
    x = ftp_x - STATUS_BAR_SPACE_X;
  }

  // TODO: make this more elegant
  // Path
  int line_width = 0;

  int i;
  for (i = 0; i < strlen(path); i++) {
    char ch_width = font_size_cache[(int)path[i]];

    // Too long
    if ((line_width + ch_width) >= MAX_WIDTH)
      break;

    // Increase line width
    line_width += ch_width;
  }

  char path_first_line[256], path_second_line[256];

  strncpy(path_first_line, path, i);
  path_first_line[i] = '\0';

  strcpy(path_second_line, path+i);

  // home (SAFE/UNSAFE MODE)
  if (strcmp(path_first_line, HOME_PATH) == 0) {    
    pgf_draw_textf(SHELL_MARGIN_X, PATH_Y, PATH_COLOR, "%s (%s)", HOME_PATH,
                   is_safe_mode ? language_container[SAFE_MODE] : language_container[UNSAFE_MODE]);
  } else {
    // Path
    pgf_draw_text(SHELL_MARGIN_X, PATH_Y, PATH_COLOR, path_first_line);
    pgf_draw_text(SHELL_MARGIN_X, PATH_Y + FONT_Y_SPACE, PATH_COLOR, path_second_line);
  }
}

static void initFtp() {
  // Add all the current mountpoints to ftpvita
  int i;
  for (i = 0; i < getNumberOfDevices(); i++) {
    char **devices = getDevices();
    if (devices[i]) {
      if (is_safe_mode && strcmp(devices[i], "ux0:") != 0)
        continue;

      ftpvita_add_device(devices[i]);
    }
  }

  ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);
}

static void initUsb() {
  char *path = NULL;

  if (vitashell_config.usbdevice == USBDEVICE_MODE_MEMORY_CARD) {
    if (checkFileExist("sdstor0:xmc-lp-ign-userext"))
      path = "sdstor0:xmc-lp-ign-userext";
    else if (checkFileExist("sdstor0:int-lp-ign-userext"))
      path = "sdstor0:int-lp-ign-userext";
    else
      infoDialog(language_container[MEMORY_CARD_NOT_FOUND]);
  } else if (vitashell_config.usbdevice == USBDEVICE_MODE_GAME_CARD) {
    if (checkFileExist("sdstor0:gcd-lp-ign-gamero"))
      path = "sdstor0:gcd-lp-ign-gamero";
    else
      infoDialog(language_container[GAME_CARD_NOT_FOUND]);
  } else if (vitashell_config.usbdevice == USBDEVICE_MODE_SD2VITA) {
    if (checkFileExist("sdstor0:gcd-lp-ign-entire"))
      path = "sdstor0:gcd-lp-ign-entire";
    else
      infoDialog(language_container[MICROSD_NOT_FOUND]);
  } else if (vitashell_config.usbdevice == USBDEVICE_MODE_PSVSD) {
    if (checkFileExist("sdstor0:uma-pp-act-a"))
      path = "sdstor0:uma-pp-act-a";
    else if (checkFileExist("sdstor0:uma-lp-act-entire"))
      path = "sdstor0:uma-lp-act-entire";
    else
      infoDialog(language_container[MICROSD_NOT_FOUND]);
  }

  if (!path)
    return;

  usbdevice_modid = startUsb("ux0:VitaShell/module/usbdevice.skprx", path, SCE_USBSTOR_VSTOR_TYPE_FAT);
  if (usbdevice_modid >= 0) {
    // Lock power timers
    powerLock();
    
    initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[USB_CONNECTED]);
    setDialogStep(DIALOG_STEP_USB);
  } else {
    errorDialog(usbdevice_modid);
  }
}

static int dialogSteps() {
  int refresh = REFRESH_MODE_NONE;

  int msg_result = updateMessageDialog();
  int netcheck_result = updateNetCheckDialog();
  int ime_result = updateImeDialog();

  switch (getDialogStep()) {
    case DIALOG_STEP_ERROR:
    case DIALOG_STEP_INFO:
    case DIALOG_STEP_SYSTEM:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_CANCELED:
      refresh = REFRESH_MODE_NORMAL;
      setDialogStep(DIALOG_STEP_NONE);
      break;
      
    case DIALOG_STEP_DELETED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (file_entry) {
          // Empty mark list if on marked entry
          if (fileListFindEntry(&mark_list, file_entry->name)) {
            fileListEmpty(&mark_list);
          }

          refresh = REFRESH_MODE_NORMAL;
        }
        
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_COMPRESSED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (file_entry) {
          // Empty mark list if on marked entry
          if (fileListFindEntry(&mark_list, file_entry->name)) {
            fileListEmpty(&mark_list);
          }
          
          // Focus
          strcpy(focus_name, compress_name);
          
          refresh = REFRESH_MODE_SETFOCUS;
        }
        
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_COPIED:
    case DIALOG_STEP_MOVED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        // Empty mark list
        fileListEmpty(&mark_list);
        
        // Copy copy list to mark list
        FileListEntry *copy_entry = copy_list.head;
        
        int i;
        for (i = 0; i < copy_list.length; i++) {
          fileListAddEntry(&mark_list, fileListCopyEntry(copy_entry), SORT_NONE);

          // Next
          copy_entry = copy_entry->next;
        }
        
        // Focus
        strcpy(focus_name, copy_list.head->name);

        // Empty copy list when moved
        if (getDialogStep() == DIALOG_STEP_MOVED)
          fileListEmpty(&copy_list);
        
        // Umount and remove from clipboard after pasting
        if (pfs_mounted_path[0] && strstr(copy_list.path, pfs_mounted_path)) {
          gameDataUmount();
          fileListEmpty(&copy_list);
        }

        refresh = REFRESH_MODE_SETFOCUS;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_REFRESH_LIVEAREA_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[REFRESHING]);
        setDialogStep(DIALOG_STEP_REFRESHING);

        SceUID thid = sceKernelCreateThread("refresh_thread", (SceKernelThreadEntry)refresh_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, 0, NULL);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_REFRESH_LICENSE_DB_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[REFRESHING]);
        setDialogStep(DIALOG_STEP_REFRESHING);

        SceUID thid = sceKernelCreateThread("license_thread", (SceKernelThreadEntry)license_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, 0, NULL);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }

    case DIALOG_STEP_USB_ATTACH_WAIT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        if (checkFileExist("sdstor0:uma-lp-act-entire")) {
          sceMsgDialogClose();
        }
      } else {
        if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
            msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
          setDialogStep(DIALOG_STEP_NONE);
          
          if (checkFileExist("sdstor0:uma-lp-act-entire")) {
            int res = vshIoMount(0xF00, NULL, 0, 0, 0, 0);
            if (res < 0)
              errorDialog(res);
            else
              infoDialog(language_container[USB_UMA0_MOUNTED]);
            refresh = REFRESH_MODE_NORMAL;
          }
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_FTP_WAIT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        int state = 0;
        sceNetCtlInetGetState(&state);
        if (state == 3) {
          int res = ftpvita_init(vita_ip, &vita_port);
          if (res >= 0) {
            initFtp();
            sceMsgDialogClose();
          }
        }
      } else {
        if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
            msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
          setDialogStep(DIALOG_STEP_NONE);

          // Dialog
          if (ftpvita_is_initialized()) {
            initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
            setDialogStep(DIALOG_STEP_FTP);
          } else {
            powerUnlock();
          }
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_FTP:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        powerUnlock();
        ftpvita_fini();
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_USB_WAIT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        SceUdcdDeviceState state;
        sceUdcdGetDeviceState(&state);
        
        if (state.cable & SCE_UDCD_STATUS_CABLE_CONNECTED) {
          sceMsgDialogClose();
        }
      } else {
        if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
            msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
          setDialogStep(DIALOG_STEP_NONE);

          SceUdcdDeviceState state;
          sceUdcdGetDeviceState(&state);
          
          if (state.cable & SCE_UDCD_STATUS_CABLE_CONNECTED) {
            initUsb();
          }
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_USB:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        SceUdcdDeviceState state;
        sceUdcdGetDeviceState(&state);
        
        if (state.cable & SCE_UDCD_STATUS_CABLE_DISCONNECTED) {
          sceMsgDialogClose();
        }
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        powerUnlock();
        stopUsb(usbdevice_modid);
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_PASTE:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        CopyArguments args;
        args.file_list = &file_list;
        args.copy_list = &copy_list;
        args.archive_path = archive_copy_path;
        args.copy_mode = copy_mode;
        args.file_type = file_type;

        setDialogStep(DIALOG_STEP_COPYING);

        SceUID thid = sceKernelCreateThread("copy_thread", (SceKernelThreadEntry)copy_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(CopyArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_DELETE_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DELETING]);
        setDialogStep(DIALOG_STEP_DELETE_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_DELETE_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        DeleteArguments args;
        args.file_list = &file_list;
        args.mark_list = &mark_list;
        args.index = base_pos + rel_pos;

        setDialogStep(DIALOG_STEP_DELETING);

        SceUID thid = sceKernelCreateThread("delete_thread", (SceKernelThreadEntry)delete_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(DeleteArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_EXPORT_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[EXPORTING]);
        setDialogStep(DIALOG_STEP_EXPORT_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_EXPORT_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        ExportArguments args;
        args.file_list = &file_list;
        args.mark_list = &mark_list;
        args.index = base_pos + rel_pos;

        setDialogStep(DIALOG_STEP_EXPORTING);

        SceUID thid = sceKernelCreateThread("export_thread", (SceKernelThreadEntry)export_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(ExportArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_RENAME:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
          if (!file_entry) {
            setDialogStep(DIALOG_STEP_NONE);
            break;
          }
          
          char old_name[MAX_NAME_LENGTH];
          strcpy(old_name, file_entry->name);
          removeEndSlash(old_name);

          if (strcasecmp(old_name, name) == 0) { // No change
            setDialogStep(DIALOG_STEP_NONE);
          } else {
            char old_path[MAX_PATH_LENGTH];
            char new_path[MAX_PATH_LENGTH];

            snprintf(old_path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, old_name);
            snprintf(new_path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, name);

            int res = sceIoRename(old_path, new_path);
            if (res < 0) {
              errorDialog(res);
            } else {
              refresh = REFRESH_MODE_NORMAL;
              setDialogStep(DIALOG_STEP_NONE);
            }
          }
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_NEW_FOLDER:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          char path[MAX_PATH_LENGTH];
          snprintf(path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, name);

          int res = sceIoMkdir(path, 0777);
          if (res < 0) {
            errorDialog(res);
          } else {
            // Focus
            strcpy(focus_name, name);
            addEndSlash(focus_name);
            
            refresh = REFRESH_MODE_SETFOCUS;
            setDialogStep(DIALOG_STEP_NONE);
          }
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_COMPRESS_NAME:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *name = (char *)getImeDialogInputTextUTF8();
        if (name[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          strcpy(compress_name, name);

          initImeDialog(language_container[COMPRESSION_LEVEL], "6", 1, SCE_IME_TYPE_NUMBER, 0, 0);
          setDialogStep(DIALOG_STEP_COMPRESS_LEVEL);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_COMPRESS_LEVEL:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *level = (char *)getImeDialogInputTextUTF8();
        if (level[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          snprintf(cur_file, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, compress_name);

          CompressArguments args;
          args.file_list = &file_list;
          args.mark_list = &mark_list;
          args.index = base_pos + rel_pos;
          args.level = atoi(level);
          args.path = cur_file;

          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[COMPRESSING]);
          setDialogStep(DIALOG_STEP_COMPRESSING);

          SceUID thid = sceKernelCreateThread("compress_thread", (SceKernelThreadEntry)compress_thread, 0x40, 0x100000, 0, 0, NULL);
          if (thid >= 0)
            sceKernelStartThread(thid, sizeof(CompressArguments), &args);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_HASH_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        // Throw up the progress bar, enter hashing state
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[HASHING]);
        setDialogStep(DIALOG_STEP_HASH_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        // Quit
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_HASH_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        // User has confirmed desire to hash, get requested file entry
        FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
        if (!file_entry) {
          setDialogStep(DIALOG_STEP_NONE);
          break;
        }
        
        // Place the full file path in cur_file
        snprintf(cur_file, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, file_entry->name);

        HashArguments args;
        args.file_path = cur_file;

        setDialogStep(DIALOG_STEP_HASHING);

        // Create a thread to run out actual sum
        SceUID thid = sceKernelCreateThread("hash_thread", (SceKernelThreadEntry)hash_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(HashArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALL_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
        setDialogStep(DIALOG_STEP_INSTALL_CONFIRMED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALL_CONFIRMED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        InstallArguments args;

        if (install_list.length > 0) {
          FileListEntry *entry = install_list.head;
          snprintf(install_path, MAX_PATH_LENGTH - 1, "%s%s", install_list.path, entry->name);
          args.file = install_path;

          // Focus
          setFocusOnFilename(entry->name);

          // Remove entry
          fileListRemoveEntry(&install_list, entry);
        } else {
          args.file = cur_file;
        }

        setDialogStep(DIALOG_STEP_INSTALLING);

        SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(InstallArguments), &args);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALL_CONFIRMED_QR:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        InstallArguments args;
        args.file = getLastDownloadQR();

        setDialogStep(DIALOG_STEP_INSTALLING);

        SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, sizeof(InstallArguments), &args);
      }

      break;
    }
 
    case DIALOG_STEP_INSTALL_WARNING:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        setDialogStep(DIALOG_STEP_INSTALL_WARNING_AGREED);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_CANCELED);
      }

      break;
    }
    
    case DIALOG_STEP_INSTALLED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_NONE ||
          msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        if (install_list.length > 0) {
          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
          setDialogStep(DIALOG_STEP_INSTALL_CONFIRMED);
          break;
        }

        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_UPDATE_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DOWNLOADING]);
        setDialogStep(DIALOG_STEP_DOWNLOADING);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }
    }
    
    case DIALOG_STEP_DOWNLOADED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);

        setDialogStep(DIALOG_STEP_EXTRACTING);

        SceUID thid = sceKernelCreateThread("update_extract_thread", (SceKernelThreadEntry)update_extract_thread, 0x40, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, 0, NULL);
      }

      break;
    }
    
    case DIALOG_STEP_SETTINGS_AGREEMENT:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        settingsAgree();
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        settingsDisagree();
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        settingsAgree();
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_SETTINGS_STRING:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *string = (char *)getImeDialogInputTextUTF8();
        if (string[0] != '\0') {
          strcpy((char *)getImeDialogInitialText(), string);
        }

        setDialogStep(DIALOG_STEP_NONE);
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_EXTRACTED:
    {
      launchAppByUriExit("VSUPDATER");
      setDialogStep(DIALOG_STEP_NONE);
      break;
    }
      
    case DIALOG_STEP_QR:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        setDialogStep(DIALOG_STEP_NONE);
        setScannedQR(0);
      } else if (scannedQR()) {
        setDialogStep(DIALOG_STEP_QR_DONE);
        sceMsgDialogClose();
        setScannedQR(0);
        stopQR();
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_DONE:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        setDialogStep(DIALOG_STEP_QR_WAITING);
        stopQR();
        SceUID thid = sceKernelCreateThread("qr_scan_thread", (SceKernelThreadEntry)qr_scan_thread, 0x10000100, 0x100000, 0, 0, NULL);
        if (thid >= 0)
          sceKernelStartThread(thid, 0, NULL);
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_WAITING:
    {
      break;
    }
    
    case DIALOG_STEP_QR_CONFIRM:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DOWNLOADING]);
        setDialogStep(DIALOG_STEP_QR_DOWNLOADING);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_DOWNLOADED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_DOWNLOADED_VPK:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
        setDialogStep(DIALOG_STEP_INSTALL_CONFIRMED_QR);
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_OPEN_WEBSITE:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        setDialogStep(DIALOG_STEP_NONE);
        sceAppMgrLaunchAppByUri(0xFFFFF, getLastQR());
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        setDialogStep(DIALOG_STEP_NONE);
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        setDialogStep(DIALOG_STEP_NONE);
        sceAppMgrLaunchAppByUri(0xFFFFF, getLastQR());
      }
      
      break;
    }
    
    case DIALOG_STEP_QR_SHOW_CONTENTS:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ENTER_PASSWORD:
    {
      if (ime_result == IME_DIALOG_RESULT_FINISHED) {
        char *password = (char *)getImeDialogInputTextUTF8();
        if (password[0] == '\0') {
          setDialogStep(DIALOG_STEP_NONE);
        } else {
          // TODO: verify password
          archiveSetPassword(password);
          
          FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
          if (!file_entry) {
            setDialogStep(DIALOG_STEP_NONE);
            break;
          }
          
          is_in_archive = 1;
          dir_level_archive = dir_level;

          snprintf(archive_path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, file_entry->name);

          strcat(file_list.path, file_entry->name);
          addEndSlash(file_list.path);

          dirLevelUp();
          
          refresh = REFRESH_MODE_NORMAL;
          setDialogStep(DIALOG_STEP_NONE);
        }
      } else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
        setDialogStep(DIALOG_STEP_NONE);
      }

      break;
    }
    
    case DIALOG_STEP_ADHOC_SEND_NETCHECK:
    {
      if (netcheck_result == NETCHECK_DIALOG_RESULT_CONNECTED) {
        initAdhocDialog();
        setDialogStep(DIALOG_STEP_NONE);
      } else if (netcheck_result == NETCHECK_DIALOG_RESULT_NOT_CONNECTED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_SEND_WAITING:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        // Wait for a response, then close
        if (strcmp(adhocReceiveClientReponse(), "YES") == 0 ||
            strcmp(adhocReceiveClientReponse(), "NO") == 0) {
          sceMsgDialogClose();
        }
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        if (strcmp(adhocReceiveClientReponse(), "YES") == 0) {
          SendArguments args;
          args.file_list = &file_list;
          args.mark_list = &mark_list;
          args.index = base_pos + rel_pos;

          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[SENDING]);
          setDialogStep(DIALOG_STEP_ADHOC_SENDING);

          SceUID thid = sceKernelCreateThread("send_thread", (SceKernelThreadEntry)send_thread, 0x40, 0x100000, 0, 0, NULL);
          if (thid >= 0)
            sceKernelStartThread(thid, sizeof(SendArguments), &args);
        } else if (strcmp(adhocReceiveClientReponse(), "NO") == 0) {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[ADHOC_CLIENT_DECLINED]);
          setDialogStep(DIALOG_STEP_ADHOC_SEND_CLIENT_DECLINED);
        } else {
          // Return to select menu
          adhocCloseSockets();
          initAdhocDialog();
          setDialogStep(DIALOG_STEP_NONE);
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_SEND_CLIENT_DECLINED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        // Return to select menu
        adhocCloseSockets();
        initAdhocDialog();
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_RECEIVE_NETCHECK:
    {
      if (netcheck_result == NETCHECK_DIALOG_RESULT_CONNECTED) {
        adhocWaitingForServerRequest();
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[ADHOC_RECEIVE_SEARCHING_PSVITA]);
        setDialogStep(DIALOG_STEP_ADHOC_RECEIVE_SEARCHING);
      } else if (netcheck_result == NETCHECK_DIALOG_RESULT_NOT_CONNECTED) {
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_RECEIVE_SEARCHING:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
        // Wait for a request, then close
        if (adhocReceiveServerRequest() == 1) {
          sceMsgDialogClose();
        }
      } else if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        // If the dialog is closed and we got a request, go to question state, otherwise end waiting
        if (adhocReceiveServerRequest() == 1) {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[ADHOC_RECEIVE_QUESTION], adhocGetServerNickname());
          setDialogStep(DIALOG_STEP_ADHOC_RECEIVE_QUESTION);
        } else {
          adhocCloseSockets();
          sceNetCtlAdhocDisconnect();
          setDialogStep(DIALOG_STEP_NONE);
        }
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_RECEIVE_QUESTION:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
        int res = adhocSendServerResponse("YES");
        if (res < 0) {
          adhocCloseSockets();
          sceNetCtlAdhocDisconnect();
          errorDialog(res);
        } else {
          ReceiveArguments args;
          args.file_list = &file_list;
          args.mark_list = &mark_list;
          args.index = base_pos + rel_pos;

          initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[RECEIVING]);
          setDialogStep(DIALOG_STEP_ADHOC_RECEIVING);

          SceUID thid = sceKernelCreateThread("receive_thread", (SceKernelThreadEntry)receive_thread, 0x40, 0x100000, 0, 0, NULL);
          if (thid >= 0)
            sceKernelStartThread(thid, sizeof(ReceiveArguments), &args);
        }
      } else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
        adhocSendServerResponse("NO"); // Do not check result
        
        // Go back to searching
        adhocCloseSockets();
        adhocWaitingForServerRequest();
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[ADHOC_RECEIVE_SEARCHING_PSVITA]);
        setDialogStep(DIALOG_STEP_ADHOC_RECEIVE_SEARCHING);
      }

      break;
    }
    
    case DIALOG_STEP_ADHOC_SENDED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_RECEIVED:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        refresh = REFRESH_MODE_NORMAL;
        setDialogStep(DIALOG_STEP_NONE);
      }
      
      break;
    }
    
    case DIALOG_STEP_ADHOC_SENDING:
    case DIALOG_STEP_ADHOC_RECEIVING:
    {
      if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
        // Alert sockets
        adhocAlertSockets();
      }
      
      break;
    }
  }

  return refresh;
}

static int fileBrowserMenuCtrl() {
  int refresh = 0;

  // Settings menu
  if (pressed_pad[PAD_START]) {
    openSettingsMenu();
  }

  // SELECT button
  if (pressed_pad[PAD_SELECT]) {
    if (vitashell_config.select_button == SELECT_BUTTON_MODE_USB && sceKernelGetModel() == SCE_KERNEL_MODEL_VITA) {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        SceUdcdDeviceState state;
        sceUdcdGetDeviceState(&state);
        
        if (state.cable & SCE_UDCD_STATUS_CABLE_CONNECTED) {
          initUsb();
        } else {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[USB_NOT_CONNECTED]);
          setDialogStep(DIALOG_STEP_USB_WAIT);
        }
      }
    } else if (vitashell_config.select_button == SELECT_BUTTON_MODE_FTP || sceKernelGetModel() == SCE_KERNEL_MODEL_VITATV) {
      // Init FTP
      if (!ftpvita_is_initialized()) {
        int res = ftpvita_init(vita_ip, &vita_port);
        if (res < 0) {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[PLEASE_WAIT]);
          setDialogStep(DIALOG_STEP_FTP_WAIT);
        } else {
          initFtp();
        }

        // Lock power timers
        powerLock();
      }

      // Dialog
      if (ftpvita_is_initialized()) {
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
        setDialogStep(DIALOG_STEP_FTP);
      }
    }
  }

  // QR
  if (hold_pad[PAD_LTRIGGER] && hold_pad[PAD_RTRIGGER] && enabledQR()) {
    startQR();
    initMessageDialog(MESSAGE_DIALOG_QR_CODE, language_container[QR_SCANNING]);
    setDialogStep(DIALOG_STEP_QR);
  }
  
  // Move  
  if (hold_pad[PAD_UP] || hold2_pad[PAD_LEFT_ANALOG_UP]) {
    int old_pos = base_pos + rel_pos;
    
    if (rel_pos > 0) {
      rel_pos--;
    } else if (base_pos > 0) {
      base_pos--;
    }

    if (old_pos != base_pos + rel_pos) {
      scroll_count = 0;
    }
  } else if (hold_pad[PAD_DOWN] || hold2_pad[PAD_LEFT_ANALOG_DOWN]) {
    int old_pos = base_pos + rel_pos;

    if ((rel_pos + 1) < file_list.length) {
      if ((rel_pos + 1) < MAX_POSITION) {
        rel_pos++;
      } else if ((base_pos + rel_pos + 1) < file_list.length) {
        base_pos++;
      }
    }

    if (old_pos != base_pos + rel_pos) {
      scroll_count = 0;
    }
  }

  // Context menu trigger
  if (pressed_pad[PAD_TRIANGLE]) {
    if (getContextMenuMode() == CONTEXT_MENU_CLOSED) {
      if (dir_level > 0) {
        setContextMenu(&context_menu_main);
        setContextMenuMainVisibilities();
        setContextMenuMode(CONTEXT_MENU_OPENING);
      } else {
        setContextMenu(&context_menu_home);
        setContextMenuHomeVisibilities();
        setContextMenuMode(CONTEXT_MENU_OPENING);
      }
    }
  }

  // Not at 'home'
  if (dir_level > 0) {
    // Mark entry    
    if (pressed_pad[PAD_SQUARE]) {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry && strcmp(file_entry->name, DIR_UP) != 0) {
        if (!fileListFindEntry(&mark_list, file_entry->name)) {
          fileListAddEntry(&mark_list, fileListCopyEntry(file_entry), SORT_NONE);
        } else {
          fileListRemoveEntryByName(&mark_list, file_entry->name);
        }
      }
    }

    // Back
    if (pressed_pad[PAD_CANCEL]) {
      scroll_count = 0;
      fileListEmpty(&mark_list);
      dirUp();
      WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);
      refreshFileList();
    }
  }

  // Handle
  if (pressed_pad[PAD_ENTER]) {
    scroll_count = 0;

    fileListEmpty(&mark_list);

    // Handle file or folder
    FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
    if (file_entry) {
      if (file_entry->is_folder) {
        if (strcmp(file_entry->name, DIR_UP) == 0) {
          dirUp();
        } else {
          if (dir_level == 0) {
            strcpy(file_list.path, file_entry->name);
          } else {
            if (dir_level > 1)
              addEndSlash(file_list.path);
            strcat(file_list.path, file_entry->name);
          }

          dirLevelUp();
        }

        // Save last dir
        WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);

        // Open folder
        int res = refreshFileList();
        if (res < 0)
          errorDialog(res);
      } else {
        snprintf(cur_file, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, file_entry->name);
        int type = handleFile(cur_file, file_entry);

        // Archive mode
        if (type == FILE_TYPE_ARCHIVE && getDialogStep() != DIALOG_STEP_ENTER_PASSWORD) {
          is_in_archive = 1;
          dir_level_archive = dir_level;

          snprintf(archive_path, MAX_PATH_LENGTH - 1, "%s%s", file_list.path, file_entry->name);

          strcat(file_list.path, file_entry->name);
          addEndSlash(file_list.path);

          dirLevelUp();
          refreshFileList();
        }
      }
    }
  }

  return refresh;
}

static int shellMain() {
  // Position
  memset(base_pos_list, 0, sizeof(base_pos_list));
  memset(rel_pos_list, 0, sizeof(rel_pos_list));

  // Paths
  memset(cur_file, 0, sizeof(cur_file));
  memset(archive_path, 0, sizeof(archive_path));

  // File lists
  memset(&file_list, 0, sizeof(FileList));
  memset(&mark_list, 0, sizeof(FileList));
  memset(&copy_list, 0, sizeof(FileList));
  memset(&install_list, 0, sizeof(FileList));

  // Current path is 'home'
  strcpy(file_list.path, HOME_PATH);

  // Last dir
  char lastdir[MAX_PATH_LENGTH];
  ReadFile(VITASHELL_LASTDIR, lastdir, sizeof(lastdir));

  // Calculate dir positions if the dir is valid
  if (checkFolderExist(lastdir)) {
    int i;
    for (i = 0; i < strlen(lastdir) + 1; i++) {
      if (lastdir[i] == ':' || lastdir[i] == '/') {
        char ch = lastdir[i + 1];
        lastdir[i + 1] = '\0';

        char ch2 = lastdir[i];
        lastdir[i] = '\0';

        char *p = strrchr(lastdir, '/');
        if (!p)
          p = strrchr(lastdir, ':');
        if (!p)
          p = lastdir - 1;

        lastdir[i] = ch2;

        refreshFileList();
        setFocusOnFilename(p + 1);

        strcpy(file_list.path, lastdir);

        lastdir[i + 1] = ch;

        dirLevelUp();
      }
    }
  }

  // Refresh file list
  refreshFileList();

  // Init settings menu
  initSettingsMenu();

  while (1) {
    readPad();

    int refresh = REFRESH_MODE_NONE;

    // Control
    if (getDialogStep() != DIALOG_STEP_NONE) {
      refresh = dialogSteps();
      // scroll_count = 0;
    } else if (getAdhocDialogStatus() != ADHOC_DIALOG_CLOSED) {
      adhocDialogCtrl();
    } else if (getPropertyDialogStatus() != PROPERTY_DIALOG_CLOSED) {
      propertyDialogCtrl();
      scroll_count = 0;
    } else if (getSettingsMenuStatus() != SETTINGS_MENU_CLOSED) {
      settingsMenuCtrl();
    } else if (getContextMenuMode() != CONTEXT_MENU_CLOSED) {
      contextMenuCtrl();
    } else {
      refresh = fileBrowserMenuCtrl();
    }

    // Receive system event
    SceAppMgrSystemEvent event;
    sceAppMgrReceiveSystemEvent(&event);

    // Refresh on app resume
    if (event.systemEvent == SCE_APPMGR_SYSTEMEVENT_ON_RESUME) {
      sceShellUtilLock(SCE_SHELL_UTIL_LOCK_TYPE_USB_CONNECTION);
      gameDataUmount(); // umount game data at resume
      refresh = REFRESH_MODE_NORMAL;
    }

    if (refresh != REFRESH_MODE_NONE) {
      // Refresh lists
      refreshFileList();
      refreshMarkList();
      refreshCopyList();

      // Focus
      if (refresh == REFRESH_MODE_SETFOCUS)
        setFocusOnFilename(focus_name);
    }

    // Start drawing
    startDrawing(bg_browser_image);

    // Draw shell info
    drawShellInfo(file_list.path);

    // Draw scroll bar
    drawScrollBar(base_pos, file_list.length);

    // Draw
    FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos);
    if (file_entry) {
      int i;
      for (i = 0; i < MAX_ENTRIES && (base_pos+i) < file_list.length; i++) {
        uint32_t color = FILE_COLOR;
        float y = START_Y + (i * FONT_Y_SPACE);

        vita2d_texture *icon = NULL;

        // Folder
        if (file_entry->is_folder) {
          color = FOLDER_COLOR;
          icon = folder_icon;
        } else {
          switch (file_entry->type) {
            case FILE_TYPE_BMP:
            case FILE_TYPE_PNG:
            case FILE_TYPE_JPEG:
              color = IMAGE_COLOR;
              icon = image_icon;
              break;
              
            case FILE_TYPE_VPK:
            case FILE_TYPE_ARCHIVE:
              color = ARCHIVE_COLOR;
              icon = archive_icon;
              break;
              
            case FILE_TYPE_MP3:
            case FILE_TYPE_OGG:
              color = IMAGE_COLOR;
              icon = audio_icon;
              break;
              
            case FILE_TYPE_SFO:
              color = SFO_COLOR;
              icon = sfo_icon;
              break;
            
            case FILE_TYPE_INI:
            case FILE_TYPE_TXT:
            case FILE_TYPE_XML:
              color = TXT_COLOR;
              icon = text_icon;
              break;
              
            default:
              color = FILE_COLOR;
              icon = file_icon;
              break;
          }
        }

        // Draw icon
        if (icon)
          vita2d_draw_texture(icon, SHELL_MARGIN_X, y + 3.0f);

        // Current position
        if (i == rel_pos)
          color = FOCUS_COLOR;

        // Marked
        if (fileListFindEntry(&mark_list, file_entry->name))
          vita2d_draw_rectangle(SHELL_MARGIN_X, y + 3.0f, MARK_WIDTH, FONT_Y_SPACE, MARKED_COLOR);

        // Draw file name
        vita2d_enable_clipping();
        vita2d_set_clip_rectangle(FILE_X + 1.0f, y, FILE_X + 1.0f + MAX_NAME_WIDTH, y + FONT_Y_SPACE);
        
        float x = FILE_X;
        
        if (i == rel_pos) {
          int width = (int)pgf_text_width(file_entry->name);
          if (width >= MAX_NAME_WIDTH) {
            if (scroll_count < 60) {
              scroll_x = x;
            } else if (scroll_count < width + 90) {
              scroll_x--;
            } else if (scroll_count < width + 120) {
              color = (color & 0x00FFFFFF) | ((((color >> 24) * (scroll_count - width - 90)) / 30) << 24); // fade-in in 0.5s
              scroll_x = x;
            } else {
              scroll_count = 0;
            }
            
            scroll_count++;
            
            x = scroll_x;
          }
        }

        pgf_draw_text(x, y, color, file_entry->name);

        vita2d_disable_clipping();

        // File information
        if (strcmp(file_entry->name, DIR_UP) != 0) {
          if (dir_level == 0) {
            char used_size_string[16], max_size_string[16];
            int max_size_x = ALIGN_RIGHT(INFORMATION_X, pgf_text_width("0000.00 MB"));
            int separator_x = ALIGN_RIGHT(max_size_x, pgf_text_width("  /  "));
            if (file_entry->size != 0 && file_entry->size2 != 0) {
              getSizeString(used_size_string, file_entry->size2 - file_entry->size);
              getSizeString(max_size_string, file_entry->size2);
            } else {
              strcpy(used_size_string, "-");
              strcpy(max_size_string, "-");
            }
            
            float x = ALIGN_RIGHT(INFORMATION_X, pgf_text_width(max_size_string));
            pgf_draw_text(x, y, color, max_size_string);
            pgf_draw_text(separator_x, y, color, "  /");
            x = ALIGN_RIGHT(separator_x, pgf_text_width(used_size_string));
            pgf_draw_text(x, y, color, used_size_string);
          } else {
            char *str = NULL;
            if (!file_entry->is_folder) {
              // Folder/size
              char string[16];
              getSizeString(string, file_entry->size);
              str = string;
            } else {
              str = language_container[FOLDER];
            }
            pgf_draw_text(ALIGN_RIGHT(INFORMATION_X, pgf_text_width(str)), y, color, str);
          }

          // Date
          char date_string[16];
          getDateString(date_string, date_format, &file_entry->mtime);

          char time_string[24];
          getTimeString(time_string, time_format, &file_entry->mtime);

          char string[64];
          sprintf(string, "%s %s", date_string, time_string);

          float x = ALIGN_RIGHT(SCREEN_WIDTH - SHELL_MARGIN_X, pgf_text_width(string));
          pgf_draw_text(x, y, color, string);
        }

        // Next
        file_entry = file_entry->next;
      }
    }

    // Draw settings menu
    drawSettingsMenu();

    // Draw context menu
    drawContextMenu();

    // Draw adhoc dialog
    drawAdhocDialog();
    
    // Draw property dialog
    drawPropertyDialog();

    // End drawing
    endDrawing();
  }

  // Empty lists
  fileListEmpty(&copy_list);
  fileListEmpty(&mark_list);
  fileListEmpty(&file_list);

  return 0;
}

void ftpvita_PROM(ftpvita_client_info_t *client) {
  char cmd[64];
  char path[MAX_PATH_LENGTH];
  sscanf(client->recv_buffer, "%s %s", cmd, path);

  if (installPackage(path) == 0) {
    ftpvita_ext_client_send_ctrl_msg(client, "200 OK PROMOTING\r\n");
  } else {
    ftpvita_ext_client_send_ctrl_msg(client, "500 ERROR PROMOTING\r\n");
  }
}

int main(int argc, const char *argv[]) {  
  // Create mutex
  sceKernelCreateLwMutex(&dialog_mutex, "dialog_mutex", 2, 0, NULL);

  // Init VitaShell
  initVitaShell();

  // No custom config, in case they are damaged or unuseable
  readPad();
  if (current_pad[PAD_LTRIGGER])
    use_custom_config = 0;

  // Load settings
  loadSettingsConfig();

  // Load theme
  loadTheme();

  // Load language
  loadLanguage(language);

  // Init context menu width
  initContextMenuWidth();
  initTextContextMenuWidth();

  // Automatic network update
  if (!vitashell_config.disable_autoupdate) {
    SceUID thid = sceKernelCreateThread("network_update_thread", (SceKernelThreadEntry)network_update_thread, 0x10000100, 0x100000, 0, 0, NULL);
    if (thid >= 0)
      sceKernelStartThread(thid, 0, NULL);
  }

  // Main
  shellMain();

  // Finish VitaShell
  finishVitaShell();
  
  return 0;
}
