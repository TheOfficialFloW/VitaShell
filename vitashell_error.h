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

#ifndef __VITASHELL_ERROR_H__
#define __VITASHELL_ERROR_H__

enum VitaShellErrors {
  VITASHELL_ERROR_INTERNAL                = 0xF0010000,
  VITASHELL_ERROR_NO_MEMORY               = 0xF0010001,
  VITASHELL_ERROR_NOT_FOUND               = 0xF0010002,
  VITASHELL_ERROR_INVALID_ARGUMENT        = 0xF0010003,
  VITASHELL_ERROR_INVALID_MAGIC           = 0xF0010004,
  VITASHELL_ERROR_INVALID_TYPE            = 0xF0010005,
  VITASHELL_ERROR_ILLEGAL_ADDR            = 0xF0010006,
  VITASHELL_ERROR_ALREADY_RUNNING         = 0xF0010007,
  VITASHELL_ERROR_NOT_RUNNING             = 0xF0010008,

  VITASHELL_ERROR_SRC_AND_DST_IDENTICAL   = 0xF0020000,
  VITASHELL_ERROR_DST_IS_SUBFOLDER_OF_SRC = 0xF0020001,

  VITASHELL_ERROR_INVALID_TITLEID         = 0xF0030000,

  VITASHELL_ERROR_SYMLINK_INVALID_PATH = 0xF0040000,
  VITASHELL_ERROR_SYMLINK_CANT_RESOLVE_BASEDIR = 0xF0040001,
  VITASHELL_ERROR_SYMLINK_CANT_RESOLVE_FILENAME = 0xF0040002,
  VITASHELL_ERROR_SYMLINK_INTERNAL = 0xF0040003,

  VITASHELL_ERROR_NAVIGATION = 0xF0050000,




};

#endif
