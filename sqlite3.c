/*
  PS Vita override for R/W SQLite functionality
  Copyright (C) 2017 VitaSmith
  Based on original work (C) 2015 xyzz

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

/*
  Note: We must override part of the default SQlite VFS for full SQLite
  DB access because the native Vita one, called "psp2", only allows read
  operations (which Sony probably did to avoid exploit propagation from
  potential SQLite vulnerabilities).
  Short of using the VFS overrides below, any attempt to create a new DB
  or write to an existing one will result in SQLITE_CANTOPEN...
*/

#include <stdlib.h>
#include <string.h>
#include <psp2/io/fcntl.h>
#include <psp2/sqlite.h>

#include "sqlite3.h"

//#define VERBOSE 1
#if VERBOSE
extern int psvDebugScreenPrintf(const char *format, ...);
#define LOG psvDebugScreenPrintf
#else
#define LOG(...)
#endif

#define IS_ERROR(x) ((unsigned)x & 0x80000000)

static sqlite3_io_methods* rw_methods = NULL;
static sqlite3_vfs *rw_vfs = NULL;

// The file structure used by Sony
typedef struct {
  sqlite3_file file;
  int* fd;
} vfs_file;

static int vita_xWrite(sqlite3_file *file, const void *buf, int count, sqlite_int64 offset)
{
  vfs_file *p = (vfs_file*)file;
  int seek = sceIoLseek(*p->fd, offset, SCE_SEEK_SET);
  LOG("seek %08x %x => %x\n", *p->fd, offset, seek);
  if (seek != offset)
    return SQLITE_IOERR_WRITE;
  int write = sceIoWrite(*p->fd, buf, count);
  LOG("write %08x %08x %x => %x\n", *p->fd, buf, count, write);
  if (write != count) {
    LOG("write error %08x\n", write);
    return SQLITE_IOERR_WRITE;
  }
  return SQLITE_OK;
}

static int vita_xSync(sqlite3_file *file, int flags)
{
  vfs_file *p = (vfs_file*)file;
  int r = sceIoSyncByFd(*p->fd, 0);
  LOG("xSync %x, %x => %x\n", *p->fd, flags, r);
  if (IS_ERROR(r))
    return SQLITE_IOERR_FSYNC;
  return SQLITE_OK;
}

static int vita_xOpen(sqlite3_vfs *vfs, const char *name, sqlite3_file *file, int flags, int *out_flags)
{
  sqlite3_vfs* org_vfs = (sqlite3_vfs*)vfs->pAppData;

  LOG("open %s: flags = %08x, ", name, flags);
  // Default xOpen() does not create files => do that ourselves
  // TODO: handle SQLITE_OPEN_EXCLUSIVE
  if (flags & SQLITE_OPEN_CREATE) {
    SceUID fd = sceIoOpen(name, SCE_O_RDONLY, 0777);
    if (IS_ERROR(fd))
      fd = sceIoOpen(name, SCE_O_WRONLY | SCE_O_CREAT, 0777);
    if (!IS_ERROR(fd))
      sceIoClose(fd);
    else
      LOG("create error: %08x\n", fd);
  }

  // Call the original xOpen()
  int r = org_vfs->xOpen(org_vfs, name, file, flags, out_flags);
  vfs_file *p = (vfs_file*)file;
  LOG("fd = %08x, r = %d\n", (p == NULL) ? 0 : *p->fd, r);

  // Default xOpen() also forces read-only on SQLITE_OPEN_READWRITE files
  if ((file->pMethods != NULL) && (flags & SQLITE_OPEN_READWRITE)) {
    if (!IS_ERROR(*p->fd)) {
      // Reopen the file with write access
      sceIoClose(*p->fd);
      *p->fd = sceIoOpen(name, SCE_O_RDWR, 0777);
      LOG("override fd = %08x\n", *p->fd);
      if (IS_ERROR(*p->fd))
        return SQLITE_IOERR_WRITE;
    }
    // Need to override xWrite() and xSync() as well
    if (rw_methods == NULL) {
      rw_methods = malloc(sizeof(sqlite3_io_methods));
      if (rw_methods != NULL) {
        memcpy(rw_methods, file->pMethods, sizeof(sqlite3_io_methods));
        rw_methods->xWrite = vita_xWrite;
        rw_methods->xSync = vita_xSync;
      }
    }
    if (rw_methods != NULL)
      file->pMethods = rw_methods;
  }
  return r;
}

static int vita_xDelete(sqlite3_vfs *vfs, const char *name, int syncDir)
{
  int ret = sceIoRemove(name);
  LOG("delete %s: 0x%08x\n", name, ret);
  if (IS_ERROR(ret))
    return SQLITE_IOERR_DELETE;
  return SQLITE_OK;
}

int sqlite_init()
{
  int rc;

  if (rw_vfs != NULL)
    return SQLITE_OK;

  SceSqliteMallocMethods mf = {
    (void* (*) (int)) malloc,
    (void* (*) (void*, int)) realloc,
    free
  };
  sceSqliteConfigMallocMethods(&mf);

  rw_vfs = malloc(sizeof(sqlite3_vfs));
  sqlite3_vfs *vfs = sqlite3_vfs_find(NULL);
  if ((vfs != NULL) && (rw_vfs != NULL)) {
    // Override xOpen() and xDelete()
    memcpy(rw_vfs, vfs, sizeof(sqlite3_vfs));
    rw_vfs->zName = "psp2_rw";
    rw_vfs->xOpen = vita_xOpen;
    rw_vfs->xDelete = vita_xDelete;
    // Keep a copy of the original vfs pointer
    rw_vfs->pAppData = vfs;
    rc = sqlite3_vfs_register(rw_vfs, 1);
    if (rc != SQLITE_OK) {
      LOG("sqlite_init: could not register vfs: %d\n", rc);
      return rc;
    }
  }
  return SQLITE_OK;
}

int sqlite_exit()
{
  int rc = SQLITE_OK;
  free(rw_methods);
  rw_methods = NULL;
  if (rw_vfs != NULL) {
    rc = sqlite3_vfs_unregister(rw_vfs);
    if (rc != SQLITE_OK)
      LOG("sqlite_exit: error unregistering vfs:  %d\n", rc);
    free(rw_vfs);
    rw_vfs = NULL;
  }
  return rc;
}
