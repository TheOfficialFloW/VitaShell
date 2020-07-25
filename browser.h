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

#ifndef __BROWSER_H__
#define __BROWSER_H__

#define MAX_DIR_LEVELS 128

#define HOME_PATH "home"
#define DIR_UP ".."

extern FileList file_list, mark_list, copy_list, install_list;

extern char cur_file[MAX_PATH_LENGTH];
extern char archive_copy_path[MAX_PATH_LENGTH];
extern char archive_path[MAX_PATH_LENGTH];

extern int base_pos, rel_pos;
extern int sort_mode, copy_mode;

extern int last_set_sort_mode;

// minimum time to pass before shortcutting to recent files/ bookmarks via L/R keys
extern SceInt64 time_last_recent_files, time_last_bookmarks;
#define THRESHOLD_LAST_PAD_RECENT_FILES_WAIT 1000000
#define THRESHOLD_LAST_PAD_BOOKMARKS_WAIT 1000000

void dirLevelUp();

void setDirArchiveLevel();
void setInArchive();
int isInArchive();

void setFocusName(const char *name);
void setFocusOnFilename(const char *name);

int refreshFileList();

int jump_to_directory_track_current_path(char *path);

int browserMain();

#endif
