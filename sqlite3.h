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
** This header was intentionally truncated from the original version to
** only include the elements needed by this application.
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

#define SQLITE_OPEN_READONLY         0x00000001
#define SQLITE_OPEN_READWRITE        0x00000002
#define SQLITE_OPEN_CREATE           0x00000004
#define SQLITE_OPEN_URI              0x00000040
#define SQLITE_OPEN_NOMUTEX          0x00008000
#define SQLITE_OPEN_FULLMUTEX        0x00010000
#define SQLITE_OPEN_SHAREDCACHE      0x00020000
#define SQLITE_OPEN_PRIVATECACHE     0x00040000

typedef struct sqlite3 sqlite3;
typedef struct sqlite3_stmt sqlite3_stmt;

SQLITE_API int sqlite3_errcode(sqlite3 *db);
SQLITE_API int sqlite3_extended_errcode(sqlite3 *db);
SQLITE_API const char *sqlite3_errmsg(sqlite3*);
SQLITE_API void *sqlite3_malloc(int);
SQLITE_API void *sqlite3_realloc(void*, int);
SQLITE_API void sqlite3_free(void*);
SQLITE_API int sqlite3_open_v2(const char *filename, sqlite3 **ppDb, int flags, const char *zVfs);
SQLITE_API int sqlite3_close(sqlite3 *);
SQLITE_API int sqlite3_exec(sqlite3*, const char *sql, int (*callback)(void*,int,char**,char**), void *, char **errmsg);
SQLITE_API int sqlite3_prepare_v2(sqlite3 *db, const char *zSql, int nByte, sqlite3_stmt **ppStmt, const char **pzTail);
SQLITE_API int sqlite3_step(sqlite3_stmt*);
SQLITE_API int sqlite3_column_bytes(sqlite3_stmt*, int iCol);
SQLITE_API const void *sqlite3_column_blob(sqlite3_stmt*, int iCol);

#ifdef __cplusplus
}
#endif

#endif