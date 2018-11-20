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
#include "browser.h"
#include "init.h"
#include "io_process.h"
#include "refresh.h"
#include "makezip.h"
#include "package_installer.h"
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
#include "pfs.h"

// File lists
FileList file_list, mark_list, copy_list, install_list;

// Paths
char cur_file[MAX_PATH_LENGTH];
char archive_copy_path[MAX_PATH_LENGTH];
char archive_path[MAX_PATH_LENGTH];

static char focus_name[MAX_NAME_LENGTH];

// Position
int base_pos = 0, rel_pos = 0;
static int base_pos_list[MAX_DIR_LEVELS];
static int rel_pos_list[MAX_DIR_LEVELS];
static int dir_level = 0;

// Modes
int sort_mode = SORT_BY_NAME;
int last_set_sort_mode = SORT_BY_NAME;
int copy_mode = COPY_MODE_NORMAL;
int file_type = FILE_TYPE_UNKNOWN;

// Archive
static int is_in_archive = 0;
static char dir_level_archive = -1;

// Scrolling filename
static int scroll_count = 0;
static float scroll_x = FILE_X;

SceInt64 time_last_recent_files, time_last_bookmarks;

static void fileBrowserHandleSymlink(FileListEntry* file_entry);
static void fileBrowserHandleFolder(FileListEntry* file_entry);
static void fileBrowserHandleFile(FileListEntry* file_entry);

static void create_recent_symlink(FileListEntry *file_entry);

// escape from dir hierarchy with a symlink
typedef struct SymlinkDirectoryPath {
  struct SymlinkDirectoryPath* previous;
  // contains / at the end, is directory where jumped from
  char last_path[MAX_PATH_LENGTH];
  // contains / at the end, is directory where jumped to
  char last_hook[MAX_PATH_LENGTH];
} SymlinkDirectoryPath;

static SymlinkDirectoryPath* symlink_directory_path = NULL;
static SymlinkDirectoryPath* symlink_directory_path_head = NULL;

static void storeSymlinkPath(SymlinkDirectoryPath * path) {
  if (!symlink_directory_path) {
    symlink_directory_path = path;
    symlink_directory_path_head = path;
    symlink_directory_path->previous = 0;
  } else {
    SymlinkDirectoryPath *prev = symlink_directory_path;
    symlink_directory_path = path;
    symlink_directory_path->previous = prev;
  }
}

void dirLevelUp() {
  if (dir_level < MAX_DIR_LEVELS - 1) {
    base_pos_list[dir_level] = base_pos;
    rel_pos_list[dir_level] = rel_pos;
    dir_level++;
    base_pos_list[dir_level] = 0;
    rel_pos_list[dir_level] = 0;
  }

  base_pos = 0;
  rel_pos = 0;
}

void setDirArchiveLevel() {
  dir_level_archive = dir_level;
}

void setInArchive() {
  is_in_archive = 1;
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

int change_to_directory(char *lastdir) {
  if (!checkFolderExist(lastdir)) {
    return VITASHELL_ERROR_NAVIGATION;
  } else {
    if (isInArchive()) {
      dirUpCloseArchive();
    }
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
  refreshFileList();
  return 0;
}

static void dirUp() {
  if (pfs_mounted_path[0] &&
      strcmp(file_list.path, pfs_mounted_path) == 0 && // we're about to leave the pfs path
      !strstr(copy_list.path, pfs_mounted_path)) { // nothing has been copied from pfs path
    // Then umount
    pfsUmount();
  }

  // skip all symlink hierarchies when pressing O in bookmarks/ recent files
  if (symlink_directory_path_head &&
      ((strncmp(file_list.path, VITASHELL_BOOKMARKS_PATH, MAX_PATH_LENGTH) == 0)
       || strncmp(file_list.path, VITASHELL_RECENT_PATH, MAX_PATH_LENGTH) == 0)) {
    strcpy(file_list.path, symlink_directory_path_head->last_path);
    SymlinkDirectoryPath *e = symlink_directory_path;
    while(e != NULL) {
      SymlinkDirectoryPath *prev = e->previous;
      free(e);
      dir_level--;
      e = prev;
    }
    symlink_directory_path_head = 0;
    symlink_directory_path = 0;
    goto DIR_UP_RETURN;
  }

  if (symlink_directory_path
      && strncmp(file_list.path, symlink_directory_path->last_hook, MAX_PATH_LENGTH) == 0) {
    strcpy(file_list.path, symlink_directory_path->last_path);
    SymlinkDirectoryPath* prev = symlink_directory_path->previous;
    free(symlink_directory_path);
    symlink_directory_path = prev;
    dir_level--;
    goto DIR_UP_RETURN;
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
  if (dir_level < 0)
    dir_level = 0;

  base_pos = (int)base_pos_list[dir_level];
  rel_pos = (int)rel_pos_list[dir_level];
  dirUpCloseArchive();
}

void setFocusName(const char *name) {
  strncpy(focus_name, name, MAX_NAME_LENGTH-1);
  focus_name[MAX_NAME_LENGTH-1] = '\0';
}

void setFocusOnFilename(const char *name) {
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

  // always sort recent files by date
  char *contains = strstr(file_list.path, VITASHELL_RECENT_PATH);
  if (contains) {
    sort_mode = SORT_BY_DATE;
  } else {
    sort_mode = last_set_sort_mode;
  }

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
    snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, entry->name);

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
    snprintf(path, MAX_PATH_LENGTH, "%s%s", copy_list.path, entry->name);

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

	// try to fix GPU freeze
	vita2d_wait_rendering_done();

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

int shortCuts() {
  // bookmarks shortcut
  if (current_pad[PAD_SQUARE]) {
    SceInt64 now = sceKernelGetSystemTimeWide();

    // switching too quickly back and forth between recent and bookmarks
    // causes VS to crash
    if (now - time_last_bookmarks > THRESHOLD_LAST_PAD_BOOKMARKS_WAIT) {
      if (strncmp(file_list.path, VITASHELL_BOOKMARKS_PATH, MAX_PATH_LENGTH) != 0) {
        char path[MAX_PATH_LENGTH] = VITASHELL_BOOKMARKS_PATH;
        sort_mode = last_set_sort_mode;
        jump_to_directory_track_current_path(path);
        time_last_bookmarks = now;
        return 0;
      }
    }
  }

  // recent files shortcut
  if (current_pad[PAD_TRIANGLE]) {
    SceInt64 now = sceKernelGetSystemTimeWide();
    if (now - time_last_recent_files > THRESHOLD_LAST_PAD_RECENT_FILES_WAIT) {
      if (strncmp(file_list.path, VITASHELL_RECENT_PATH, MAX_PATH_LENGTH) != 0) {
        char path[MAX_PATH_LENGTH] = VITASHELL_RECENT_PATH;
        sort_mode = SORT_BY_DATE;
        jump_to_directory_track_current_path(path);
        time_last_recent_files = now;
        return 0;
      }
    }
  }

  // QR
  if (current_pad[PAD_CIRCLE] && enabledQR()) {
    startQR();
    initMessageDialog(MESSAGE_DIALOG_QR_CODE, language_container[QR_SCANNING]);
    setDialogStep(DIALOG_STEP_QR);
  }

  return 0;
}

static int fileBrowserMenuCtrl() {
  int refresh = 0;

  // Short cuts combo
  if (current_pad[PAD_LEFT])
    return shortCuts();

  // Settings menu
  if (pressed_pad[PAD_START]) {
    openSettingsMenu();
  }

  // SELECT button
  if (pressed_pad[PAD_SELECT]) {
    if (vitashell_config.select_button == SELECT_BUTTON_MODE_USB &&
        sceKernelGetModel() == SCE_KERNEL_MODEL_VITA) {
      if (isSafeMode()) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        SceUdcdDeviceState state;
        sceUdcdGetDeviceState(&state);

        if (state.cable & SCE_UDCD_STATUS_CABLE_CONNECTED) {
          initUsb();
        } else {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL,
                            language_container[USB_NOT_CONNECTED]);
          setDialogStep(DIALOG_STEP_USB_WAIT);
        }
      }
    } else if (vitashell_config.select_button == SELECT_BUTTON_MODE_FTP ||
               sceKernelGetModel() == SCE_KERNEL_MODEL_VITATV) {
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
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER],
                          vita_ip, vita_port);
        setDialogStep(DIALOG_STEP_FTP);
      }
    }
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

    if ((old_pos + 1) < file_list.length) {
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

  // Page skip
  if (hold_pad[PAD_LTRIGGER] || hold_pad[PAD_RTRIGGER]) {
    int old_pos = base_pos + rel_pos;

    if (hold_pad[PAD_LTRIGGER]) { // Skip page up
      base_pos = base_pos - MAX_ENTRIES;
      if (base_pos < 0) {
        base_pos = 0;
        rel_pos = 0;
      }
    } else { // Skip page down
      base_pos = base_pos + MAX_ENTRIES;
      if (base_pos >= file_list.length - MAX_POSITION) {
        base_pos = MAX(file_list.length - MAX_POSITION, 0);
        rel_pos = MIN(MAX_POSITION - 1, file_list.length - 1);
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

    // Handle file, symlink or folder
    FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
    if (file_entry) {
      if (file_entry->is_symlink) {
        fileBrowserHandleSymlink(file_entry);
      } else if (file_entry->is_folder) {
        fileBrowserHandleFolder(file_entry);
      } else {
        fileBrowserHandleFile(file_entry);
        create_recent_symlink(file_entry);
      }
    }
  }
  return refresh;
}

static void create_recent_symlink(FileListEntry *file_entry) {
  if (isInArchive()) return;

  char target[MAX_PATH_LENGTH];
  snprintf(target, MAX_PATH_LENGTH, "%s%s."SYMLINK_EXT, VITASHELL_RECENT_PATH,
               file_entry->name);
  snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

  // create file recent symlink
  createSymLink(target, cur_file);
}

static void fileBrowserHandleFile(FileListEntry *file_entry) {
  snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
  int type = handleFile(cur_file, file_entry);

  // Archive mode
  if (type == FILE_TYPE_ARCHIVE && getDialogStep() != DIALOG_STEP_ENTER_PASSWORD) {
    is_in_archive = 1;
    dir_level_archive = dir_level;

    snprintf(archive_path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

    strcat(file_list.path, file_entry->name);
    addEndSlash(file_list.path);

    dirLevelUp();
    refreshFileList();
  }
}

static void fileBrowserHandleFolder(FileListEntry *file_entry) {
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
}

// escape from dir level structure so that parent directory is browsed
// where this jump came from and not the hierarchically higher folder
int jump_to_directory_track_current_path(char *path) {
  SymlinkDirectoryPath *symlink_path = malloc(sizeof(SymlinkDirectoryPath));

  int archive = 0;
  while(isInArchive()) {
    archive = 1;
    dirUp();
  }
  if (archive) {
    dirUpCloseArchive();
  }

  if (symlink_path) {
    strncpy(symlink_path->last_path, file_list.path, MAX_PATH_LENGTH);
    strncpy(symlink_path->last_hook, path, MAX_PATH_LENGTH);
    dirLevelUp();
    int _dir_level = dir_level; // we escape from hierarchical dir level structure
    if (change_to_directory(path) < 0) {
      free(symlink_path);
      return VITASHELL_ERROR_NAVIGATION;
    }
    WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);
    storeSymlinkPath(symlink_path);
    dir_level = _dir_level;
    refreshFileList();
  }
  return 0;
}

static void fileBrowserHandleSymlink(FileListEntry *file_entry) {
  if ((file_entry->symlink->to_file == 1 && !checkFileExist(file_entry->symlink->target_path))
      || (file_entry->symlink->to_file == 0 && !checkFolderExist(file_entry->symlink->target_path))) {
    // TODO: What if in archive?
    snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
    textViewer(cur_file);
    return;
  }
  if (file_entry->symlink->to_file == 0) {
    if (jump_to_directory_track_current_path(file_entry->symlink->target_path) < 0) {
      errorDialog(VITASHELL_ERROR_SYMLINK_INVALID_PATH);
    }
  } else {
    char *target_base_directory = getBaseDirectory(file_entry->symlink->target_path);
    if (!target_base_directory) {
      errorDialog(VITASHELL_ERROR_SYMLINK_CANT_RESOLVE_BASEDIR);
      return;
    }
    char *target_file_name = getFilename(file_entry->symlink->target_path);
    if (!target_file_name) {
      errorDialog(VITASHELL_ERROR_SYMLINK_CANT_RESOLVE_FILENAME);
      return;
    }
    if (jump_to_directory_track_current_path(target_base_directory) < 0) {
      errorDialog(VITASHELL_ERROR_SYMLINK_INVALID_PATH);
      return;
    }
    FileListEntry *resolved_file_entry = fileListFindEntry(&file_list, target_file_name);
    if (!resolved_file_entry) {
      errorDialog(VITASHELL_ERROR_SYMLINK_INVALID_PATH);
      return;
    }
    fileBrowserHandleFile(resolved_file_entry);
    dirUp();

    free(target_file_name);
    free(target_base_directory);
  }
  int res = refreshFileList();
  if (res < 0)
    errorDialog(res);
}

int browserMain() {
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

  if (use_custom_config) {
    // Last dir
    char lastdir[MAX_PATH_LENGTH];
    ReadFile(VITASHELL_LASTDIR, lastdir, sizeof(lastdir));
    change_to_directory(lastdir);
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
      pfsUmount(); // umount game data at resume
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

    // Draw
    drawShellInfo(file_list.path);
    drawScrollBar(base_pos, file_list.length);

    // Draw
    FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos);
    if (file_entry) {
      int i;
      for (i = 0; i < MAX_ENTRIES && (base_pos + i) < file_list.length; i++) {
        uint32_t color = FILE_COLOR;
        float y = START_Y + (i * FONT_Y_SPACE);

        vita2d_texture *icon = NULL;
        if (file_entry->is_symlink) {
          if (file_entry->symlink->to_file) {
            color = FILE_SYMLINK_COLOR;
            icon = file_symlink_icon;
          } else {
            color = FOLDER_SYMLINK_COLOR;
            icon = folder_symlink_icon;
          }
        }
        // Folder
        else if (file_entry->is_folder) {
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

        char file_name[MAX_PATH_LENGTH];
        memset(file_name, 0, sizeof(MAX_PATH_LENGTH));

        if (file_entry->is_symlink) {
          snprintf(file_name, MAX_PATH_LENGTH, "%s  → %s",
                   file_entry->name, file_entry->symlink->target_path);
        } else {
          strncpy(file_name, file_entry->name, file_entry->name_length + 1);
          file_name[file_entry->name_length] = '\0';
        }
        if (i == rel_pos) {
          int width = (int)pgf_text_width(file_name);
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

        pgf_draw_text(x, y, color, file_name);

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

    // Draw
    drawSettingsMenu();
    drawContextMenu();
    drawAdhocDialog();
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
