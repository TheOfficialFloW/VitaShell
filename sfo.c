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
#include "archive.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"

int getSfoValue(void *buffer, const char *name, uint32_t *value) {
  SfoHeader *header = (SfoHeader *)buffer;
  SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

  if (header->magic != SFO_MAGIC)
    return VITASHELL_ERROR_INVALID_MAGIC;

  int i;
  for (i = 0; i < header->count; i++) {
    if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
      *value = *(uint32_t *)(buffer + header->valofs + entries[i].dataofs);
      return 0;
    }
  }

  return VITASHELL_ERROR_NOT_FOUND;
}

int getSfoString(void *buffer, const char *name, char *string, int length) {
  SfoHeader *header = (SfoHeader *)buffer;
  SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

  if (header->magic != SFO_MAGIC)
    return VITASHELL_ERROR_INVALID_MAGIC;

  int i;
  for (i = 0; i < header->count; i++) {
    if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
      memset(string, 0, length);
      strncpy(string, buffer + header->valofs + entries[i].dataofs, length);
      string[length - 1] = '\0';
      return 0;
    }
  }

  return VITASHELL_ERROR_NOT_FOUND;
}

int setSfoValue(void *buffer, const char *name, uint32_t value) {
  SfoHeader *header = (SfoHeader *)buffer;
  SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

  if (header->magic != SFO_MAGIC)
    return VITASHELL_ERROR_INVALID_MAGIC;

  int i;
  for (i = 0; i < header->count; i++) {
    if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
      *(uint32_t *)(buffer + header->valofs + entries[i].dataofs) = value;
      return 0;
    }
  }

  return VITASHELL_ERROR_NOT_FOUND;
}

int setSfoString(void *buffer, const char *name, const char *string) {
  SfoHeader *header = (SfoHeader *)buffer;
  SfoEntry *entries = (SfoEntry *)((uint32_t)buffer + sizeof(SfoHeader));

  if (header->magic != SFO_MAGIC)
    return VITASHELL_ERROR_INVALID_MAGIC;

  int i;
  for (i = 0; i < header->count; i++) {
    if (strcmp(buffer + header->keyofs + entries[i].nameofs, name) == 0) {
      strcpy(buffer + header->valofs + entries[i].dataofs, string);
      return 0;
    }
  }

  return VITASHELL_ERROR_NOT_FOUND;
}

int SFOReader(const char *file) {
  uint8_t *buffer = memalign(4096, BIG_BUFFER_SIZE);
  if (!buffer)
    return VITASHELL_ERROR_NO_MEMORY;

  int size = 0;

  if (isInArchive()) {
    size = ReadArchiveFile(file, buffer, BIG_BUFFER_SIZE);
  } else {
    size = ReadFile(file, buffer, BIG_BUFFER_SIZE);
  }

  if (size <= 0) {
    free(buffer);
    return size;
  }

  SfoHeader *sfo_header = (SfoHeader *)buffer;
  if (sfo_header->magic != SFO_MAGIC)
    return VITASHELL_ERROR_INVALID_MAGIC;

  int base_pos = 0, rel_pos = 0;

  int scroll_count = 0;
  float scroll_x = FILE_X;

  while (1) {
    readPad();

    if (pressed_pad[PAD_CANCEL]) {
      break;
    }

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

      if ((rel_pos + 1) < sfo_header->count) {
        if ((rel_pos + 1) < MAX_POSITION) {
          rel_pos++;
        } else if ((base_pos + rel_pos + 1) < sfo_header->count) {
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
        if (base_pos >= (int)sfo_header->count - MAX_POSITION) {
          base_pos = MAX((int)sfo_header->count - MAX_POSITION, 0);
          rel_pos = MIN(MAX_POSITION - 1, (int)sfo_header->count - 1);
        }
      }

      if (old_pos != base_pos + rel_pos) {
        scroll_count = 0;
      }
    }

    // Start drawing
    startDrawing(bg_text_image);

    // Draw
    drawShellInfo(file);
    drawScrollBar(base_pos, sfo_header->count);

    int i;
    for (i = 0; i < MAX_ENTRIES && (base_pos + i) < sfo_header->count; i++) {
      SfoEntry *entries = (SfoEntry *)(buffer + sizeof(SfoHeader) + (sizeof(SfoEntry) * (i + base_pos)));

      uint32_t color = (rel_pos == i) ? TEXT_FOCUS_COLOR : TEXT_COLOR;

      char *name = (char *)(buffer + sfo_header->keyofs + entries->nameofs);
      float name_x = pgf_draw_text(SHELL_MARGIN_X, START_Y + (FONT_Y_SPACE * i), color, name);

      char string[128];

      void *data = (void *)(buffer + sfo_header->valofs + entries->dataofs);
      switch (entries->type) {
        case PSF_TYPE_BIN:
        {
          string[0] = '\0';

          int i;
          for (i = 0; i < entries->valsize; i++) {
            char ch[4];
            sprintf(ch, "%02X", ((uint8_t *)data)[i]);
            strlcat(string, ch, sizeof(string) - 1);
          }

          break;
        }

        case PSF_TYPE_STR:
        {
          strncpy(string, (char *)data, sizeof(string) - 1);
          int len = strlen(string);
          int i;
          for (i = 0; i < len; i++)
            if (string[i] == '\n')
              string[i] = ' ';
          break;
        }

        case PSF_TYPE_VAL:
        {
          snprintf(string, sizeof(string), "0x%X", *(unsigned int *)data);
          break;
        }
      }

      // Draw
      float width = pgf_text_width(string);
      float aligned_x = ALIGN_RIGHT(INFORMATION_X, width);
      float min_x = aligned_x >= (name_x + 100.0f) ? aligned_x : (name_x + 100.0f);
      float y = START_Y + (FONT_Y_SPACE * i);

      vita2d_enable_clipping();
      vita2d_set_clip_rectangle(min_x + 1.0f, y, INFORMATION_X + 1.0f, y + FONT_Y_SPACE);

      float x = min_x;

      if (i == rel_pos) {
        int width_int = (int)width;
        if (aligned_x < (name_x + 100.0f)) {
          if (scroll_count < 60) {
            scroll_x = x;
          } else if (scroll_count < width_int + 90) {
            scroll_x--;
          } else if (scroll_count < width_int + 120) {
            color = (color & 0x00FFFFFF) | ((((color >> 24) * (scroll_count - width_int - 90)) / 30) << 24); // fade-in in 0.5s
            scroll_x = x;
          } else {
            scroll_count = 0;
          }

          scroll_count++;

          x = scroll_x;
        }
      }

      pgf_draw_text(x, y, color, string);

      vita2d_disable_clipping();
    }

    // End drawing
    endDrawing();
  }

  free(buffer);
  return 0;
}
