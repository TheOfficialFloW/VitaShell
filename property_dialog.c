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
#include "archive.h"
#include "init.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "property_dialog.h"
#include "uncommon_dialog.h"

typedef struct {
  int status;
  int reset;
  float info_x;
  float x;
  float y;
  float width;
  float height;
  float scale;
} PropertyDialog;

static PropertyDialog property_dialog;

static char property_name[256], property_type[32], property_fself_mode[16];
static char property_size[16], property_size_new[16];
static char property_compressed_size[16];
static char property_contains[64], property_contains_new[64];
static char property_creation_date[64], property_modification_date[64];

static int scroll_count = 0;
static float scroll_x = 0;

typedef struct {
  int name;
  int visibility;
  char *entry;
  int entry_size;
} PropertyEntry;

/*
  TODO:
  - Audio information
  - Image information
*/

PropertyEntry property_entries[] = {
  { PROPERTY_NAME, PROPERTY_ENTRY_VISIBLE, property_name, sizeof(property_name) },
  { PROPERTY_TYPE, PROPERTY_ENTRY_VISIBLE, property_type, sizeof(property_type) },
  { PROPERTY_FSELF_MODE, PROPERTY_ENTRY_VISIBLE, property_fself_mode, sizeof(property_fself_mode) },
  { PROPERTY_SIZE, PROPERTY_ENTRY_VISIBLE, property_size, sizeof(property_size) },
  // { PROPERTY_COMPRESSED_SIZE, PROPERTY_ENTRY_VISIBLE, property_compressed_size, sizeof(property_compressed_size) },
  { PROPERTY_CONTAINS, PROPERTY_ENTRY_VISIBLE, property_contains, sizeof(property_contains) },
  { -1, PROPERTY_ENTRY_UNUSED, NULL },
  { PROPERTY_CREATION_DATE, PROPERTY_ENTRY_VISIBLE, property_creation_date, sizeof(property_creation_date) },
  { PROPERTY_MODFICATION_DATE, PROPERTY_ENTRY_VISIBLE, property_modification_date, sizeof(property_modification_date) },
};

enum PropertyEntries {
  PROPERTY_ENTRY_NAME,
  PROPERTY_ENTRY_TYPE,
  PROPERTY_ENTRY_FSELF_MODE,
  PROPERTY_ENTRY_SIZE,
  // PROPERTY_ENTRY_COMPRESSED_SIZE,
  PROPERTY_ENTRY_CONTAINS,
  PROPERTY_ENTRY_EMPTY_1,
  PROPERTY_ENTRY_CREATION_DATE,
  PROPERTY_ENTRY_MODIFICATION_DATE,
};

#define N_PROPERTIES_ENTRIES (sizeof(property_entries) / sizeof(PropertyEntry))

int getPropertyDialogStatus() {
  return property_dialog.status;
}

static float copyStringGetWidth(char *out, char *in) {
  strcpy(out, in);
  return pgf_text_width(out);
}

typedef struct {
  char *path;
} InfoArguments;

SceUID info_thid = -1;
int info_done = 0;

static int propertyCancelHandler() {
  return info_done;
}

static int info_thread(SceSize args_size, InfoArguments *args) {
  uint64_t size = 0;
  uint32_t folders = 0, files = 0;

  info_done = 0;
  if (isInArchive()) {
    getArchivePathInfo(args->path, &size, &folders, &files, propertyCancelHandler);
  } else {
    getPathInfo(args->path, &size, &folders, &files, propertyCancelHandler);
  }
  info_done = 1;

  if (folders > 0)
    folders--;

  getSizeString(property_size_new, size);

  snprintf(property_contains_new, sizeof(property_contains_new), language_container[PROPERTY_CONTAINS_FILES_FOLDERS], files, folders);

  property_dialog.reset = 1;

  return sceKernelExitDeleteThread(0);
}

static void resetWidth() {
  float width = 0.0f, max_width = 0.0f;
  
  width = copyStringGetWidth(property_size, property_size_new);
  if (width > max_width)
    max_width = width;

  width = copyStringGetWidth(property_contains, property_contains_new);
  if (width > max_width)
    max_width = width;

  if (property_dialog.width < property_dialog.info_x + max_width + 2.0f * SHELL_MARGIN_X) {
    property_dialog.width = property_dialog.info_x + max_width + 2.0f * SHELL_MARGIN_X;
    property_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, property_dialog.width);
  }  
}

int initPropertyDialog(char *path, FileListEntry *entry) {
  int i;

  scroll_count = 0;
  scroll_x = 0;

  memset(&property_dialog, 0, sizeof(PropertyDialog));

  // Opening status
  property_dialog.status = PROPERTY_DIALOG_OPENING;

  // Get info x
  property_dialog.info_x = 0.0f;

  for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
    if (property_entries[i].name != -1) {
      float width = pgf_text_width(language_container[property_entries[i].name]);
      if (width > property_dialog.info_x)
        property_dialog.info_x = width;

      property_entries[i].visibility = PROPERTY_ENTRY_VISIBLE;
    } else {
      property_entries[i].visibility = PROPERTY_ENTRY_UNUSED;
    }
  }

  // Extend
  property_dialog.info_x += 2.0f * SHELL_MARGIN_X;

  // Entries
  float width = 0.0f, max_width = 0.0f;

  // Name
  strcpy(property_name, entry->name);
  if (entry->is_folder)
    removeEndSlash(property_name);

  // Type
  int type = entry->is_folder ? FOLDER : FILE_;

  int size = 0;
  uint32_t buffer[0x88/4];

  if (isInArchive()) {
    size = ReadArchiveFile(path, buffer, sizeof(buffer));
  } else {
    size = ReadFile(path, buffer, sizeof(buffer));
  }

  // FSELF mode
  if (*(uint32_t *)buffer == 0x00454353) {
    type = PROPERTY_TYPE_FSELF;

    // Check authid flag
    uint64_t authid = *(uint64_t *)((uint32_t)buffer + 0x80);
    if (authid == 0x2F00000000000001 || authid == 0x2F00000000000003) {      
      width = copyStringGetWidth(property_fself_mode, language_container[PROPERTY_FSELF_MODE_UNSAFE]);
    } else if (authid == 0x2F00000000000002) {
      width = copyStringGetWidth(property_fself_mode, language_container[PROPERTY_FSELF_MODE_SAFE]);
    } else {
      width = copyStringGetWidth(property_fself_mode, language_container[PROPERTY_FSELF_MODE_SCE]);
    }

    if (width > max_width)
      max_width = width;

    property_entries[PROPERTY_ENTRY_FSELF_MODE].visibility = PROPERTY_ENTRY_VISIBLE;
  } else {
    property_entries[PROPERTY_ENTRY_FSELF_MODE].visibility = PROPERTY_ENTRY_INVISIBLE;
  }

  switch (entry->type) {
    case FILE_TYPE_BMP:
      type = PROPERTY_TYPE_BMP;
      break;
      
    case FILE_TYPE_INI:
      type = PROPERTY_TYPE_INI;
      break;
      
    case FILE_TYPE_JPEG:
      type = PROPERTY_TYPE_JPEG;
      break;
      
    case FILE_TYPE_MP3:
      type = PROPERTY_TYPE_MP3;
      break;
      
    case FILE_TYPE_OGG:
      type = PROPERTY_TYPE_OGG;
      break;
      
    case FILE_TYPE_PNG:
      type = PROPERTY_TYPE_PNG;
      break;
      
    case FILE_TYPE_SFO:
      type = PROPERTY_TYPE_SFO;
      break;
      
    case FILE_TYPE_TXT:
      type = PROPERTY_TYPE_TXT;
      break;
      
    case FILE_TYPE_VPK:
      type = PROPERTY_TYPE_VPK;
      break;
      
    case FILE_TYPE_XML:
      type = PROPERTY_TYPE_XML;
      break;
      
    case FILE_TYPE_ARCHIVE:  
      type = PROPERTY_TYPE_ARCHIVE;
      break;
  }

  width = copyStringGetWidth(property_type, language_container[type]);
  if (width > max_width)
    max_width = width;

  // Size & contains
  if (entry->is_folder) {
    strcpy(property_size, "...");
    strcpy(property_contains, "...");

    // Info thread
    InfoArguments info_args;
    info_args.path = path;

    info_thid = sceKernelCreateThread("info_thread", (SceKernelThreadEntry)info_thread, 0x10000100, 0x100000, 0, 0, NULL);
    if (info_thid >= 0)
      sceKernelStartThread(info_thid, sizeof(InfoArguments), &info_args);
    
    property_entries[PROPERTY_ENTRY_CONTAINS].visibility = PROPERTY_ENTRY_VISIBLE;

    // property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_INVISIBLE;
  } else {
    getSizeString(property_size, entry->size);
    property_entries[PROPERTY_ENTRY_CONTAINS].visibility = PROPERTY_ENTRY_INVISIBLE;
    /*
    // Compressed size
    if (isInArchive()) {
      getSizeString(property_compressed_size, entry->size2);
      property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_VISIBLE;
    } else {
      property_entries[PROPERTY_ENTRY_COMPRESSED_SIZE].visibility = PROPERTY_ENTRY_INVISIBLE;
    }*/
  }

  // Dates
  char date_string[16];
  char time_string[24];
  char string[64];

  // Modification date
  getDateString(date_string, date_format, &entry->mtime);
  getTimeString(time_string, time_format, &entry->mtime);
  snprintf(string, sizeof(string), "%s %s", date_string, time_string);
  width = copyStringGetWidth(property_modification_date, string);
  if (width > max_width)
    max_width = width;

  // Creation date
  getDateString(date_string, date_format, &entry->ctime);
  getTimeString(time_string, time_format, &entry->ctime);
  snprintf(string, sizeof(string), "%s %s", date_string, time_string);
  width = copyStringGetWidth(property_creation_date, string);
  if (width > max_width)
    max_width = width;

  // Number of entries
  int j = 0;

  for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
    if (property_entries[i].visibility != PROPERTY_ENTRY_INVISIBLE)
      j++;
  }

  // Width and height
  property_dialog.width = property_dialog.info_x + max_width;
  property_dialog.height = FONT_Y_SPACE * j;

  // For buttons
  property_dialog.height += 2.0f * FONT_Y_SPACE;

  // Margin
  property_dialog.width += 2.0f * SHELL_MARGIN_X;
  property_dialog.height += 2.0f * SHELL_MARGIN_Y;

  // Position
  property_dialog.x = ALIGN_CENTER(SCREEN_WIDTH, property_dialog.width);
  property_dialog.y = ALIGN_CENTER(SCREEN_HEIGHT, property_dialog.height);

  // Align
  int y_n = (int)((float)(property_dialog.y - 2.0f) / FONT_Y_SPACE);
  property_dialog.y = (float)y_n * FONT_Y_SPACE + 2.0f;

  // Scale
  property_dialog.scale = 0.0f;

  return 0;
}

void propertyDialogCtrl() {
  if (pressed_pad[PAD_ENTER]) {
    info_done = 1;
    sceKernelWaitThreadEnd(info_thid, NULL, NULL);
    property_dialog.status = PROPERTY_DIALOG_CLOSING;
  }
}

void drawPropertyDialog() {
  if (property_dialog.status == PROPERTY_DIALOG_CLOSED)
    return;

  // Dialog background
  float dialog_width = vita2d_texture_get_width(dialog_image);
  float dialog_height = vita2d_texture_get_height(dialog_image);
  vita2d_draw_texture_scale_rotate_hotspot(dialog_image, property_dialog.x + property_dialog.width / 2.0f,
                                           property_dialog.y + property_dialog.height / 2.0f,
                                           property_dialog.scale * (property_dialog.width/dialog_width),
                                           property_dialog.scale * (property_dialog.height/dialog_height),
                                           0.0f, dialog_width / 2.0f, dialog_height / 2.0f);

  // Easing out
  if (property_dialog.status == PROPERTY_DIALOG_CLOSING) {
    if (property_dialog.scale > 0.0f) {
      property_dialog.scale -= easeOut(0.0f, property_dialog.scale, 0.25f, 0.01f);
    } else {
      property_dialog.status = PROPERTY_DIALOG_CLOSED;
    }
  }

  if (property_dialog.status == PROPERTY_DIALOG_OPENING) {
    if (property_dialog.scale < 1.0f) {
      property_dialog.scale += easeOut(property_dialog.scale, 1.0f, 0.25f, 0.01f);
    } else {
      property_dialog.status = PROPERTY_DIALOG_OPENED;
    }
  }

  if (property_dialog.reset > 0) {
    if (property_dialog.reset == 1) {
      if (property_dialog.status == PROPERTY_DIALOG_OPENED) {
        property_dialog.status = PROPERTY_DIALOG_CLOSING;
        property_dialog.reset++;
      } else {
        resetWidth();
        property_dialog.reset = 0;
      }
    } else if (property_dialog.reset == 2) {
      if (property_dialog.status == PROPERTY_DIALOG_CLOSED) {
        resetWidth();
        property_dialog.status = PROPERTY_DIALOG_OPENING;
        property_dialog.reset++;
      }
    } else if (property_dialog.reset == 3) {
      if (property_dialog.status == PROPERTY_DIALOG_OPENED) {
        property_dialog.reset = 0;
      }
    }
  }

  if (property_dialog.status == PROPERTY_DIALOG_OPENED) {
    float string_y = property_dialog.y + SHELL_MARGIN_Y - 2.0f;

    int i;
    for (i = 0; i < N_PROPERTIES_ENTRIES; i++) {
      if (property_entries[i].visibility == PROPERTY_ENTRY_VISIBLE) {
        pgf_draw_text(property_dialog.x + SHELL_MARGIN_X, string_y, DIALOG_COLOR, language_container[property_entries[i].name]);

        if (property_entries[i].entry != NULL) {
          uint32_t color = DIALOG_COLOR;
          
          float x = property_dialog.x + SHELL_MARGIN_X + property_dialog.info_x;
          float max_width = property_dialog.width-property_dialog.info_x - 2 * SHELL_MARGIN_X;

          vita2d_enable_clipping();
          vita2d_set_clip_rectangle(x + 1.0f, string_y, x + 1.0f + max_width, string_y + FONT_Y_SPACE);
          
          if (property_entries[i].name == PROPERTY_NAME) {
            int width = (int)pgf_text_width(property_entries[i].entry);
            if (width >= max_width) {
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
          
          pgf_draw_text(x, string_y, color, property_entries[i].entry);

          vita2d_disable_clipping();
        }
      }

      if (property_entries[i].visibility != PROPERTY_ENTRY_INVISIBLE)
        string_y += FONT_Y_SPACE;
    }

    char button_string[128];
    sprintf(button_string, "%s %s", enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE ? CIRCLE : CROSS, language_container[OK]);
    pgf_draw_text(ALIGN_CENTER(SCREEN_WIDTH, pgf_text_width(button_string)), string_y + FONT_Y_SPACE, DIALOG_COLOR, button_string);
  }
}