/*
  VitaShell - RIF and license.db handling
  Copyright (C) 2017 VitaSmith

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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "rif.h"
#include "sqlite3.h"

#define MAX_QUERY_LENGTH 128

int create_db(const char* db_path, const char* schema)
{
  int rc;
  sqlite3 *db = NULL;

  rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_CREATE | SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK)
    goto out;
  rc = sqlite3_exec(db, schema, NULL, NULL, NULL);
  if (rc != SQLITE_OK)
    goto out;

out:
  sqlite3_close(db);
  return rc;
}

// Insert a new RIF into the DB
int insert_rif(const char* db_path, const uint8_t* rif)
{
  int rc;
  sqlite3 *db = NULL;
  sqlite3_stmt *stmt;
  char query[MAX_QUERY_LENGTH];

  rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READWRITE, NULL);
  if (rc != SQLITE_OK)
    goto out;

  snprintf(query, sizeof(query), "INSERT INTO Licenses VALUES('%s', ?)", &rif[0x10]);

  rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    goto out;
  rc = sqlite3_bind_blob(stmt, 1, rif, RIF_SIZE, SQLITE_STATIC);
  if (rc != SQLITE_OK)
    goto out;
  rc = sqlite3_step(stmt);
  if (rc != SQLITE_DONE)
    goto out;
  rc = sqlite3_finalize(stmt);

out:
  sqlite3_close(db);
  return rc;
}

// Query a RIF from the license database. Returned data must be freed by the caller.
uint8_t* query_rif(const char* db_path, const char* content_id)
{
  int rc, rif_size = 0;
  sqlite3 *db = NULL;
  sqlite3_stmt *stmt;
  char query[MAX_QUERY_LENGTH];
  uint8_t *rif = NULL;
  const uint8_t *db_rif;

  rc = sqlite3_open_v2(db_path, &db, SQLITE_OPEN_READONLY, NULL);
  if (rc != SQLITE_OK)
    goto out;

  snprintf(query, sizeof(query), "SELECT RIF FROM Licenses WHERE CONTENT_ID = '%s'", content_id);

  rc = sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
  if (rc != SQLITE_OK)
    goto out;

  rc = sqlite3_step(stmt);
  if (rc == SQLITE_ROW)
    rif_size = sqlite3_column_bytes(stmt, 0);
  if (rif_size != RIF_SIZE)
    goto out;

  rif = malloc(rif_size);
  db_rif = sqlite3_column_blob(stmt, 0);

  if ((rif != NULL) && (db_rif != NULL))
    memcpy(rif, db_rif, rif_size);
  rc = sqlite3_finalize(stmt);

out:
  sqlite3_close(db);
  return rif;
}
