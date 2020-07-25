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
#include "browser.h"
#include "init.h"
#include "io_process.h"
#include "context_menu.h"
#include "file.h"
#include "language.h"
#include "property_dialog.h"
#include "message_dialog.h"
#include "netcheck_dialog.h"
#include "ime_dialog.h"
#include "utils.h"
#include "usb.h"
#include "pfs.h"

enum MenuHomeEntrys {
  MENU_HOME_ENTRY_REFRESH_LIVEAREA,
  MENU_HOME_ENTRY_REFRESH_LICENSE_DB,
  MENU_HOME_ENTRY_MOUNT_UMA0,
  MENU_HOME_ENTRY_MOUNT_IMC0,
  MENU_HOME_ENTRY_MOUNT_XMC0,
  MENU_HOME_ENTRY_UMOUNT_UMA0,
  MENU_HOME_ENTRY_UMOUNT_IMC0,
  MENU_HOME_ENTRY_UMOUNT_XMC0,
  MENU_HOME_ENTRY_MOUNT_USB_UX0,
  MENU_HOME_ENTRY_UMOUNT_USB_UX0,
  MENU_HOME_ENTRY_MOUNT_GAMECARD_UX0,
  MENU_HOME_ENTRY_UMOUNT_GAMECARD_UX0,
};

MenuEntry menu_home_entries[] = {
  { REFRESH_LIVEAREA,     0, 0, CTX_INVISIBLE },
  { REFRESH_LICENSE_DB,   1, 0, CTX_INVISIBLE },
  { MOUNT_UMA0,           3, 0, CTX_INVISIBLE },
  { MOUNT_IMC0,           4, 0, CTX_INVISIBLE },
  { MOUNT_XMC0,           5, 0, CTX_INVISIBLE },
  { UMOUNT_UMA0,          7, 0, CTX_INVISIBLE },
  { UMOUNT_IMC0,          8, 0, CTX_INVISIBLE },
  { UMOUNT_XMC0,          9, 0, CTX_INVISIBLE },
  { MOUNT_USB_UX0,       11, 0, CTX_INVISIBLE },
  { UMOUNT_USB_UX0,      12, 0, CTX_INVISIBLE },
  { MOUNT_GAMECARD_UX0,  14, 0, CTX_INVISIBLE },
  { UMOUNT_GAMECARD_UX0, 15, 0, CTX_INVISIBLE },
};

#define N_MENU_HOME_ENTRIES (sizeof(menu_home_entries) / sizeof(MenuEntry))

enum MenuMainEntrys {
  MENU_MAIN_ENTRY_OPEN_DECRYPTED,
  MENU_MAIN_ENTRY_MARK_UNMARK_ALL,
  MENU_MAIN_ENTRY_MOVE,
  MENU_MAIN_ENTRY_COPY,
  MENU_MAIN_ENTRY_PASTE,
  MENU_MAIN_ENTRY_DELETE,
  MENU_MAIN_ENTRY_RENAME,
  MENU_MAIN_ENTRY_PROPERTIES,
  MENU_MAIN_ENTRY_NEW,
  MENU_MAIN_ENTRY_SORT_BY,
  MENU_MAIN_ENTRY_MORE,
  MENU_MAIN_ENTRY_ADHOC,
  MENU_MAIN_ENTRY_BOOKMARKS,
};

MenuEntry menu_main_entries[] = {
  { OPEN_DECRYPTED, 0, 0, CTX_INVISIBLE },
  { MARK_ALL,       1, 0, CTX_INVISIBLE },
  { MOVE,           3, 0, CTX_INVISIBLE },
  { COPY,           4, 0, CTX_INVISIBLE },
  { PASTE,          5, 0, CTX_INVISIBLE },
  { DELETE,         7, 0, CTX_INVISIBLE },
  { RENAME,         8, 0, CTX_INVISIBLE },
  { PROPERTIES,     10, 0, CTX_INVISIBLE },
  { NEW,            12, CTX_FLAG_MORE, CTX_VISIBLE },
  { SORT_BY,        13, CTX_FLAG_MORE, CTX_VISIBLE },
  { MORE,           14, CTX_FLAG_MORE, CTX_INVISIBLE },
  { ADHOC_TRANSFER, 16, CTX_FLAG_MORE, CTX_INVISIBLE },
  { BOOKMARKS,      17, CTX_FLAG_MORE, CTX_INVISIBLE },
};

#define N_MENU_MAIN_ENTRIES (sizeof(menu_main_entries) / sizeof(MenuEntry))

enum MenuSortEntrys {
  MENU_SORT_ENTRY_BY_NAME,
  MENU_SORT_ENTRY_BY_SIZE,
  MENU_SORT_ENTRY_BY_DATE,
};

MenuEntry menu_sort_entries[] = {
  { BY_NAME, 13, 0, CTX_INVISIBLE },
  { BY_SIZE, 14, 0, CTX_INVISIBLE },
  { BY_DATE, 15, 0, CTX_INVISIBLE },
};

#define N_MENU_SORT_ENTRIES (sizeof(menu_sort_entries) / sizeof(MenuEntry))

enum MenuBookmarksEntrys {
  MENU_BOOKMARKS_SHOW_BOOKMARKS,
  MENU_BOOKMARKS_RECENT_FILES
};

MenuEntry menu_bookmark_entries[] = {
  { BOOKMARKS_SHOW,    17, 0, CTX_INVISIBLE },
  { RECENT_FILES_SHOW, 18, 0, CTX_INVISIBLE },
};

#define N_MENU_BOOKMARK_ENTRIES (sizeof(menu_bookmark_entries) / sizeof(MenuEntry))

enum MenuAdhocEntrys {
  MENU_ADHOC_SEND,
  MENU_ADHOC_RECEIVE
};

MenuEntry menu_adhoc_entries[] = {
  { SEND,    16, 0, CTX_INVISIBLE },
  { RECEIVE, 17, 0, CTX_INVISIBLE },
};

#define N_MENU_ADHOC_ENTRIES (sizeof(menu_adhoc_entries) / sizeof(MenuEntry))

enum MenuMoreEntrys {
  MENU_MORE_ENTRY_COMPRESS,
  MENU_MORE_ENTRY_INSTALL_ALL,
  MENU_MORE_ENTRY_INSTALL_FOLDER,
  MENU_MORE_ENTRY_EXPORT_MEDIA,
  MENU_MORE_ENTRY_CALCULATE_SHA1,
};

MenuEntry menu_more_entries[] = {
  { COMPRESS,       14, 0, CTX_INVISIBLE },
  { INSTALL_ALL,    15, 0, CTX_INVISIBLE },
  { INSTALL_FOLDER, 16, 0, CTX_INVISIBLE },
  { EXPORT_MEDIA,   17, 0, CTX_INVISIBLE },
  { CALCULATE_SHA1, 18, 0, CTX_INVISIBLE },
};

#define N_MENU_MORE_ENTRIES (sizeof(menu_more_entries) / sizeof(MenuEntry))

enum MenuNewEntrys {
  MENU_NEW_FILE,
  MENU_NEW_FOLDER,
  MENU_NEW_BOOKMARK
};

MenuEntry menu_new_entries[] = {
  { NEW_FILE,      12, 0, CTX_INVISIBLE },
  { NEW_FOLDER,    13, 0, CTX_INVISIBLE },
  { BOOKMARKS_NEW, 14, 0, CTX_INVISIBLE },
};

#define N_MENU_NEW_ENTRIES (sizeof(menu_new_entries) / sizeof(MenuEntry))

static int contextMenuHomeEnterCallback(int sel, void *context);
static int contextMenuMainEnterCallback(int sel, void *context);
static int contextMenuSortEnterCallback(int sel, void *context);
static int contextMenuMoreEnterCallback(int sel, void *context);
static int contextMenuBookmarksEnterCallback(int sel, void *context);
static int contextMenuAdhocEnterCallback(int sel, void *context);
static int contextMenuNewEnterCallback(int sel, void *context);

ContextMenu context_menu_home = {
  .parent = NULL,
  .entries = menu_home_entries,
  .n_entries = N_MENU_HOME_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuHomeEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_main = {
  .parent = NULL,
  .entries = menu_main_entries,
  .n_entries = N_MENU_MAIN_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuMainEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_sort = {
  .parent = &context_menu_main,
  .entries = menu_sort_entries,
  .n_entries = N_MENU_SORT_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuSortEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_more = {
  .parent = &context_menu_main,
  .entries = menu_more_entries,
  .n_entries = N_MENU_MORE_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuMoreEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_bookmarks = {
  .parent = &context_menu_main,
  .entries = menu_bookmark_entries,
  .n_entries = N_MENU_BOOKMARK_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuBookmarksEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_adhoc = {
  .parent = &context_menu_main,
  .entries = menu_adhoc_entries,
  .n_entries = N_MENU_ADHOC_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuAdhocEnterCallback,
  .sel = -1,
};

ContextMenu context_menu_new = {
  .parent = &context_menu_main,
  .entries = menu_new_entries,
  .n_entries = N_MENU_NEW_ENTRIES,
  .max_width = 0.0f,
  .callback = contextMenuNewEnterCallback,
  .sel = -1,
};

void initContextMenuWidth() {
  int i;

  // Home
  for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
    context_menu_home.max_width = MAX(context_menu_home.max_width, pgf_text_width(language_container[menu_home_entries[i].name]));
  }

  context_menu_home.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_home.max_width = MAX(context_menu_home.max_width, CONTEXT_MENU_MIN_WIDTH);

  // Main
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    context_menu_main.max_width = MAX(context_menu_main.max_width,
        pgf_text_width(language_container[menu_main_entries[i].name]));

    if (menu_main_entries[i].name == MARK_ALL) {
      menu_main_entries[i].name = UNMARK_ALL;
      i--;
    }
  }

  context_menu_main.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_main.max_width = MAX(context_menu_main.max_width, CONTEXT_MENU_MIN_WIDTH);

  // Sort
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    context_menu_sort.max_width = MAX(context_menu_sort.max_width, pgf_text_width(language_container[menu_sort_entries[i].name]));
  }

  context_menu_sort.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_sort.max_width = MAX(context_menu_sort.max_width, CONTEXT_MENU_MIN_WIDTH);

  // More
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    context_menu_more.max_width = MAX(context_menu_more.max_width, pgf_text_width(language_container[menu_more_entries[i].name]));
  }

  context_menu_more.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_more.max_width = MAX(context_menu_more.max_width, CONTEXT_MENU_MIN_WIDTH);

  // New
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    context_menu_new.max_width = MAX(context_menu_new.max_width,
        pgf_text_width(language_container[menu_new_entries[i].name]));
  }
  context_menu_new.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_new.max_width = MAX(context_menu_new.max_width, CONTEXT_MENU_MIN_WIDTH);

  // bookmarks
  for (i = 0; i < N_MENU_BOOKMARK_ENTRIES; i++) {
    context_menu_bookmarks.max_width = MAX(context_menu_bookmarks.max_width, pgf_text_width
        (language_container[menu_bookmark_entries[i].name]));
  }

  context_menu_bookmarks.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_bookmarks.max_width = MAX(context_menu_bookmarks.max_width, CONTEXT_MENU_MIN_WIDTH);

  // adhoc
  for (i = 0; i < N_MENU_ADHOC_ENTRIES; i++) {
    context_menu_adhoc.max_width = MAX(context_menu_adhoc.max_width, pgf_text_width
        (language_container[menu_adhoc_entries[i].name]));
  }

  context_menu_adhoc.max_width += 2.0f * CONTEXT_MENU_MARGIN;
  context_menu_adhoc.max_width = MAX(context_menu_adhoc.max_width, CONTEXT_MENU_MIN_WIDTH);


}

void setContextMenuHomeVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
    if (menu_home_entries[i].visibility == CTX_INVISIBLE)
      menu_home_entries[i].visibility = CTX_VISIBLE;
  }

  if (checkFolderExist("uma0:")) {
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_UMA0].visibility = CTX_INVISIBLE;
  } else {
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_USB_UX0].visibility = CTX_INVISIBLE;
  }

  if ((kernel_modid >= 0 || kernel_modid == 0x8002D013) && user_modid >= 0 &&
      shellUserIsUx0Redirected("sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire") == 1) {
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_UMA0].visibility = CTX_INVISIBLE;
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_USB_UX0].visibility = CTX_INVISIBLE;
  } else {
    menu_home_entries[MENU_HOME_ENTRY_UMOUNT_USB_UX0].visibility = CTX_INVISIBLE;
  }

  if (!checkFileExist("sdstor0:gcd-lp-ign-entire")) {
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_GAMECARD_UX0].visibility = CTX_INVISIBLE;
    menu_home_entries[MENU_HOME_ENTRY_UMOUNT_GAMECARD_UX0].visibility = CTX_INVISIBLE;
  } else {
    if ((kernel_modid >= 0 || kernel_modid == 0x8002D013) && user_modid >= 0 &&
        shellUserIsUx0Redirected("sdstor0:gcd-lp-ign-entire", "sdstor0:gcd-lp-ign-entire") == 1) {
      menu_home_entries[MENU_HOME_ENTRY_MOUNT_GAMECARD_UX0].visibility = CTX_INVISIBLE;
    } else {
      menu_home_entries[MENU_HOME_ENTRY_UMOUNT_GAMECARD_UX0].visibility = CTX_INVISIBLE;
    }
  }

  // Invisible if already mounted or there is no internal storage
  if (!checkFileExist("sdstor0:int-lp-ign-userext") || checkFolderExist("imc0:"))
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_IMC0].visibility = CTX_INVISIBLE;

  // Invisible if already mounted or there is no Memory Card
  if (!checkFileExist("sdstor0:xmc-lp-ign-userext") || checkFolderExist("xmc0:"))
    menu_home_entries[MENU_HOME_ENTRY_MOUNT_XMC0].visibility = CTX_INVISIBLE;

  // Invisible if not mounted
  if (!checkFolderExist("uma0:"))
    menu_home_entries[MENU_HOME_ENTRY_UMOUNT_UMA0].visibility = CTX_INVISIBLE;

  // Invisible if not mounted
  if (!checkFolderExist("imc0:"))
    menu_home_entries[MENU_HOME_ENTRY_UMOUNT_IMC0].visibility = CTX_INVISIBLE;

  // Invisible if not mounted
  if (!checkFolderExist("xmc0:"))
    menu_home_entries[MENU_HOME_ENTRY_UMOUNT_XMC0].visibility = CTX_INVISIBLE;

  // Go to first entry
  for (i = 0; i < N_MENU_HOME_ENTRIES; i++) {
    if (menu_home_entries[i].visibility == CTX_VISIBLE) {
      context_menu_home.sel = i;
      break;
    }
  }

  if (i == N_MENU_HOME_ENTRIES)
    context_menu_home.sel = -1;
}

void setContextMenuMainVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    if (menu_main_entries[i].visibility == CTX_INVISIBLE)
      menu_main_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  // menu_main_entries[MENU_MAIN_ENTRY_SEND].flags = CTX_FLAG_BARRIER;
  // menu_main_entries[MENU_MAIN_ENTRY_RECEIVE].flags = 0;

  // Invisble entries when on '..'
  if (strcmp(file_entry->name, DIR_UP) == 0) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_COPY].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_PROPERTIES].visibility = CTX_INVISIBLE;
  }

  // Invisible 'Paste' if nothing is copied yet
  if (copy_list.length == 0) {
    menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
  }

  // Invisible write operations in archives
  // TODO: read-only mount points
  if (isInArchive() || (pfs_mounted_path[0] && strstr(file_list.path, pfs_mounted_path) && read_only)) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_MOVE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_PASTE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_DELETE].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_RENAME].visibility = CTX_INVISIBLE;
    menu_main_entries[MENU_MAIN_ENTRY_NEW].visibility = CTX_INVISIBLE;
  }

  // Mark/Unmark all text
  if (mark_list.length == (file_list.length - 1)) { // All marked
    menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
  } else { // Not all marked yet
    // On marked entry
    if (fileListFindEntry(&mark_list, file_entry->name)) {
      menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
    } else {
      menu_main_entries[MENU_MAIN_ENTRY_MARK_UNMARK_ALL].name = MARK_ALL;
    }
  }

  // Invisible if it's not folder or sce_pfs does not exist
  if (!file_entry->is_folder) {
    menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
  } else {
    char path[MAX_PATH_LENGTH];
    snprintf(path, MAX_PATH_LENGTH, "%s%ssce_pfs", file_list.path, file_entry->name);

    if (!checkFolderExist(path))
      menu_main_entries[MENU_MAIN_ENTRY_OPEN_DECRYPTED].visibility = CTX_INVISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_MAIN_ENTRIES; i++) {
    if (menu_main_entries[i].visibility == CTX_VISIBLE) {
      context_menu_main.sel = i;
      break;
    }
  }

  if (i == N_MENU_MAIN_ENTRIES)
    context_menu_main.sel = -1;
}

void setContextMenuBookmarksVisibilities() {
  int i;
  // All visible
  for (i = 0; i < N_MENU_BOOKMARK_ENTRIES; i++) {
    if (menu_bookmark_entries[i].visibility == CTX_INVISIBLE)
      menu_bookmark_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  if (strcmp(file_list.path, VITASHELL_BOOKMARKS_PATH) == 0) {
    menu_bookmark_entries[MENU_BOOKMARKS_SHOW_BOOKMARKS].visibility = CTX_INVISIBLE;
  }

  if (strcmp(file_list.path, VITASHELL_RECENT_PATH) == 0) {
    menu_bookmark_entries[MENU_BOOKMARKS_RECENT_FILES].visibility = CTX_INVISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_BOOKMARK_ENTRIES; i++) {
    if (menu_bookmark_entries[i].visibility == CTX_VISIBLE) {
      context_menu_bookmarks.sel = i;
      break;
    }
  }

  if (i == N_MENU_BOOKMARK_ENTRIES)
    context_menu_bookmarks.sel = -1;

}
void setContextMenuAdhocVisibilities() {
  int i;
  // All visible
  for (i = 0; i < N_MENU_ADHOC_ENTRIES; i++) {
    if (menu_adhoc_entries[i].visibility == CTX_INVISIBLE)
      menu_adhoc_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  // Invisble entries when on '..'
  if (strcmp(file_entry->name, DIR_UP) == 0) {
    menu_adhoc_entries[MENU_ADHOC_SEND].visibility = CTX_INVISIBLE;
    // menu_adhoc_entries[MENU_ADHOC_RECEIVE].flags = CTX_FLAG_BARRIER;
  }

  // Invisible write operations in archives
  // TODO: read-only mount points
  if (isInArchive() || (pfs_mounted_path[0] && strstr(file_list.path, pfs_mounted_path) && read_only)) {
    menu_adhoc_entries[MENU_ADHOC_SEND].visibility = CTX_INVISIBLE;
    menu_adhoc_entries[MENU_ADHOC_RECEIVE].visibility = CTX_INVISIBLE;
  }
  // Go to first entry
  for (i = 0; i < N_MENU_ADHOC_ENTRIES; i++) {
    if (menu_adhoc_entries[i].visibility == CTX_VISIBLE) {
      context_menu_adhoc.sel = i;
      break;
    }
  }

  if (i == N_MENU_ADHOC_ENTRIES)
    context_menu_adhoc.sel = -1;
}

void setContextMenuSortVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    if (menu_sort_entries[i].visibility == CTX_INVISIBLE)
      menu_sort_entries[i].visibility = CTX_VISIBLE;
  }

  // Invisible when it's the current mode
  if (sort_mode == SORT_BY_NAME)
    menu_sort_entries[MENU_SORT_ENTRY_BY_NAME].visibility = CTX_INVISIBLE;
  else if (sort_mode == SORT_BY_SIZE)
    menu_sort_entries[MENU_SORT_ENTRY_BY_SIZE].visibility = CTX_INVISIBLE;
  else if (sort_mode == SORT_BY_DATE)
    menu_sort_entries[MENU_SORT_ENTRY_BY_DATE].visibility = CTX_INVISIBLE;

  // Go to first entry
  for (i = 0; i < N_MENU_SORT_ENTRIES; i++) {
    if (menu_sort_entries[i].visibility == CTX_VISIBLE) {
      context_menu_sort.sel = i;
      break;
    }
  }

  if (i == N_MENU_SORT_ENTRIES)
    context_menu_sort.sel = -1;
}

void setContextMenuMoreVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    if (menu_more_entries[i].visibility == CTX_INVISIBLE)
      menu_more_entries[i].visibility = CTX_VISIBLE;
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (!file_entry)
    return;

  // Invisble entries when on '..'
  if (strcmp(file_entry->name, DIR_UP) == 0) {
    menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
  }

  // Invisble operations in archives
  if (isInArchive() || (pfs_mounted_path[0] && strstr(file_list.path, pfs_mounted_path) && read_only)) {
    menu_more_entries[MENU_MORE_ENTRY_COMPRESS].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;
  }

  if (file_entry->is_folder) {
    menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_INVISIBLE;

    char check_path[MAX_PATH_LENGTH];

    do {
      if (strcasecmp(file_list.path, "ux0:app/") == 0 ||
          strcasecmp(file_list.path, "ux0:patch/") == 0) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }

      snprintf(check_path, MAX_PATH_LENGTH, "%s%s/eboot.bin", file_list.path, file_entry->name);
      if (!checkFileExist(check_path)) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }

      snprintf(check_path, MAX_PATH_LENGTH, "%s%s/sce_sys/param.sfo", file_list.path, file_entry->name);
      if (!checkFileExist(check_path)) {
        menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
        break;
      }
    } while (0);
  } else {
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_INVISIBLE;
  }

  if(file_entry->type != FILE_TYPE_VPK) {
    menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_INVISIBLE;
  }

  // Invisible export for non-media files
  if (!file_entry->is_folder &&
    file_entry->type != FILE_TYPE_BMP && file_entry->type != FILE_TYPE_JPEG &&
    file_entry->type != FILE_TYPE_PNG && file_entry->type != FILE_TYPE_MP3 &&
    file_entry->type != FILE_TYPE_MP4) {
    menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_INVISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
    if (menu_more_entries[i].visibility == CTX_VISIBLE) {
      context_menu_more.sel = i;
      break;
    }
  }

  if (i == N_MENU_MORE_ENTRIES)
    context_menu_more.sel = -1;
}

void setContextMenuNewVisibilities() {
  int i;

  // All visible
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    if (menu_new_entries[i].visibility == CTX_INVISIBLE)
      menu_new_entries[i].visibility = CTX_VISIBLE;
  }

  // Go to first entry
  for (i = 0; i < N_MENU_NEW_ENTRIES; i++) {
    if (menu_new_entries[i].visibility == CTX_VISIBLE) {
      context_menu_new.sel = i;
      break;
    }
  }

  FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
  if (file_entry) {
    snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
    if (strncmp(cur_file, VITASHELL_BOOKMARKS_PATH, MAX_PATH_LENGTH) == 0) {
      menu_new_entries[MENU_NEW_BOOKMARK].visibility = CTX_INVISIBLE;
    }
    // Invisble entries when on '..'
    if (strcmp(file_entry->name, DIR_UP) == 0 || file_entry->is_symlink) {
      menu_new_entries[MENU_NEW_BOOKMARK].visibility = CTX_INVISIBLE;
    }
  } else {
    menu_new_entries[MENU_NEW_BOOKMARK].visibility = CTX_INVISIBLE;
  }


  if (i == N_MENU_NEW_ENTRIES)
    context_menu_new.sel = -1;
}

static int contextMenuHomeEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_HOME_ENTRY_REFRESH_LIVEAREA:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[REFRESH_LIVEAREA_QUESTION]);
        setDialogStep(DIALOG_STEP_REFRESH_LIVEAREA_QUESTION);
      }

      break;
    }

    case MENU_HOME_ENTRY_REFRESH_LICENSE_DB:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[REFRESH_LICENSE_DB_QUESTION]);
        setDialogStep(DIALOG_STEP_REFRESH_LICENSE_DB_QUESTION);
      }

      break;
    }

    case MENU_HOME_ENTRY_MOUNT_UMA0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        if (checkFileExist("sdstor0:uma-lp-act-entire")) {
          int res = vshIoMount(0xF00, NULL, 0, 0, 0, 0);
          if (res < 0)
            errorDialog(res);
          else
            infoDialog(language_container[UMA0_MOUNTED]);
          refreshFileList();
        } else {
          initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_CANCEL, language_container[USB_WAIT_ATTACH]);
          setDialogStep(DIALOG_STEP_USB_ATTACH_WAIT);
        }
      }

      break;
    }

    case MENU_HOME_ENTRY_MOUNT_IMC0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        int res = vshIoMount(0xD00, NULL, 2, 0, 0, 0);
        if (res < 0)
          errorDialog(res);
        else
          infoDialog(language_container[IMC0_MOUNTED]);
        refreshFileList();
      }

      break;
    }

    case MENU_HOME_ENTRY_MOUNT_XMC0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        int res = vshIoMount(0xE00, NULL, 2, 0, 0, 0);
        if (res < 0)
          errorDialog(res);
        else
          infoDialog(language_container[XMC0_MOUNTED]);
        refreshFileList();
      }

      break;
    }
    
    case MENU_HOME_ENTRY_UMOUNT_UMA0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        vshIoUmount(0xF00, 0, 0, 0);
        vshIoUmount(0xF00, 1, 0, 0);
        infoDialog(language_container[UMA0_UMOUNTED]);
        refreshFileList();
      }

      break;
    }

    case MENU_HOME_ENTRY_UMOUNT_IMC0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        vshIoUmount(0xD00, 0, 0, 0);
        vshIoUmount(0xD00, 1, 0, 0);
        infoDialog(language_container[IMC0_UMOUNTED]);
        refreshFileList();
      }

      break;
    }
    
    case MENU_HOME_ENTRY_UMOUNT_XMC0:
    {
      if (is_safe_mode) {
        infoDialog(language_container[EXTENDED_PERMISSIONS_REQUIRED]);
      } else {
        vshIoUmount(0xE00, 0, 0, 0);
        vshIoUmount(0xE00, 1, 0, 0);
        infoDialog(language_container[XMC0_UMOUNTED]);
        refreshFileList();
      }

      break;
    }
    
    case MENU_HOME_ENTRY_MOUNT_USB_UX0:
    {
      if (mountUsbUx0() >= 0) {
        infoDialog(language_container[USB_UX0_MOUNTED]);
        refreshFileList();
      }
      break;
    }

    case MENU_HOME_ENTRY_UMOUNT_USB_UX0:
    {
      if (umountUsbUx0() >= 0) {
        infoDialog(language_container[USB_UX0_UMOUNTED]);
        refreshFileList();
      }
      break;
    }
    
    case MENU_HOME_ENTRY_MOUNT_GAMECARD_UX0:
    {
      if (mountGamecardUx0() >= 0) {
        infoDialog(language_container[GAMECARD_UX0_MOUNTED]);
        refreshFileList();
      }
      break;
    }

    case MENU_HOME_ENTRY_UMOUNT_GAMECARD_UX0:
    {
      if (umountGamecardUx0() >= 0) {
        infoDialog(language_container[GAMECARD_UX0_UMOUNTED]);
        refreshFileList();
      }
      break;
    }
  }

  return CONTEXT_MENU_CLOSING;
}

static int contextMenuMainEnterCallback(int sel, void *context) {
  switch (sel) {

    case MENU_MAIN_ENTRY_OPEN_DECRYPTED:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char path[MAX_PATH_LENGTH];
        int res;

        pfsUmount();

        snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        res = pfsMount(path);

        // In case we're at ux0:patch or grw0:patch we need to apply the mounting at ux0:app or gro0:app
        if (res < 0) {
          if (strncasecmp(file_list.path, "ux0:patch", 9) == 0 ||
              strncasecmp(file_list.path, "grw0:patch", 10) == 0) {
            snprintf(path, MAX_PATH_LENGTH, "ux0:app/%s", file_entry->name);
            res = pfsMount(path);

            if (res < 0) {
              snprintf(path, MAX_PATH_LENGTH, "gro0:app/%s", file_entry->name);
              res = pfsMount(path);
            }
          }
        }

        if (res < 0)
          errorDialog(res);

        if (res >= 0) {
          addEndSlash(file_list.path);
          strcat(file_list.path, file_entry->name);
          strcpy(pfs_mounted_path, file_list.path);
          dirLevelUp();

          // Save last dir
          WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);

          // Open folder
          int res = refreshFileList();
          if (res < 0)
            errorDialog(res);
        } else {
          errorDialog(res);
        }
      }

      break;
    }

    case MENU_MAIN_ENTRY_MARK_UNMARK_ALL:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        int on_marked_entry = 0;
        int length = mark_list.length;

        if (fileListFindEntry(&mark_list, file_entry->name))
          on_marked_entry = 1;

        // Empty mark list
        fileListEmpty(&mark_list);

        // Mark all if not all entries are marked yet and we are not focusing on a marked entry
        if (length != (file_list.length - 1) && !on_marked_entry) {
          FileListEntry *file_entry = file_list.head->next; // Ignore '..'

          int i;
          for (i = 0; i < file_list.length - 1; i++) {
            fileListAddEntry(&mark_list, fileListCopyEntry(file_entry), SORT_NONE);

            // Next
            file_entry = file_entry->next;
          }
        }
      }

      break;
    }

    case MENU_MAIN_ENTRY_MOVE:
    case MENU_MAIN_ENTRY_COPY:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        // Umount if last path copied from is the pfs mounted path
        if (pfs_mounted_path[0] &&
            !strstr(file_list.path, pfs_mounted_path) &&
             strstr(copy_list.path, pfs_mounted_path)) {
          pfsUmount();
        }

        // Mode
        if (sel == MENU_MAIN_ENTRY_MOVE) {
          copy_mode = COPY_MODE_MOVE;
        } else {
          copy_mode = isInArchive() ? COPY_MODE_EXTRACT : COPY_MODE_NORMAL;
        }

        strcpy(archive_copy_path, archive_path);

        // Empty copy list at first
        fileListEmpty(&copy_list);

        // Paths
        if (fileListFindEntry(&mark_list, file_entry->name)) { // On marked entry
          // Copy mark list to copy list
          FileListEntry *mark_entry = mark_list.head;

          int i;
          for (i = 0; i < mark_list.length; i++) {
            fileListAddEntry(&copy_list, fileListCopyEntry(mark_entry), SORT_NONE);

            // Next
            mark_entry = mark_entry->next;
          }
        } else {
          fileListAddEntry(&copy_list, fileListCopyEntry(file_entry), SORT_NONE);
        }

        strcpy(copy_list.path, file_list.path);
        copy_list.is_in_archive = isInArchive();

        char *message;

        // On marked entry
        if (copy_list.length > 1 && fileListFindEntry(&copy_list, file_entry->name)) {
          message = language_container[COPIED_FILES_FOLDERS];
        } else {
          message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
        }

        // Copy message
        infoDialog(message, copy_list.length);
      }

      break;
    }

    case MENU_MAIN_ENTRY_PASTE:
    {
      int copy_text = 0;

      switch (copy_mode) {
        case COPY_MODE_NORMAL:
          copy_text = COPYING;
          break;

        case COPY_MODE_MOVE:
          copy_text = MOVING;
          break;

        case COPY_MODE_EXTRACT:
          copy_text = EXTRACTING;
          break;
      }

      initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[copy_text]);
      setDialogStep(DIALOG_STEP_PASTE);
      break;
    }

    case MENU_MAIN_ENTRY_DELETE:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char *message;

        // On marked entry
        if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
          message = language_container[DELETE_FILES_FOLDERS_QUESTION];
        } else {
          message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
        }

        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
        setDialogStep(DIALOG_STEP_DELETE_QUESTION);
      }

      break;
    }

    case MENU_MAIN_ENTRY_RENAME:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char name[MAX_NAME_LENGTH];
        strcpy(name, file_entry->name);
        removeEndSlash(name);

        initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);

        setDialogStep(DIALOG_STEP_RENAME);
      }

      break;
    }

    case MENU_MAIN_ENTRY_PROPERTIES:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        initPropertyDialog(cur_file, file_entry);
      }

      break;
    }

    case MENU_MAIN_ENTRY_NEW:
    {
      setContextMenu(&context_menu_new);
      setContextMenuNewVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_MORE:
    {
      setContextMenu(&context_menu_more);
      setContextMenuMoreVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_SORT_BY:
    {
      setContextMenu(&context_menu_sort);
      setContextMenuSortVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_BOOKMARKS:
    {
      setContextMenu(&context_menu_bookmarks);
      setContextMenuBookmarksVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }

    case MENU_MAIN_ENTRY_ADHOC:
    {
      setContextMenu(&context_menu_adhoc);
      setContextMenuAdhocVisibilities();
      return CONTEXT_MENU_MORE_OPENING;
    }
  }

  return CONTEXT_MENU_CLOSING;
}

static int contextMenuSortEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_SORT_ENTRY_BY_NAME:
      sort_mode = SORT_BY_NAME;
      break;

    case MENU_SORT_ENTRY_BY_SIZE:
      sort_mode = SORT_BY_SIZE;
      break;

    case MENU_SORT_ENTRY_BY_DATE:
      sort_mode = SORT_BY_DATE;
      break;
  }

  last_set_sort_mode = sort_mode;

  // Refresh list
  refreshFileList();

  return CONTEXT_MENU_CLOSING;
}
static int contextMenuBookmarksEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_BOOKMARKS_SHOW_BOOKMARKS:
    {
      char path[MAX_PATH_LENGTH] = VITASHELL_BOOKMARKS_PATH;
      jump_to_directory_track_current_path(path);
      break;
    }
    case MENU_BOOKMARKS_RECENT_FILES:
    {
      char path[MAX_PATH_LENGTH] = VITASHELL_RECENT_PATH;
      jump_to_directory_track_current_path(path);
      break;
    }
  }
  return CONTEXT_MENU_CLOSING;
}

static int contextMenuAdhocEnterCallback(int sel, void *context) {
  switch(sel) {
    case MENU_ADHOC_RECEIVE: {
      initNetCheckDialog(SCE_NETCHECK_DIALOG_MODE_PSP_ADHOC_CONN, 0);
      setDialogStep(DIALOG_STEP_ADHOC_RECEIVE_NETCHECK);
      break;
    };
    case MENU_ADHOC_SEND : {
      initNetCheckDialog(SCE_NETCHECK_DIALOG_MODE_PSP_ADHOC_JOIN, 60 * 1000 * 1000);
      setDialogStep(DIALOG_STEP_ADHOC_SEND_NETCHECK);
    }
    break;
  }
  return CONTEXT_MENU_CLOSING;
}


static int contextMenuMoreEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_MORE_ENTRY_COMPRESS:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char path[MAX_NAME_LENGTH];

        // On marked entry
        if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
          int end_slash = removeEndSlash(file_list.path);

          char *p = strrchr(file_list.path, '/');
          if (!p)
            p = strrchr(file_list.path, ':');

          if (strlen(p + 1) > 0) {
            strcpy(path, p + 1);
          } else {
            strncpy(path, file_list.path, p - file_list.path);
            path[p - file_list.path] = '\0';
          }

          if (end_slash)
            addEndSlash(file_list.path);
        } else {
          char *p = strrchr(file_entry->name, '.');
          if (!p)
            p = strrchr(file_entry->name, '/');
          if (!p)
            p = file_entry->name + strlen(file_entry->name);

          strncpy(path, file_entry->name, p-file_entry->name);
          path[p - file_entry->name] = '\0';
        }

        // Append .zip extension
        strcat(path, ".zip");

        initImeDialog(language_container[ARCHIVE_NAME], path, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0, 0);
        setDialogStep(DIALOG_STEP_COMPRESS_NAME);
      }

      break;
    }

    case MENU_MORE_ENTRY_INSTALL_ALL:
    {
      // Empty install list
      fileListEmpty(&install_list);

      FileListEntry *file_entry = file_list.head->next; // Ignore '..'

      int i;
      for (i = 0; i < file_list.length - 1; i++) {
        char path[MAX_PATH_LENGTH];
        snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

        int type = getFileType(path);
        if (type == FILE_TYPE_VPK) {
          fileListAddEntry(&install_list, fileListCopyEntry(file_entry), SORT_NONE);
        }

        // Next
        file_entry = file_entry->next;
      }

      strcpy(install_list.path, file_list.path);

      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_ALL_QUESTION]);
      setDialogStep(DIALOG_STEP_INSTALL_QUESTION);

      break;
    }

    case MENU_MORE_ENTRY_INSTALL_FOLDER:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_FOLDER_QUESTION]);
        setDialogStep(DIALOG_STEP_INSTALL_QUESTION);
      }

      break;
    }

    case MENU_MORE_ENTRY_EXPORT_MEDIA:
    {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        char *message;

        // On marked entry
        if (mark_list.length > 1 && fileListFindEntry(&mark_list, file_entry->name)) {
          message = language_container[EXPORT_FILES_FOLDERS_QUESTION];
        } else {
          message = language_container[file_entry->is_folder ? EXPORT_FOLDER_QUESTION : EXPORT_FILE_QUESTION];
        }

        initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
        setDialogStep(DIALOG_STEP_EXPORT_QUESTION);
      }

      break;
    }

    case MENU_MORE_ENTRY_CALCULATE_SHA1:
    {
      // Ensure user wants to actually take the hash
      initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[HASH_FILE_QUESTION]);
      setDialogStep(DIALOG_STEP_HASH_QUESTION);
      break;
    }
  }

  return CONTEXT_MENU_CLOSING;
}
static int contextMenuNewEnterCallback(int sel, void *context) {
  switch (sel) {
    case MENU_NEW_FILE: {
      char path[MAX_PATH_LENGTH];
      int count = 1;
      while (1) {
        if (count == 1) {
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path,
                   language_container[NEW_FILE]);
        } else {
          snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path,
                   language_container[NEW_FILE], count);
        }
        if (!checkFileExist(path))
          break;

        count++;
      }
      initImeDialog(language_container[NEW_FILE], path + strlen(file_list.path),
                    MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);
      setDialogStep(DIALOG_STEP_NEW_FILE);
      break;
    };
    case MENU_NEW_FOLDER: {
      // Find a new folder name
      char path[MAX_PATH_LENGTH];

      int count = 1;
      while (1) {
        if (count == 1) {
          snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path,
                   language_container[NEW_FOLDER]);
        } else {
          snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path,
                   language_container[NEW_FOLDER], count);
        }

        if (!checkFolderExist(path))
          break;

        count++;
      }

      initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path),
                    MAX_NAME_LENGTH, SCE_IME_TYPE_DEFAULT, 0, 0);
      setDialogStep(DIALOG_STEP_NEW_FOLDER);
      break;
    };
    case MENU_NEW_BOOKMARK: {
      FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
      if (file_entry) {
        snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
        char target[MAX_PATH_LENGTH];
        char name[MAX_PATH_LENGTH];
        strncpy(name, file_entry->name, MAX_PATH_LENGTH-1);
        name[MAX_PATH_LENGTH-1] = '\0';
        removeEndSlash(name);
        snprintf(target, MAX_PATH_LENGTH, "%s%s."SYMLINK_EXT, VITASHELL_BOOKMARKS_PATH, name);
        int res;
        if ((res = createSymLink(target, cur_file)) < 0) {
          errorDialog(res);
        } else {
          infoDialog(language_container[BOOKMARK_CREATED]);
        }
      } else {
        errorDialog(-2);
      }
      break;
    }
  }
  // Refresh list
  refreshFileList();

  return CONTEXT_MENU_CLOSING;
}
