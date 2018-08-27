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
#include "file.h"
#include "config.h"

static void trim(char *str) {
  int len = strlen(str);
  int i;

  for (i = len - 1; i >= 0; i--) {
    if (str[i] == 0x20 || str[i] == '\t') {
      str[i] = 0;
    } else {
      break;
    }
  }
}

static int GetLine(char *buf, int size, char *str) {
  uint8_t ch = 0;
  int n = 0;
  int i = 0;
  uint8_t *s = (uint8_t *)str;

  while (1) {
    if (i >= size)
      break;

    ch = ((uint8_t *)buf)[i];

    if (ch < 0x20 && ch != '\t') {
      if (n != 0) {
        i++;
        break;
      }
    } else {
      *str++ = ch;
      n++;
    }

    i++;
  }

  trim((char *)s);

  return i;
}

static int getDecimal(const char *str) {
  return strtol(str, NULL, 0);
}

static int getHexdecimal(const char *str) {
  return strtoul(str, NULL, 16);
}

static int getBoolean(const char *str) {
  if (strcasecmp(str, "false") == 0 ||
      strcasecmp(str, "off") == 0 ||
      strcasecmp(str, "no") == 0)
    return 0;

  if (strcasecmp(str, "true") == 0 ||
      strcasecmp(str, "on") == 0 ||
      strcasecmp(str, "yes") == 0)
    return 1;

  return -1;
}

static char *getString(const char *str) {
  if (str[0] != '"')
    return NULL;

  char *p = strchr(str + 1, '"');
  if (!p)
    return NULL;

  int len = p - (str + 1);

  char *out = malloc(len + 1);
  strncpy(out, str + 1, len);
  out[len] = '\0';

  int i;
  for (i = 0; i < len; i++) {
    if (out[i] == '\\')
      out[i] = '\n';
  }

  return out;
}

static int readEntry(const char *line, ConfigEntry *entries, int n_entries) {
  // Trim at beginning
  while (*line == ' ' || *line == '\t')
    line++;

  // Ignore comments #1
  if (line[0] == '#') {
    // debugPrintf("IGNORE %s\n", line);
    return 0;
  }

  // Ignore comments #2
  char *p = strchr(line, '#');
  if (p) {
    // debugPrintf("IGNORE %s\n", p);
    *p = '\0';
  }

  // Get token
  p = strchr(line, '=');
  if (!p)
    return -1;

  // Name of entry
  char name[MAX_CONFIG_NAME_LENGTH];
  int len = p - line;
  if (len > MAX_CONFIG_NAME_LENGTH - 1)
    len = MAX_CONFIG_NAME_LENGTH - 1;
  strncpy(name, line, len);
  name[len] = '\0';

  trim(name);

  // debugPrintf("NAME: %s\n", name);

  // String of entry
  char *string = p + 1;

  // Trim at beginning
  while (*string == ' ' || *string == '\t')
    string++;

  // Trim at end
  trim(string);

  // debugPrintf("STRING: %s\n", string);

  int i;
  for (i = 0; i < n_entries; i++) {
    if (strcasecmp(name, entries[i].name) == 0) {
      switch (entries[i].type) {
        case CONFIG_TYPE_DECIMAL:
          *(uint32_t *)entries[i].value = getDecimal(string);
          // debugPrintf("VALUE DECIMAL: %d\n", *(uint32_t *)entries[i].value);
          break;
          
        case CONFIG_TYPE_HEXDECIMAL:
          *(uint32_t *)entries[i].value = getHexdecimal(string);
          // debugPrintf("VALUE HEXDECIMAL: 0x%X\n", *(uint32_t *)entries[i].value);
          break;
          
        case CONFIG_TYPE_BOOLEAN:
          *(uint32_t *)entries[i].value = getBoolean(string);
          // debugPrintf("VALUE BOOLEAN: %d\n", *(uint32_t *)entries[i].value);
          break;
          
        case CONFIG_TYPE_STRING:
        {
          char *value = getString(string);
          if (value) {
            *(uint32_t *)entries[i].value = (uint32_t)value;
            // debugPrintf("VALUE STRING: %s\n", *(uint32_t *)entries[i].value);
          }

          break;
        }
      }

      break;
    }
  }

  return 1;
}

int readConfigBuffer(void *buffer, int size, ConfigEntry *entries, int n_entries) {
  int res = 0;
  char line[MAX_CONFIG_LINE_LENGTH];
  char *p = buffer;

  // Skip UTF-8 bom
  uint32_t bom = 0xBFBBEF;
  if (memcmp(p, &bom, 3) == 0) {
    p += 3;
    size -= 3;
  }

  do {
    memset(line, 0, sizeof(line));
    res = GetLine(p, size, line);

    if (res > 0) {
      readEntry(line, entries, n_entries);
      size -= res;
      p += res;
    }
  } while (res > 0);

  return 0;
}

int readConfig(const char *path, ConfigEntry *entries, int n_entries) {
  void *buffer = NULL;
  int size = allocateReadFile(path, &buffer);
  if (size < 0)
    return size;

  readConfigBuffer(buffer, size, entries, n_entries);

  free(buffer);

  return 0;
}

static int writeEntry(SceUID fd, ConfigEntry *entry) {
  if (!entry->value)
    return -1;

  int result;
  if ((result = sceIoWrite(fd, entry->name, strlen(entry->name))) < 0)
    return result;

  if ((result = sceIoWrite(fd, " = ", 3)) < 0)
    return result;

  char *val;
  char buffer[33];

  switch (entry->type) {
    case CONFIG_TYPE_BOOLEAN:
      val = *(uint32_t *)entry->value != 0 ? "true" : "false";
      result = sceIoWrite(fd, val, strlen(val));
      break;
      
    case CONFIG_TYPE_DECIMAL:
      itoa(*(int *)entry->value, buffer, 10);
      result = sceIoWrite(fd, buffer, strlen(buffer));
      break;
      
    case CONFIG_TYPE_HEXDECIMAL:
      itoa(*(int *)entry->value, buffer, 16);
      result = sceIoWrite(fd, buffer, strlen(buffer));
      break;
      
    case CONFIG_TYPE_STRING:
      val = *(char **)entry->value;
      sceIoWrite(fd, "\"", 1);
      result = sceIoWrite(fd, val, strlen(val));
      sceIoWrite(fd, "\"", 1);
      break;
      
    default:
      return 1;
  }

  if (result < 0)
    return result;

  if ((sceIoWrite(fd, "\n", 1)) < 0)
    return result;

  return 0;
}

int writeConfig(const char *path, ConfigEntry *entries, int n_entries) {
  SceUID fd = sceIoOpen(path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0)
    return fd;

  int i;
  for (i = 0; i < n_entries; i++) {
    int result = writeEntry(fd, entries+i);
    if (result != 0) {
      return result;
    }
  }

  sceIoClose(fd);

  return 0;
}
