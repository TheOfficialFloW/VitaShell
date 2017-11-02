/*
  VitaShell - RIF handling functions
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

#pragma once
#include <stdint.h>

#define RIF_SIZE 512
#define LICENSE_DB "ux0:license/license.db"
#define LICENSE_DB_SCHEMA \
  "CREATE TABLE Licenses (" \
    "CONTENT_ID TEXT NOT NULL UNIQUE," \
    "RIF BLOB NOT NULL," \
    "PRIMARY KEY(CONTENT_ID)" \
  ")"

int create_db(const char* db_path, const char* schema);
int insert_rif(const char* db_path, const uint8_t* rif);
uint8_t* query_rif(const char* db_path, const char* content_id);

// From sqlite3.c
int sqlite_init();
int sqlite_exit();
