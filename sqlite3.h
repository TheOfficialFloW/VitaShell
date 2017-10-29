/*
** 2001 September 15
**
** The author disclaims copyright to this source code.  In place of
** a legal notice, here is a blessing:
**
**    May you do good and not evil.
**    May you find forgiveness for yourself and forgive others.
**    May you share freely, never taking more than you give.
**
*************************************************************************
**
** This header was intentionally truncated from the original version
** to include only the elements needed by our application.
**
*/

#ifndef _SQLITE3_H_
#define _SQLITE3_H_

#ifdef __cplusplus
extern "C" {
#endif

#ifndef SQLITE_EXTERN
# define SQLITE_EXTERN extern
#endif

#ifndef SQLITE_API
# define SQLITE_API
#endif

#define SQLITE_OK           0   /* Successful result */
#define SQLITE_ERROR        1   /* SQL error or missing database */
#define SQLITE_INTERNAL     2   /* Internal logic error in SQLite */
#define SQLITE_PERM         3   /* Access permission denied */
#define SQLITE_ABORT        4   /* Callback routine requested an abort */
#define SQLITE_BUSY         5   /* The database file is locked */
#define SQLITE_LOCKED       6   /* A table in the database is locked */
#define SQLITE_NOMEM        7   /* A malloc() failed */
#define SQLITE_READONLY     8   /* Attempt to write a readonly database */
#define SQLITE_INTERRUPT    9   /* Operation terminated by sqlite3_interrupt()*/
#define SQLITE_IOERR       10   /* Some kind of disk I/O error occurred */
#define SQLITE_CORRUPT     11   /* The database disk image is malformed */
#define SQLITE_NOTFOUND    12   /* Unknown opcode in sqlite3_file_control() */
#define SQLITE_FULL        13   /* Insertion failed because database is full */
#define SQLITE_CANTOPEN    14   /* Unable to open the database file */
#define SQLITE_PROTOCOL    15   /* Database lock protocol error */
#define SQLITE_EMPTY       16   /* Database is empty */
#define SQLITE_SCHEMA      17   /* The database schema changed */
#define SQLITE_TOOBIG      18   /* String or BLOB exceeds size limit */
#define SQLITE_CONSTRAINT  19   /* Abort due to constraint violation */
#define SQLITE_MISMATCH    20   /* Data type mismatch */
#define SQLITE_MISUSE      21   /* Library used incorrectly */
#define SQLITE_NOLFS       22   /* Uses OS features not supported on host */
#define SQLITE_AUTH        23   /* Authorization denied */
#define SQLITE_FORMAT      24   /* Auxiliary database format error */
#define SQLITE_RANGE       25   /* 2nd parameter to sqlite3_bind out of range */
#define SQLITE_NOTADB      26   /* File opened that is not a database file */
#define SQLITE_ROW         100  /* sqlite3_step() has another row ready */
#define SQLITE_DONE        101  /* sqlite3_step() has finished executing */

#define SQLITE_IOERR_READ              (SQLITE_IOERR | (1<<8))
#define SQLITE_IOERR_SHORT_READ        (SQLITE_IOERR | (2<<8))
#define SQLITE_IOERR_WRITE             (SQLITE_IOERR | (3<<8))
#define SQLITE_IOERR_FSYNC             (SQLITE_IOERR | (4<<8))
#define SQLITE_IOERR_DIR_FSYNC         (SQLITE_IOERR | (5<<8))
#define SQLITE_IOERR_TRUNCATE          (SQLITE_IOERR | (6<<8))
#define SQLITE_IOERR_FSTAT             (SQLITE_IOERR | (7<<8))
#define SQLITE_IOERR_UNLOCK            (SQLITE_IOERR | (8<<8))
#define SQLITE_IOERR_RDLOCK            (SQLITE_IOERR | (9<<8))
#define SQLITE_IOERR_DELETE            (SQLITE_IOERR | (10<<8))
#define SQLITE_IOERR_BLOCKED           (SQLITE_IOERR | (11<<8))
#define SQLITE_IOERR_NOMEM             (SQLITE_IOERR | (12<<8))
#define SQLITE_IOERR_ACCESS            (SQLITE_IOERR | (13<<8))
#define SQLITE_IOERR_CHECKRESERVEDLOCK (SQLITE_IOERR | (14<<8))
#define SQLITE_IOERR_LOCK              (SQLITE_IOERR | (15<<8))
#define SQLITE_IOERR_CLOSE             (SQLITE_IOERR | (16<<8))
#define SQLITE_IOERR_DIR_CLOSE         (SQLITE_IOERR | (17<<8))
#define SQLITE_IOERR_SHMOPEN           (SQLITE_IOERR | (18<<8))
#define SQLITE_IOERR_SHMSIZE           (SQLITE_IOERR | (19<<8))
#define SQLITE_IOERR_SHMLOCK           (SQLITE_IOERR | (20<<8))
#define SQLITE_IOERR_SHMMAP            (SQLITE_IOERR | (21<<8))
#define SQLITE_IOERR_SEEK              (SQLITE_IOERR | (22<<8))
#define SQLITE_LOCKED_SHAREDCACHE      (SQLITE_LOCKED |  (1<<8))
#define SQLITE_BUSY_RECOVERY           (SQLITE_BUSY   |  (1<<8))
#define SQLITE_CANTOPEN_NOTEMPDIR      (SQLITE_CANTOPEN | (1<<8))
#define SQLITE_CORRUPT_VTAB            (SQLITE_CORRUPT | (1<<8))
#define SQLITE_READONLY_RECOVERY       (SQLITE_READONLY | (1<<8))
#define SQLITE_READONLY_CANTLOCK       (SQLITE_READONLY | (2<<8))

#define SQLITE_OPEN_READONLY         0x00000001
#define SQLITE_OPEN_READWRITE        0x00000002
#define SQLITE_OPEN_CREATE           0x00000004
#define SQLITE_OPEN_URI              0x00000040
#define SQLITE_OPEN_NOMUTEX          0x00008000
#define SQLITE_OPEN_FULLMUTEX        0x00010000
#define SQLITE_OPEN_SHAREDCACHE      0x00020000
#define SQLITE_OPEN_PRIVATECACHE     0x00040000

#ifdef SQLITE_INT64_TYPE
  typedef SQLITE_INT64_TYPE sqlite_int64;
  typedef unsigned SQLITE_INT64_TYPE sqlite_uint64;
#elif defined(_MSC_VER) || defined(__BORLANDC__)
  typedef __int64 sqlite_int64;
  typedef unsigned __int64 sqlite_uint64;
#else
  typedef long long int sqlite_int64;
  typedef unsigned long long int sqlite_uint64;
#endif
  typedef sqlite_int64 sqlite3_int64;
  typedef sqlite_uint64 sqlite3_uint64;

typedef void (*sqlite3_destructor_type)(void*);
#define SQLITE_STATIC      ((sqlite3_destructor_type)0)
#define SQLITE_TRANSIENT   ((sqlite3_destructor_type)-1)

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

typedef struct sqlite3_file sqlite3_file;
struct sqlite3_file {
  const struct sqlite3_io_methods *pMethods;
};

typedef struct sqlite3_io_methods sqlite3_io_methods;
struct sqlite3_io_methods {
  int iVersion;
  int(*xClose)(sqlite3_file*);
  int(*xRead)(sqlite3_file*, void*, int, sqlite3_int64);
  int(*xWrite)(sqlite3_file*, const void*, int, sqlite3_int64);
  int(*xTruncate)(sqlite3_file*, sqlite3_int64);
  int(*xSync)(sqlite3_file*, int);
  int(*xFileSize)(sqlite3_file*, sqlite3_int64*);
  int(*xLock)(sqlite3_file*, int);
  int(*xUnlock)(sqlite3_file*, int);
  int(*xCheckReservedLock)(sqlite3_file*, int*);
  int(*xFileControl)(sqlite3_file*, int, void*);
  int(*xSectorSize)(sqlite3_file*);
  int(*xDeviceCharacteristics)(sqlite3_file*);
  int(*xShmMap)(sqlite3_file*, int, int, int, void volatile**);
  int(*xShmLock)(sqlite3_file*, int, int, int);
  void(*xShmBarrier)(sqlite3_file*);
  int(*xShmUnmap)(sqlite3_file*, int);
};

typedef struct sqlite3_vfs sqlite3_vfs;
typedef void(*sqlite3_syscall_ptr)(void);
struct sqlite3_vfs {
  int iVersion;
  int szOsFile;
  int mxPathname;
  sqlite3_vfs *pNext;
  const char *zName;
  void *pAppData;
  int(*xOpen)(sqlite3_vfs*, const char *zName, sqlite3_file*, int flags, int *pOutFlags);
  int(*xDelete)(sqlite3_vfs*, const char *zName, int syncDir);
  int(*xAccess)(sqlite3_vfs*, const char *zName, int flags, int *pResOut);
  int(*xFullPathname)(sqlite3_vfs*, const char *zName, int nOut, char *zOut);
  void *(*xDlOpen)(sqlite3_vfs*, const char *zFilename);
  void(*xDlError)(sqlite3_vfs*, int nByte, char *zErrMsg);
  void(*(*xDlSym)(sqlite3_vfs*, void*, const char *zSymbol))(void);
  void(*xDlClose)(sqlite3_vfs*, void*);
  int(*xRandomness)(sqlite3_vfs*, int nByte, char *zOut);
  int(*xSleep)(sqlite3_vfs*, int microseconds);
  int(*xCurrentTime)(sqlite3_vfs*, double*);
  int(*xGetLastError)(sqlite3_vfs*, int, char *);
  int(*xCurrentTimeInt64)(sqlite3_vfs*, sqlite3_int64*);
  int(*xSetSystemCall)(sqlite3_vfs*, const char *zName, sqlite3_syscall_ptr);
  sqlite3_syscall_ptr(*xGetSystemCall)(sqlite3_vfs*, const char *zName);
  const char *(*xNextSystemCall)(sqlite3_vfs*, const char *zName);
};

SQLITE_API int sqlite3_errcode(sqlite3*);
SQLITE_API int sqlite3_extended_errcode(sqlite3*);
SQLITE_API const char *sqlite3_errmsg(sqlite3*);
SQLITE_API void *sqlite3_malloc(int);
SQLITE_API void *sqlite3_realloc(void*, int);
SQLITE_API void sqlite3_free(void*);
SQLITE_API int sqlite3_open(const char*, sqlite3**);
SQLITE_API int sqlite3_open_v2(const char *filename, sqlite3**, int, const char*);
SQLITE_API int sqlite3_close(sqlite3*);
SQLITE_API int sqlite3_exec(sqlite3*, const char *sql, int (*)(void*,int,char**,char**), void*, char**);
SQLITE_API int sqlite3_prepare_v2(sqlite3*, const char*, int, sqlite3_stmt**, const char**);
SQLITE_API int sqlite3_step(sqlite3_stmt*);
SQLITE_API int sqlite3_finalize(sqlite3_stmt*);
SQLITE_API int sqlite3_column_bytes(sqlite3_stmt*, int);
SQLITE_API const void *sqlite3_column_blob(sqlite3_stmt*, int);
SQLITE_API int sqlite3_bind_blob(sqlite3_stmt*, int, const void*, int, void(*)(void*));
SQLITE_API sqlite3_vfs *sqlite3_vfs_find(const char *);
SQLITE_API int sqlite3_vfs_register(sqlite3_vfs*, int);
SQLITE_API int sqlite3_vfs_unregister(sqlite3_vfs*);

#ifdef __cplusplus
}
#endif

#endif
