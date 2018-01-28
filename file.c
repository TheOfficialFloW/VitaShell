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
#include "archive.h"
#include "file.h"
#include "utils.h"
#include "sha1.h"
#include "strnatcmp.h"

static char *devices[] = {
  "gro0:",
  "grw0:",
  "imc0:",
  "os0:",
  "pd0:",
  "sa0:",
  "tm0:",
  "ud0:",
  "uma0:",
  "ur0:",
  "ux0:",
  "vd0:",
  "vs0:",
  "host0:",
};

#define N_DEVICES (sizeof(devices) / sizeof(char **))

int allocateReadFile(const char *file, void **buffer) {
  SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  int size = sceIoLseek32(fd, 0, SCE_SEEK_END);
  sceIoLseek32(fd, 0, SCE_SEEK_SET);

  *buffer = malloc(size);
  if (!*buffer) {
    sceIoClose(fd);
    return -1;
  }

  int read = sceIoRead(fd, *buffer, size);
  sceIoClose(fd);

  return read;
}

int ReadFile(const char *file, void *buf, int size) {
  SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  int read = sceIoRead(fd, buf, size);

  sceIoClose(fd);
  return read;
}

int WriteFile(const char *file, const void *buf, int size) {
  SceUID fd = sceIoOpen(file, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fd < 0)
    return fd;

  int written = sceIoWrite(fd, buf, size);

  sceIoClose(fd);
  return written;
}

int getFileSize(const char *file) {
  SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  int fileSize = sceIoLseek(fd, 0, SCE_SEEK_END);

  sceIoClose(fd);
  return fileSize;
}

int checkFileExist(const char *file) {
  SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return 0;

  sceIoClose(fd);
  return 1;
}

int checkFolderExist(const char *folder) {
  SceUID dfd = sceIoDopen(folder);
  if (dfd < 0)
    return 0;

  sceIoDclose(dfd);
  return 1;
}

int getFileSha1(const char *file, uint8_t *pSha1Out, FileProcessParam *param) {
  // Set up SHA1 context
  SHA1_CTX ctx;
  sha1_init(&ctx);

  // Open the file to read, else return the error
  SceUID fd = sceIoOpen(file, SCE_O_RDONLY, 0);
  if (fd < 0)
    return fd;

  // Open up the buffer for copying data into
  void *buf = memalign(4096, TRANSFER_SIZE);

  // Actually take the SHA1 sum
  while (1) {
    int read = sceIoRead(fd, buf, TRANSFER_SIZE);

    if (read < 0) {
      free(buf);
      sceIoClose(fd);
      return read;
    }

    if (read == 0)
      break;

    sha1_update(&ctx, buf, read);

    if (param) {
      // Defined in io_process.c, check to make sure pointer isn't null before incrementing
      if (param->value)
        (*param->value)++; // Note: Max value is filesize/TRANSFER_SIZE

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      // Check to see if param->cancelHandler exists, if so call it and free memory if canceled
      if (param->cancelHandler && param->cancelHandler()) {
        free(buf);
        sceIoClose(fd);
        return 0;
      }

      // This is CPU intensive so the progress bar won't refresh unless we sleep
      // DIALOG_WAIT seemed too long for this application
      // so I set it to 1/2 of a second every 8192 TRANSFER_SIZE blocks
      if ((*param->value) % 8192 == 0)
        sceKernelDelayThread(500000);
    }
  }

  // Final iteration of SHA1 sum, dump final value into pSha1Out buffer
  sha1_final(&ctx, pSha1Out);

  // Free up file buffer
  free(buf);

  // Close file proper
  sceIoClose(fd);
  return 1;
}

int getPathInfo(const char *path, uint64_t *size, uint32_t *folders, uint32_t *files, int (* handler)(const char *path)) {
  SceUID dfd = sceIoDopen(path);
  if (dfd >= 0) {
    int res = 0;

    do {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dfd, &dir);
      if (res > 0) {
        char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
        snprintf(new_path, MAX_PATH_LENGTH - 1, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);

        if (handler && handler(new_path)) {
          free(new_path);
          continue;
        }

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
          int ret = getPathInfo(new_path, size, folders, files, handler);
          if (ret <= 0) {
            free(new_path);
            sceIoDclose(dfd);
            return ret;
          }
        } else {
          if (size)
            (*size) += dir.d_stat.st_size;

          if (files)
            (*files)++;
        }

        free(new_path);
      }
    } while (res > 0);

    sceIoDclose(dfd);

    if (folders)
      (*folders)++;
  } else {
    if (handler && handler(path))
      return 1;

    if (size) {
      SceIoStat stat;
      memset(&stat, 0, sizeof(SceIoStat));

      int res = sceIoGetstat(path, &stat);
      if (res < 0)
        return res;

      (*size) += stat.st_size;
    }

    if (files)
      (*files)++;
  }

  return 1;
}

int removePath(const char *path, FileProcessParam *param) {
  SceUID dfd = sceIoDopen(path);
  if (dfd >= 0) {
    int res = 0;

    do {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dfd, &dir);
      if (res > 0) {
        char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
        snprintf(new_path, MAX_PATH_LENGTH - 1, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
          int ret = removePath(new_path, param);
          if (ret <= 0) {
            free(new_path);
            sceIoDclose(dfd);
            return ret;
          }
        } else {
          int ret = sceIoRemove(new_path);
          if (ret < 0) {
            free(new_path);
            sceIoDclose(dfd);
            return ret;
          }

          if (param) {
            if (param->value)
              (*param->value)++;

            if (param->SetProgress)
              param->SetProgress(param->value ? *param->value : 0, param->max);

            if (param->cancelHandler && param->cancelHandler()) {
              free(new_path);
              sceIoDclose(dfd);
              return 0;
            }
          }
        }

        free(new_path);
      }
    } while (res > 0);

    sceIoDclose(dfd);

    int ret = sceIoRmdir(path);
    if (ret < 0)
      return ret;

    if (param) {
      if (param->value)
        (*param->value)++;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        return 0;
      }
    }
  } else {
    int ret = sceIoRemove(path);
    if (ret < 0)
      return ret;

    if (param) {
      if (param->value)
        (*param->value)++;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        return 0;
      }
    }
  }

  return 1;
}

int copyFile(const char *src_path, const char *dst_path, FileProcessParam *param) {
  // The source and destination paths are identical
  if (strcasecmp(src_path, dst_path) == 0) {
    return -1;
  }

  // The destination is a subfolder of the source folder
  int len = strlen(src_path);
  if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
    return -2;
  }

  SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
  if (fdsrc < 0)
    return fdsrc;

  SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fddst < 0) {
    sceIoClose(fdsrc);
    return fddst;
  }

  void *buf = memalign(4096, TRANSFER_SIZE);

  while (1) {
    int read = sceIoRead(fdsrc, buf, TRANSFER_SIZE);

    if (read < 0) {
      free(buf);

      sceIoClose(fddst);
      sceIoClose(fdsrc);

      sceIoRemove(dst_path);

      return read;
    }

    if (read == 0)
      break;

    int written = sceIoWrite(fddst, buf, read);

    if (written < 0) {
      free(buf);

      sceIoClose(fddst);
      sceIoClose(fdsrc);

      sceIoRemove(dst_path);

      return written;
    }

    if (param) {
      if (param->value)
        (*param->value) += read;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        free(buf);

        sceIoClose(fddst);
        sceIoClose(fdsrc);

        sceIoRemove(dst_path);

        return 0;
      }
    }
  }

  free(buf);

  // Inherit file stat
  SceIoStat stat;
  memset(&stat, 0, sizeof(SceIoStat));
  sceIoGetstatByFd(fdsrc, &stat);
  sceIoChstatByFd(fddst, &stat, 0x3B);

  sceIoClose(fddst);
  sceIoClose(fdsrc);

  return 1;
}

int copyPath(const char *src_path, const char *dst_path, FileProcessParam *param) {
  // The source and destination paths are identical
  if (strcasecmp(src_path, dst_path) == 0) {
    return -1;
  }

  // The destination is a subfolder of the source folder
  int len = strlen(src_path);
  if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
    return -2;
  }

  SceUID dfd = sceIoDopen(src_path);
  if (dfd >= 0) {
    SceIoStat stat;
    memset(&stat, 0, sizeof(SceIoStat));
    sceIoGetstatByFd(dfd, &stat);

    stat.st_mode |= SCE_S_IWUSR;
    
    int ret = sceIoMkdir(dst_path, stat.st_mode & 0xFFF);
    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
      sceIoDclose(dfd);
      return ret;
    }
    
    if (ret == SCE_ERROR_ERRNO_EEXIST) {
      sceIoChstat(dst_path, &stat, 0x3B);
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

        char *new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
        snprintf(new_dst_path, MAX_PATH_LENGTH - 1, "%s%s%s", dst_path, hasEndSlash(dst_path) ? "" : "/", dir.d_name);

        int ret = 0;

        if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
          ret = copyPath(new_src_path, new_dst_path, param);
        } else {
          ret = copyFile(new_src_path, new_dst_path, param);
        }

        free(new_dst_path);
        free(new_src_path);

        if (ret <= 0) {
          sceIoDclose(dfd);
          return ret;
        }
      }
    } while (res > 0);

    sceIoDclose(dfd);
  } else {
    return copyFile(src_path, dst_path, param);
  }

  return 1;
}

int movePath(const char *src_path, const char *dst_path, int flags, FileProcessParam *param) {
  // The source and destination paths are identical
  if (strcasecmp(src_path, dst_path) == 0) {
    return -1;
  }

  // The destination is a subfolder of the source folder
  int len = strlen(src_path);
  if (strncasecmp(src_path, dst_path, len) == 0 && (dst_path[len] == '/' || dst_path[len - 1] == '/')) {
    return -2;
  }

  int res = sceIoRename(src_path, dst_path);

  if (res == SCE_ERROR_ERRNO_EEXIST && flags & (MOVE_INTEGRATE | MOVE_REPLACE)) {
    // Src stat
    SceIoStat src_stat;
    memset(&src_stat, 0, sizeof(SceIoStat));
    res = sceIoGetstat(src_path, &src_stat);
    if (res < 0)
      return res;

    // Dst stat
    SceIoStat dst_stat;
    memset(&dst_stat, 0, sizeof(SceIoStat));
    res = sceIoGetstat(dst_path, &dst_stat);
    if (res < 0)
      return res;

    // Is dir
    int src_is_dir = SCE_S_ISDIR(src_stat.st_mode);
    int dst_is_dir = SCE_S_ISDIR(dst_stat.st_mode);

    // One of them is a file and the other a directory, no replacement or integration possible
    if (src_is_dir != dst_is_dir)
      return -3;

    // Replace file
    if (!src_is_dir && !dst_is_dir && flags & MOVE_REPLACE) {
      sceIoRemove(dst_path);

      res = sceIoRename(src_path, dst_path);
      if (res < 0)
        return res;

      return 1;
    }

    // Integrate directory
    if (src_is_dir && dst_is_dir && flags & MOVE_INTEGRATE) {
      SceUID dfd = sceIoDopen(src_path);
      if (dfd < 0)
        return dfd;

      int res = 0;

      do {
        SceIoDirent dir;
        memset(&dir, 0, sizeof(SceIoDirent));

        res = sceIoDread(dfd, &dir);
        if (res > 0) {
          char *new_src_path = malloc(strlen(src_path) + strlen(dir.d_name) + 2);
          snprintf(new_src_path, MAX_PATH_LENGTH - 1, "%s%s%s", src_path, hasEndSlash(src_path) ? "" : "/", dir.d_name);

          char *new_dst_path = malloc(strlen(dst_path) + strlen(dir.d_name) + 2);
          snprintf(new_dst_path, MAX_PATH_LENGTH - 1, "%s%s%s", dst_path, hasEndSlash(dst_path) ? "" : "/", dir.d_name);

          // Recursive move
          int ret = movePath(new_src_path, new_dst_path, flags, param);

          free(new_dst_path);
          free(new_src_path);

          if (ret <= 0) {
            sceIoDclose(dfd);
            return ret;
          }
        }
      } while (res > 0);

      sceIoDclose(dfd);

      // Integrated, now remove this directory
      sceIoRmdir(src_path);
    }
  }

  return 1;
}

typedef struct {
  char *extension;
  int type;
} ExtensionType;

static ExtensionType extension_types[] = {
  { ".7Z",       FILE_TYPE_ARCHIVE },
  { ".AR",       FILE_TYPE_ARCHIVE },
  { ".BMP",      FILE_TYPE_BMP },
  { ".BZ2",      FILE_TYPE_ARCHIVE },
  { ".CPIO",     FILE_TYPE_ARCHIVE },
  { ".GRZ",      FILE_TYPE_ARCHIVE },
  { ".GZ",       FILE_TYPE_ARCHIVE },
  { ".INI",      FILE_TYPE_INI },
  { ".ISO",      FILE_TYPE_ARCHIVE },
  { ".JPEG",     FILE_TYPE_JPEG },
  { ".JPG",      FILE_TYPE_JPEG },
  { ".LRZ",      FILE_TYPE_ARCHIVE },
  { ".LZ",       FILE_TYPE_ARCHIVE },
  { ".LZ4",      FILE_TYPE_ARCHIVE },
  { ".LZMA",     FILE_TYPE_ARCHIVE },
  { ".LZO",      FILE_TYPE_ARCHIVE },
  { ".MP3",      FILE_TYPE_MP3 },
  { ".MP4",      FILE_TYPE_MP4 },
  { ".MTREE",    FILE_TYPE_ARCHIVE },
  { ".OGG",      FILE_TYPE_OGG },
  { ".PNG",      FILE_TYPE_PNG },
  { ".PSARC",    FILE_TYPE_ARCHIVE },
  { ".PSP2ARC",  FILE_TYPE_ARCHIVE },
  { ".PSP2DMP",  FILE_TYPE_PSP2DMP },
  { ".RAR",      FILE_TYPE_ARCHIVE },
  { ".SFO",      FILE_TYPE_SFO },
  { ".SHAR",     FILE_TYPE_ARCHIVE },
  { ".TAR",      FILE_TYPE_ARCHIVE },
  { ".TAZ",      FILE_TYPE_ARCHIVE },
  { ".TBZ",      FILE_TYPE_ARCHIVE },
  { ".TBZ2",     FILE_TYPE_ARCHIVE },
  { ".TGZ",      FILE_TYPE_ARCHIVE },
  { ".TLZ",      FILE_TYPE_ARCHIVE },
  { ".TMP",      FILE_TYPE_PSP2DMP },
  { ".TXT",      FILE_TYPE_TXT },
  { ".TXZ",      FILE_TYPE_ARCHIVE },
  { ".TZ",       FILE_TYPE_ARCHIVE },
  { ".TZ2",      FILE_TYPE_ARCHIVE },
  { ".TZMA",     FILE_TYPE_ARCHIVE },
  { ".TZO",      FILE_TYPE_ARCHIVE },
  { ".TZST",     FILE_TYPE_ARCHIVE },
  { ".UU",       FILE_TYPE_ARCHIVE },
  { ".VPK",      FILE_TYPE_VPK },
  { ".WAR",      FILE_TYPE_ARCHIVE },
  { ".XAR",      FILE_TYPE_ARCHIVE },
  { ".XML",      FILE_TYPE_XML },
  { ".XZ",       FILE_TYPE_ARCHIVE },
  { ".Z",        FILE_TYPE_ARCHIVE },
  { ".ZIP",      FILE_TYPE_ARCHIVE },
  { ".ZST",      FILE_TYPE_ARCHIVE },
};

int getFileType(const char *file) {
  char *p = strrchr(file, '.');
  if (p) {
    int i;
    for (i = 0; i < (sizeof(extension_types) / sizeof(ExtensionType)); i++) {
      if (strcasecmp(p, extension_types[i].extension) == 0) {
        return extension_types[i].type;
      }
    }
  }

  return FILE_TYPE_UNKNOWN;
}

int getNumberOfDevices() {
  return N_DEVICES;
}

char **getDevices() {
  return devices;
}

FileListEntry *fileListCopyEntry(FileListEntry *src) {
  FileListEntry *dst = malloc(sizeof(FileListEntry));
  if (!dst)
    return NULL;

  memcpy(dst, src, sizeof(FileListEntry));
  dst->name = malloc(src->name_length + 1);
  strcpy(dst->name, src->name);
  return dst;
}

FileListEntry *fileListFindEntry(FileList *list, const char *name) {
  if (!list)
    return NULL;

  FileListEntry *entry = list->head;

  int name_length = strlen(name);

  while (entry) {
    if (entry->name_length == name_length && strcasecmp(entry->name, name) == 0)
      return entry;

    entry = entry->next;
  }

  return NULL;
}

FileListEntry *fileListGetNthEntry(FileList *list, int n) {
  if (!list)
    return NULL;

  FileListEntry *entry = list->head;

  while (n > 0 && entry) {
    n--;
    entry = entry->next;
  }

  if (n != 0)
    return NULL;

  return entry;
}

int fileListGetNumberByName(FileList *list, const char *name) {
  if (!list)
    return -1;

  FileListEntry *entry = list->head;

  int name_length = strlen(name);

  int n = 0;

  while (entry) {
    if (entry->name_length == name_length && strcasecmp(entry->name, name) == 0)
      return n;

    n++;
    entry = entry->next;
  }

  return -1;
}

void fileListAddEntry(FileList *list, FileListEntry *entry, int sort) {
  if (!list || !entry)
    return;

  entry->next = NULL;
  entry->previous = NULL;

  if (list->head == NULL) {
    list->head = entry;
    list->tail = entry;
  } else {
    if (sort != SORT_NONE) {
      FileListEntry *p = list->head;
      FileListEntry *previous = NULL;

      char entry_name[MAX_NAME_LENGTH];
      strcpy(entry_name, entry->name);
      removeEndSlash(entry_name);
      
      while (p) {
        char p_name[MAX_NAME_LENGTH];
        strcpy(p_name, p->name);
        removeEndSlash(p_name);
        
        // '..' is always at first
        if (strcmp(entry_name, "..") == 0)
          break;

        if (strcmp(p_name, "..") == 0) {
          previous = p;
          p = p->next;
          continue;
        }

        // Sort by type
        if (sort == SORT_BY_NAME) {
          // First folders then files
          if (entry->is_folder > p->is_folder)
            break;
        } else if (sort == SORT_BY_SIZE || sort == SORT_BY_DATE) {
          // First files then folders
          if (entry->is_folder < p->is_folder)
            break;
        }

        if (sort == SORT_BY_NAME) {
          // Sort by name within the same type
          if (entry->is_folder == p->is_folder) {
            if (strnatcasecmp(entry_name, p_name) < 0) {
              break;
            }
          }
        } else if (sort == SORT_BY_SIZE) {
          // Sort by name for folders
          if (entry->is_folder && p->is_folder) {
            if (strnatcasecmp(entry_name, p_name) < 0) {
              break;
            }
          } else if (!entry->is_folder && !p->is_folder) {
            // Sort by size for files
            if (entry->size > p->size)
              break;

            // Sort by name for files with the same size
            if (entry->size == p->size) {
              if (strnatcasecmp(entry_name, p_name) < 0) {
                break;
              }
            }
          }
        } else if (sort == SORT_BY_DATE) {
          if (entry->is_folder == p->is_folder) {
            SceRtcTick entry_tick, p_tick;
            sceRtcGetTick(&entry->mtime, &entry_tick);
            sceRtcGetTick(&p->mtime, &p_tick);

            // Sort by date within the same type
            if (entry_tick.tick > p_tick.tick)
              break;

            // Sort by name for files and folders with the same date
            if (entry_tick.tick == p_tick.tick) {
              if (strnatcasecmp(entry_name, p_name) < 0) {
                break;
              }
            }
          }
        }

        previous = p;
        p = p->next;
      }

      if (previous == NULL) { // Order: entry (new head) -> p (old head)
        entry->next = p;
        p->previous = entry;
        list->head = entry;
      } else if (previous->next == NULL) { // Order: p (old tail) -> entry (new tail)
        FileListEntry *tail = list->tail;
        tail->next = entry;
        entry->previous = tail;
        list->tail = entry;
      } else { // Order: previous -> entry -> p
        previous->next = entry;
        entry->previous = previous; 
        entry->next = p;
        p->previous = entry;
      }
    } else {
      FileListEntry *tail = list->tail;
      tail->next = entry;
      entry->previous = tail;
      list->tail = entry;
    }
  }

  list->length++;
}

int fileListRemoveEntry(FileList *list, FileListEntry *entry) {
  if (!list || !entry)
    return 0;

  if (entry->previous) {
    entry->previous->next = entry->next;
  } else {
    list->head = entry->next;
  }

  if (entry->next) {
    entry->next->previous = entry->previous;
  } else {
    list->tail = entry->previous;
  }

  list->length--;
  free(entry->name);
  free(entry);

  if (list->length == 0) {
    list->head = NULL;
    list->tail = NULL;
  }

  return 1;
}

int fileListRemoveEntryByName(FileList *list, const char *name) {
  if (!list)
    return 0;

  FileListEntry *entry = list->head;
  FileListEntry *previous = NULL;

  int name_length = strlen(name);

  while (entry) {
    if (entry->name_length == name_length && strcasecmp(entry->name, name) == 0) {
      if (previous) {
        previous->next = entry->next;
      } else {
        list->head = entry->next;
      }

      if (list->tail == entry) {
        list->tail = previous;
      }

      list->length--;
      free(entry->name);
      free(entry);

      if (list->length == 0) {
        list->head = NULL;
        list->tail = NULL;
      }

      return 1;
    }

    previous = entry;
    entry = entry->next;
  }

  return 0;
}

void fileListEmpty(FileList *list) {
  if (!list)
    return;

  FileListEntry *entry = list->head;

  while (entry) {
    FileListEntry *next = entry->next;
    free(entry->name);
    free(entry);
    entry = next;
  }

  list->head = NULL;
  list->tail = NULL;
  list->length = 0;
  list->files = 0;
  list->folders = 0;
}

int fileListGetDeviceEntries(FileList *list) {
  if (!list)
    return -1;

  int i;
  for (i = 0; i < N_DEVICES; i++) {
    if (devices[i]) {
      if (is_safe_mode && strcmp(devices[i], "ux0:") != 0)
        continue;

      SceIoStat stat;
      memset(&stat, 0, sizeof(SceIoStat));
      if (sceIoGetstat(devices[i], &stat) >= 0) {
        FileListEntry *entry = malloc(sizeof(FileListEntry));
        if (entry) {
          entry->name_length = strlen(devices[i]);
          entry->name = malloc(entry->name_length + 1);
          strcpy(entry->name, devices[i]);
          entry->is_folder = 1;
          entry->type = FILE_TYPE_UNKNOWN;

          SceIoDevInfo info;
          memset(&info, 0, sizeof(SceIoDevInfo));
          int res = sceIoDevctl(entry->name, 0x3001, NULL, 0, &info, sizeof(SceIoDevInfo));
          if (res >= 0) {
            entry->size = info.free_size;
            entry->size2 = info.max_size;
          } else {
            if (strcmp(devices[i], "ux0:") == 0) {
              sceAppMgrGetDevInfo("ux0:", (uint64_t *)&entry->size2, (uint64_t *)&entry->size);
            } else {
              entry->size = 0;
              entry->size2 = 0;
            }
          }

          memcpy(&entry->ctime, (SceDateTime *)&stat.st_ctime, sizeof(SceDateTime));
          memcpy(&entry->mtime, (SceDateTime *)&stat.st_mtime, sizeof(SceDateTime));
          memcpy(&entry->atime, (SceDateTime *)&stat.st_atime, sizeof(SceDateTime));

          fileListAddEntry(list, entry, SORT_BY_NAME);

          list->folders++;
        }
      }
    }
  }

  return 0;
}

int fileListGetDirectoryEntries(FileList *list, const char *path, int sort) {
  if (!list)
    return -1;

  SceUID dfd = sceIoDopen(path);
  if (dfd < 0)
    return dfd;

  FileListEntry *entry = malloc(sizeof(FileListEntry));
  if (entry) {
    entry->name_length = strlen(DIR_UP);
    entry->name = malloc(entry->name_length + 1);
    strcpy(entry->name, DIR_UP);
    entry->is_folder = 1;
    entry->type = FILE_TYPE_UNKNOWN;
    fileListAddEntry(list, entry, sort);
  }

  int res = 0;

  do {
    SceIoDirent dir;
    memset(&dir, 0, sizeof(SceIoDirent));

    res = sceIoDread(dfd, &dir);
    if (res > 0) {
      FileListEntry *entry = malloc(sizeof(FileListEntry));
      if (entry) {
        entry->is_folder = SCE_S_ISDIR(dir.d_stat.st_mode);
        if (entry->is_folder) {
          entry->name_length = strlen(dir.d_name) + 1;
          entry->name = malloc(entry->name_length + 1);
          strcpy(entry->name, dir.d_name);
          addEndSlash(entry->name);
          entry->type = FILE_TYPE_UNKNOWN;
          list->folders++;
        } else {
          entry->name_length = strlen(dir.d_name);
          entry->name = malloc(entry->name_length + 1);
          strcpy(entry->name, dir.d_name);
          entry->type = getFileType(entry->name);
          list->files++;
        }

        entry->size = dir.d_stat.st_size;

        memcpy(&entry->ctime, (SceDateTime *)&dir.d_stat.st_ctime, sizeof(SceDateTime));
        memcpy(&entry->mtime, (SceDateTime *)&dir.d_stat.st_mtime, sizeof(SceDateTime));
        memcpy(&entry->atime, (SceDateTime *)&dir.d_stat.st_atime, sizeof(SceDateTime));

        fileListAddEntry(list, entry, sort);
      }
    }
  } while (res > 0);

  sceIoDclose(dfd);

  return 0;
}

int fileListGetEntries(FileList *list, const char *path, int sort) {
  if (!list)
    return -1;

  if (isInArchive()) {
    return fileListGetArchiveEntries(list, path, sort);
  }

  if (strcasecmp(path, HOME_PATH) == 0) {
    return fileListGetDeviceEntries(list);
  }

  return fileListGetDirectoryEntries(list, path, sort);
}
