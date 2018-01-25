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
#include "psarc.h"
#include "file.h"
#include "utils.h"

#define SCE_FIOS_FH_SIZE 80
#define SCE_FIOS_DH_SIZE 80
#define SCE_FIOS_OP_SIZE 168
#define SCE_FIOS_CHUNK_SIZE 64

#define SCE_FIOS_ALIGN_UP(val, align) (((val) + ((align) - 1)) & ~((align) - 1))
#define SCE_FIOS_STORAGE_SIZE(num, size) (((num) * (size)) + SCE_FIOS_ALIGN_UP(SCE_FIOS_ALIGN_UP((num), 8) / 8, 8))

#define SCE_FIOS_DH_STORAGE_SIZE(numDHs, pathMax) SCE_FIOS_STORAGE_SIZE(numDHs, SCE_FIOS_DH_SIZE + pathMax)
#define SCE_FIOS_FH_STORAGE_SIZE(numFHs, pathMax) SCE_FIOS_STORAGE_SIZE(numFHs, SCE_FIOS_FH_SIZE + pathMax)
#define SCE_FIOS_OP_STORAGE_SIZE(numOps, pathMax) SCE_FIOS_STORAGE_SIZE(numOps, SCE_FIOS_OP_SIZE + pathMax)
#define SCE_FIOS_CHUNK_STORAGE_SIZE(numChunks) SCE_FIOS_STORAGE_SIZE(numChunks, SCE_FIOS_CHUNK_SIZE)

#define SCE_FIOS_BUFFER_INITIALIZER  { 0, 0 }
#define SCE_FIOS_PSARC_DEARCHIVER_CONTEXT_INITIALIZER { sizeof(SceFiosPsarcDearchiverContext), 0, 0, 0, {0, 0, 0} }
#define SCE_FIOS_PARAMS_INITIALIZER { 0, sizeof(SceFiosParams), 0, 0, 2, 1, 0, 0, 256 * 1024, 2, 0, 0, 0, 0, 0, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, SCE_FIOS_BUFFER_INITIALIZER, NULL, NULL, NULL, { 66, 189, 66 }, { 0x40000, 0, 0x40000}, { 8 * 1024, 16 * 1024, 8 * 1024}}

typedef int32_t SceFiosFH;
typedef int32_t SceFiosDH;
typedef uint64_t SceFiosDate;
typedef int64_t SceFiosOffset;
typedef int64_t SceFiosSize;

typedef struct SceFiosPsarcDearchiverContext {
  size_t sizeOfContext;
  size_t workBufferSize;
  void *pWorkBuffer;
  intptr_t flags;
  intptr_t reserved[3];
} SceFiosPsarcDearchiverContext;

typedef struct SceFiosBuffer {
  void *pPtr;
  size_t length;
} SceFiosBuffer;

typedef struct SceFiosParams {
  uint32_t initialized : 1;
  uint32_t paramsSize : 15;
  uint32_t pathMax : 16;
  uint32_t profiling;
  uint32_t ioThreadCount;
  uint32_t threadsPerScheduler;
  uint32_t extraFlag1 : 1;
  uint32_t extraFlags : 31;
  uint32_t maxChunk;
  uint8_t maxDecompressorThreadCount;
  uint8_t reserved1;   
  uint8_t reserved2;
  uint8_t reserved3;
  intptr_t reserved4;
  intptr_t reserved5;
  SceFiosBuffer opStorage;
  SceFiosBuffer fhStorage;
  SceFiosBuffer dhStorage;
  SceFiosBuffer chunkStorage;
  void *pVprintf;
  void *pMemcpy;
  void *pProfileCallback;
  int threadPriority[3];
  int threadAffinity[3];
  int threadStackSize[3];
} SceFiosParams;

typedef struct SceFiosDirEntry {
  SceFiosOffset fileSize;
  uint32_t statFlags;
  uint16_t nameLength;
  uint16_t fullPathLength;
  uint16_t offsetToName;
  uint16_t reserved[3];
  char fullPath[1024];
} SceFiosDirEntry;

typedef struct SceFiosStat {
	SceFiosOffset fileSize;
	SceFiosDate accessDate;
	SceFiosDate modificationDate;
	SceFiosDate creationDate;
	uint32_t statFlags;
	uint32_t reserved;
	int64_t uid;
	int64_t gid;
	int64_t dev;
	int64_t ino;
	int64_t mode;
} SceFiosStat;

int sceFiosInitialize(const SceFiosParams *params);
void sceFiosTerminate();

SceFiosSize sceFiosArchiveGetMountBufferSizeSync(const void *attr, const char *path, void *params);
int sceFiosArchiveMountSync(const void *attr, SceFiosFH *fh, const char *path, const char *mount_point, SceFiosBuffer mount_buffer, void *params);
int sceFiosArchiveUnmountSync(const void *attr, SceFiosFH fh);

int sceFiosStatSync(const void *attr, const char *path, SceFiosStat *stat);

int sceFiosFHOpenSync(const void *attr, SceFiosFH *fh, const char *path, const void *params);
SceFiosSize sceFiosFHReadSync(const void *attr, SceFiosFH fh, void *data, SceFiosSize size);
int sceFiosFHCloseSync(const void *attr, SceFiosFH fh);

int sceFiosDHOpenSync(const void *attr, SceFiosDH *dh, const char *path, SceFiosBuffer buf);
int sceFiosDHReadSync(const void *attr, SceFiosDH dh, SceFiosDirEntry *dir);
int sceFiosDHCloseSync(const void *attr, SceFiosDH dh);

SceDateTime *sceFiosDateToSceDateTime(SceFiosDate date, SceDateTime *sce_date);

int sceFiosIOFilterAdd(int index, void (* callback)(), void *context);
int sceFiosIOFilterRemove(int index);
 
void sceFiosIOFilterPsarcDearchiver();

int64_t g_OpStorage[SCE_FIOS_OP_STORAGE_SIZE(64, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
int64_t g_ChunkStorage[SCE_FIOS_CHUNK_STORAGE_SIZE(1024) / sizeof(int64_t) + 1];
int64_t g_FHStorage[SCE_FIOS_FH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];
int64_t g_DHStorage[SCE_FIOS_DH_STORAGE_SIZE(32, MAX_PATH_LENGTH) / sizeof(int64_t) + 1];

SceFiosPsarcDearchiverContext g_DearchiverContext = SCE_FIOS_PSARC_DEARCHIVER_CONTEXT_INITIALIZER;
char g_DearchiverWorkBuffer[0x30000] __attribute__((aligned(64)));
int g_ArchiveIndex = 0;
SceFiosBuffer g_MountBuffer = SCE_FIOS_BUFFER_INITIALIZER;
SceFiosFH g_ArchiveFH = -1;

int psarcOpen(const char *file) {
  int res;
  
  SceFiosParams params = SCE_FIOS_PARAMS_INITIALIZER;
  params.opStorage.pPtr = g_OpStorage;
  params.opStorage.length = sizeof(g_OpStorage);
  params.chunkStorage.pPtr = g_ChunkStorage;
  params.chunkStorage.length = sizeof(g_ChunkStorage);
  params.fhStorage.pPtr = g_FHStorage;
  params.fhStorage.length = sizeof(g_FHStorage);
  params.dhStorage.pPtr = g_DHStorage;
  params.dhStorage.length = sizeof(g_DHStorage);  
  params.pathMax = MAX_PATH_LENGTH;
  
  res = sceFiosInitialize(&params);
  if (res < 0)
    goto ERROR_INITIALIZE;
  
  g_DearchiverContext.workBufferSize = sizeof(g_DearchiverWorkBuffer);
  g_DearchiverContext.pWorkBuffer = g_DearchiverWorkBuffer;

  res = sceFiosIOFilterAdd(g_ArchiveIndex, sceFiosIOFilterPsarcDearchiver, &g_DearchiverContext);
  if (res < 0)
    goto ERROR_FILTER_ADD;
  
  SceFiosSize result = sceFiosArchiveGetMountBufferSizeSync(NULL, file, NULL);
  if (result < 0) {
    res = (int)result;
    goto ERROR_ARCHIVE_GET_MOUNT_BUFFER_SIZE;
  }

  g_MountBuffer.length = (size_t)result;
  g_MountBuffer.pPtr = malloc(g_MountBuffer.length);

  res = sceFiosArchiveMountSync(NULL, &g_ArchiveFH, file, file, g_MountBuffer, NULL);
  if (res < 0)
    goto ERROR_ARCHIVE_MOUNT;

  return res;

ERROR_ARCHIVE_MOUNT:
  free(g_MountBuffer.pPtr);
  
ERROR_ARCHIVE_GET_MOUNT_BUFFER_SIZE:
  sceFiosIOFilterRemove(g_ArchiveIndex);
  
ERROR_FILTER_ADD:
  sceFiosTerminate();
  
ERROR_INITIALIZE:
  return res;
}

int psarcClose() {
  sceFiosArchiveUnmountSync(NULL, g_ArchiveFH);
  free(g_MountBuffer.pPtr);
  sceFiosIOFilterRemove(g_ArchiveIndex);
  sceFiosTerminate();

  return 0;
}

int fileListGetPsarcEntries(FileList *list, const char *path, int sort) {
  int res;
  
  if (!list)
    return -1;

  SceFiosDH dh = -1;
  SceFiosBuffer buf = SCE_FIOS_BUFFER_INITIALIZER;
  res = sceFiosDHOpenSync(NULL, &dh, path, buf);
  if (res < 0)
    return res;

  FileListEntry *entry = malloc(sizeof(FileListEntry));
  if (entry) {
    entry->name_length = strlen(DIR_UP);
    entry->name = malloc(entry->name_length + 1);
    strcpy(entry->name, DIR_UP);
    entry->is_folder = 1;
    entry->type = FILE_TYPE_UNKNOWN;
    fileListAddEntry(list, entry, sort);
  }

  do {
    SceFiosDirEntry dir;
    memset(&dir, 0, sizeof(SceFiosDirEntry));

    res = sceFiosDHReadSync(NULL, dh, &dir);
    
    if (res >= 0) {
      char *name = dir.fullPath + dir.offsetToName;

      SceFiosStat stat;
      memset(&stat, 0, sizeof(SceFiosStat));
      
      if (sceFiosStatSync(NULL, dir.fullPath, &stat) >= 0) {
        FileListEntry *entry = malloc(sizeof(FileListEntry));
        if (entry) {
          entry->is_folder = stat.statFlags & 0x1;
          if (entry->is_folder) {
            entry->name_length = strlen(name) + 1;
            entry->name = malloc(entry->name_length + 1);
            strcpy(entry->name, name);
            addEndSlash(entry->name);
            entry->type = FILE_TYPE_UNKNOWN;
            list->folders++;
          } else {
            entry->name_length = strlen(name);
            entry->name = malloc(entry->name_length + 1);
            strcpy(entry->name, name);
            entry->type = getFileType(entry->name);
            list->files++;
          }

          entry->size = stat.fileSize;
          
          SceDateTime time;
          
          sceFiosDateToSceDateTime(stat.accessDate, &time);
          memcpy(&entry->ctime, (SceDateTime *)&time, sizeof(SceDateTime));
          
          sceFiosDateToSceDateTime(stat.modificationDate, &time);
          memcpy(&entry->mtime, (SceDateTime *)&time, sizeof(SceDateTime));
          
          sceFiosDateToSceDateTime(stat.creationDate, &time);
          memcpy(&entry->atime, (SceDateTime *)&time, sizeof(SceDateTime));
          
          fileListAddEntry(list, entry, sort);
        }
      }
    }
  } while (res >= 0);

  sceFiosDHCloseSync(NULL, dh);

  return 0;
}

int getPsarcPathInfo(const char *path, uint64_t *size, uint32_t *folders, uint32_t *files, int (* handler)(const char *path)) {
  SceFiosDH dh = -1;
  SceFiosBuffer buf = SCE_FIOS_BUFFER_INITIALIZER;
  if (sceFiosDHOpenSync(NULL, &dh, path, buf) >= 0) {
    int res = 0;

    do {
      SceFiosDirEntry dir;
      memset(&dir, 0, sizeof(SceFiosDirEntry));

      res = sceFiosDHReadSync(NULL, dh, &dir);
      if (res >= 0) {
        char *name = dir.fullPath + dir.offsetToName;
        
        char *new_path = malloc(strlen(path) + strlen(name) + 2);
        snprintf(new_path, MAX_PATH_LENGTH - 1, "%s%s%s", path, hasEndSlash(path) ? "" : "/", name);

        if (handler && handler(new_path)) {
          free(new_path);
          continue;
        }

        if (dir.statFlags & 0x1) {
          int ret = getPsarcPathInfo(new_path, size, folders, files, handler);
          if (ret <= 0) {
            free(new_path);
            sceFiosDHCloseSync(NULL, dh);
            return ret;
          }
        } else {
          if (size)
            (*size) += dir.fileSize;

          if (files)
            (*files)++;
        }

        free(new_path);
      }
    } while (res >= 0);

    sceFiosDHCloseSync(NULL, dh);

    if (folders)
      (*folders)++;
  } else {
    if (handler && handler(path))
      return 1;

    if (size) {
      SceFiosStat stat;
      memset(&stat, 0, sizeof(SceFiosStat));
      
      int res = sceFiosStatSync(NULL, path, &stat);
      if (res < 0)
        return res;
      
      (*size) += stat.fileSize;
    }

    if (files)
      (*files)++;
  }

  return 1;
}

int extractPsarcFile(const char *src_path, const char *dst_path, FileProcessParam *param) {
  SceUID fdsrc = psarcFileOpen(src_path, SCE_O_RDONLY, 0);
  if (fdsrc < 0)
    return fdsrc;

  SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
  if (fddst < 0) {
    psarcFileClose(fdsrc);
    return fddst;
  }

  void *buf = memalign(4096, TRANSFER_SIZE);

  while (1) {
    int read = psarcFileRead(fdsrc, buf, TRANSFER_SIZE);

    if (read < 0) {
      free(buf);

      sceIoClose(fddst);
      psarcFileClose(fdsrc);

      sceIoRemove(dst_path);

      return read;
    }

    if (read == 0)
      break;

    int written = sceIoWrite(fddst, buf, read);

    if (written < 0) {
      free(buf);

      sceIoClose(fddst);
      psarcFileClose(fdsrc);

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
        psarcFileClose(fdsrc);

        sceIoRemove(dst_path);

        return 0;
      }
    }
  }

  free(buf);

  sceIoClose(fddst);
  psarcFileClose(fdsrc);

  return 1;
}

int extractPsarcPath(const char *src_path, const char *dst_path, FileProcessParam *param) {
  SceFiosDH dh = -1;
  SceFiosBuffer buf = SCE_FIOS_BUFFER_INITIALIZER;
  if (sceFiosDHOpenSync(NULL, &dh, src_path, buf) >= 0) {
    int ret = sceIoMkdir(dst_path, 0777);
    if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
      sceFiosDHCloseSync(NULL, dh);
      return ret;
    }
    
    if (param) {
      if (param->value)
        (*param->value) += DIRECTORY_SIZE;

      if (param->SetProgress)
        param->SetProgress(param->value ? *param->value : 0, param->max);

      if (param->cancelHandler && param->cancelHandler()) {
        sceFiosDHCloseSync(NULL, dh);
        return 0;
      }
    }

    int res = 0;

    do {
      SceFiosDirEntry dir;
      memset(&dir, 0, sizeof(SceFiosDirEntry));

      res = sceFiosDHReadSync(NULL, dh, &dir);
      if (res >= 0) {
        char *name = dir.fullPath + dir.offsetToName;

        char *new_src_path = malloc(strlen(src_path) + strlen(name) + 2);
        snprintf(new_src_path, MAX_PATH_LENGTH - 1, "%s%s%s", src_path, hasEndSlash(src_path) ? "" : "/", name);

        char *new_dst_path = malloc(strlen(dst_path) + strlen(name) + 2);
        snprintf(new_dst_path, MAX_PATH_LENGTH - 1, "%s%s%s", dst_path, hasEndSlash(dst_path) ? "" : "/", name);

        int ret = 0;

        if (dir.statFlags & 0x1) {
          ret = extractPsarcPath(new_src_path, new_dst_path, param);
        } else {
          ret = extractPsarcFile(new_src_path, new_dst_path, param);
        }

        free(new_dst_path);
        free(new_src_path);

        if (ret <= 0) {
          sceFiosDHCloseSync(NULL, dh);
          return ret;
        }
      }
    } while (res >= 0);

    sceFiosDHCloseSync(NULL, dh);
  } else {
    return extractPsarcFile(src_path, dst_path, param);
  }

  return 1;
}

int psarcFileGetstat(const char *file, SceIoStat *stat) {
  SceFiosStat fios_stat;
  memset(&fios_stat, 0, sizeof(SceFiosStat));
  int res = sceFiosStatSync(NULL, file, &fios_stat);
  if (res < 0)
    return res;
  
  if (stat) {
    stat->st_mode = (fios_stat.statFlags & 0x1) ? SCE_S_IFDIR : SCE_S_IFREG;
    stat->st_size = fios_stat.fileSize;
    
    SceDateTime time;
    
    sceFiosDateToSceDateTime(fios_stat.accessDate, &time);
    memcpy(&stat->st_ctime, (SceDateTime *)&time, sizeof(SceDateTime));
    
    sceFiosDateToSceDateTime(fios_stat.modificationDate, &time);
    memcpy(&stat->st_mtime, (SceDateTime *)&time, sizeof(SceDateTime));
    
    sceFiosDateToSceDateTime(fios_stat.creationDate, &time);
    memcpy(&stat->st_atime, (SceDateTime *)&time, sizeof(SceDateTime));
  }
  
  return res;
}

int psarcFileOpen(const char *file, int flags, SceMode mode) {
  SceFiosFH fh = -1;
  
  int res = sceFiosFHOpenSync(NULL, &fh, file, NULL);
  if (res < 0)
    return res;
  
  return fh;
}

int psarcFileRead(SceUID fd, void *data, SceSize size) {
  return (int)sceFiosFHReadSync(NULL, fd, data, (SceFiosSize)size);
}

int psarcFileClose(SceUID fd) {
  return sceFiosFHCloseSync(NULL, fd);
}