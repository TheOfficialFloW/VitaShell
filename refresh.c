/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW
  Copyright (C) 2017, VitaSmith
  Copyright (C) 2018, TheRadziu
  Copyright (C) 2020, SilicaAndPina

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
#include "io_process.h"
#include "refresh.h"
#include "package_installer.h"
#include "sfo.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"
#include "rif.h"
#include "pfs.h"

// Note: The promotion process is *VERY* sensitive to the directories used below
// Don't change them unless you know what you are doing!
#define APP_TEMP "ux0:temp/app"
#define DLC_TEMP "ux0:temp/addcont"
#define PATCH_TEMP "ux0:temp/patch"
#define PSM_TEMP "ux0:temp/game"
#define THEME_TEMP "ux0:temp/theme"

#define MAX_DLC_PER_TITLE 1024

int isCustomHomebrew(const char* path) {
  uint32_t work[RIF_SIZE/4];

  if (ReadFile(path, work, sizeof(work)) != sizeof(work))
    return 0;

  for (int i = 0; i < sizeof(work) / sizeof(uint32_t); i++)
    if (work[i] != 0)
      return 0;

  return 1;
}



int refreshNeeded(const char *app_path, const char* content_type) {
  char appmeta_path[MAX_PATH_LENGTH];
  char appmeta_param[MAX_PATH_LENGTH];
  char sfo_path[MAX_PATH_LENGTH];
  int mounted_appmeta;
  char titleid[12], contentid[50], appver[8];
  if(strcmp(content_type,"psm") == 0) 
  {
    char contentid_path[MAX_PATH_LENGTH];
    void *cidFile = NULL;
    
    //Initalize buffers
    memset(titleid,0,12);
    memset(contentid,0,50);
    memset(appver,0,8);
    
    snprintf(contentid_path, MAX_PATH_LENGTH, "%s/RW/System/content_id", app_path);
    
    // Get content id
    int contentid_size = allocateReadFile(contentid_path, &cidFile);
    if(contentid_size != 48) //Check if valid contentid file
      return 0;
  
    // Get title id from content id
    strncpy(titleid,cidFile+7,9);
    strncpy(contentid,cidFile,49);
    
    
    free(cidFile);
  }
  else {  
    // Read param.sfo
    snprintf(sfo_path, MAX_PATH_LENGTH, "%s/sce_sys/param.sfo", app_path);
    
    void *sfo_buffer = NULL;
    int sfo_size = allocateReadFile(sfo_path, &sfo_buffer);
    if (sfo_size < 0)
      return 0;

    // Get title and content ids
    
    getSfoString(sfo_buffer, "TITLE_ID", titleid, sizeof(titleid));
    getSfoString(sfo_buffer, "CONTENT_ID", contentid, sizeof(contentid));
    getSfoString(sfo_buffer, "APP_VER", appver, sizeof(appver));

    // Free sfo buffer
    free(sfo_buffer);
  }
  
  
  // Check if app or dlc exists
  if (((strcmp(content_type, "app") == 0)||(strcmp(content_type, "dlc") == 0)||(strcmp(content_type,"psm") == 0))&&(checkAppExist(titleid))) {
    char rif_name[48];
    char rif_path[MAX_PATH_LENGTH];
  
    uint64_t aid;
    sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &aid, sizeof(uint64_t));

    // Check if bounded rif file exits
    _sceNpDrmGetRifName(rif_name, 0, aid);
    if (strcmp(content_type, "app") == 0)
      snprintf(rif_path, MAX_PATH_LENGTH, "ux0:license/app/%s/%s", titleid, rif_name);
    else if (strcmp(content_type, "dlc") == 0)
      snprintf(rif_path, MAX_PATH_LENGTH, "ux0:license/addcont/%s/%s/%s", titleid, &contentid[20], rif_name);
    if (checkFileExist(rif_path))
      return 0;
  
    // Check if fixed rif file exits
    _sceNpDrmGetFixedRifName(rif_name, 0, 0);
    if (strcmp(content_type, "app") == 0)
      snprintf(rif_path, MAX_PATH_LENGTH, "ux0:license/app/%s/%s", titleid, rif_name);
    else if (strcmp(content_type, "dlc") == 0)
      snprintf(rif_path, MAX_PATH_LENGTH, "ux0:license/addcont/%s/%s/%s", titleid, &contentid[20], rif_name);
    if (checkFileExist(rif_path))
      return 0;
  
    if(strcmp(content_type,"psm") == 0) 
      return 0;
 }
  
  // Check if patch for installed app exists
  else if (strcmp(content_type, "patch") == 0) {
    if (!checkAppExist(titleid))
      return 0;
  if (checkFileExist(sfo_path)) {
    void *sfo_buffer = NULL;
      snprintf(appmeta_path, MAX_PATH_LENGTH, "ux0:appmeta/%s", titleid);
      pfsUmount();
      if(pfsMount(appmeta_path)<0)
        return 0;
      //Now read it
    snprintf(appmeta_param, MAX_PATH_LENGTH, "ux0:appmeta/%s/param.sfo", titleid);
      int sfo_size = allocateReadFile(appmeta_param, &sfo_buffer);
      if (sfo_size < 0)
        return sfo_size;
      char promoted_appver[8];
      getSfoString(sfo_buffer, "APP_VER", promoted_appver, sizeof(promoted_appver));
      pfsUmount();
    //Finally compare it
    if (strcmp(appver, promoted_appver) == 0)
      return 0;
    }
  }
  return 1;
}

int refreshApp(const char *app_path) {
  char work_bin_path[MAX_PATH_LENGTH];
  int res;

  snprintf(work_bin_path, MAX_PATH_LENGTH, "%s/sce_sys/package/work.bin", app_path);

  // Remove work.bin for custom homebrews
  if (isCustomHomebrew(work_bin_path)) {
    sceIoRemove(work_bin_path);
  } else if (!checkFileExist(work_bin_path)) {
    // If available, restore work.bin from licenses.db
    void *sfo_buffer = NULL;
    char sfo_path[MAX_PATH_LENGTH], contentid[50];
    snprintf(sfo_path, MAX_PATH_LENGTH, "%s/sce_sys/param.sfo", app_path);
    int sfo_size = allocateReadFile(sfo_path, &sfo_buffer);
    if (sfo_size > 0) {
      getSfoString(sfo_buffer, "CONTENT_ID", contentid, sizeof(contentid));
      uint8_t* rif = query_rif(LICENSE_DB, contentid);
      if (rif != NULL) {
        int fh = sceIoOpen(work_bin_path, SCE_O_WRONLY | SCE_O_CREAT, 0777);
        if (fh > 0) {
          sceIoWrite(fh, rif, RIF_SIZE);
          sceIoClose(fh);
        }
        free(rif);
      }
    }
    free(sfo_buffer);
  }

  // Promote vita app/vita dlc/vita patch (if needed)
  res = promoteApp(app_path);
  return (res < 0) ? res : 1;
}

// target_type should be either SCE_S_IFREG for files or SCE_S_IFDIR for directories
int parse_dir_with_callback(int target_type, const char* path, void(*callback)(void*, const char*, const char*), void* data) {
  SceUID dfd = sceIoDopen(path);
  if (dfd >= 0) {
    int res = 0;

    do {
      SceIoDirent dir;
      memset(&dir, 0, sizeof(SceIoDirent));

      res = sceIoDread(dfd, &dir);
      if (res > 0) {
        if ((dir.d_stat.st_mode & SCE_S_IFMT) == target_type) {
          callback(data, path, dir.d_name);
          if (cancelHandler()) {
            closeWaitDialog();
            setDialogStep(DIALOG_STEP_CANCELED);
            return -1;
          }
        }
      }
    } while (res > 0);
    sceIoDclose(dfd);
  }
  return 0;
}

typedef struct {
  int refresh_pass;
  int count;
  int processed;
  int refreshed;
} refresh_data_t;

typedef struct {
  refresh_data_t *refresh_data;
  char* list[MAX_DLC_PER_TITLE];
  int list_size;
} dlc_data_t;

typedef struct {
  int copy_pass;
  int count;
  int processed;
  int copied;
  int cur_depth;
  int max_depth;
  uint8_t* rif;
} license_data_t;

void app_callback(void* data, const char* dir, const char* subdir) {
  refresh_data_t *refresh_data = (refresh_data_t*)data;
  char path[MAX_PATH_LENGTH];

  if (strcasecmp(subdir, vitashell_titleid) == 0)
    return;

  if (refresh_data->refresh_pass) {
    snprintf(path, MAX_PATH_LENGTH, "%s/%s", dir, subdir);
    if (refreshNeeded(path, "app")) {
      // Move the directory to temp for installation
      removePath(APP_TEMP, NULL);
      sceIoRename(path, APP_TEMP);
      if (refreshApp(APP_TEMP) == 1)
        refresh_data->refreshed++;
      else
        // Restore folder on error
        sceIoRename(APP_TEMP, path);
    }
    SetProgress(++refresh_data->processed, refresh_data->count);
  } else {
    refresh_data->count++;
  }
}

void dlc_callback_inner(void* data, const char* dir, const char* subdir) {
  dlc_data_t *dlc_data = (dlc_data_t*)data;
  char path[MAX_PATH_LENGTH];

  // Ignore  "sce_sys" and "sce_pfs" directories
  if (strncasecmp(subdir, "sce_", 4) == 0)
    return;

  if (dlc_data->refresh_data->refresh_pass) {
    snprintf(path, MAX_PATH_LENGTH, "%s/%s", dir, subdir);
    if (dlc_data->list_size < MAX_DLC_PER_TITLE)
      dlc_data->list[dlc_data->list_size++] = strdup(path);
  } else {
    dlc_data->refresh_data->count++;
  }
}

void dlc_callback_outer(void* data, const char* dir, const char* subdir) {
  refresh_data_t *refresh_data = (refresh_data_t*)data;
  dlc_data_t dlc_data;
  dlc_data.refresh_data = refresh_data;
  dlc_data.list_size = 0;
  char path[MAX_PATH_LENGTH];

  // Get the title's dlc subdirectories
  int len = snprintf(path, sizeof(path), "%s/%s", dir, subdir);
  parse_dir_with_callback(SCE_S_IFDIR, path, dlc_callback_inner, &dlc_data);

  if (refresh_data->refresh_pass) {
    // For dlc, the process happens in two phases to avoid promotion errors:
    // 1. Move all dlc that require refresh out of addcont/title_id
    // 2. Refresh the moved dlc_data
    for (int i = 0; i < dlc_data.list_size; i++) {
      if (refreshNeeded(dlc_data.list[i], "dlc")) {
        snprintf(path, MAX_PATH_LENGTH, DLC_TEMP "/%s", &dlc_data.list[i][len + 1]);
        removePath(path, NULL);
        sceIoRename(dlc_data.list[i], path);
      } else {
        free(dlc_data.list[i]);
        dlc_data.list[i] = NULL;
        SetProgress(++refresh_data->processed, refresh_data->count);
      }
    }

    // Now that the dlc we need are out of addcont/title_id, refresh them
    for (int i = 0; i < dlc_data.list_size; i++) {
      if (dlc_data.list[i] != NULL) {
        snprintf(path, MAX_PATH_LENGTH, DLC_TEMP "/%s", &dlc_data.list[i][len + 1]);
        if (refreshApp(path) == 1)
          refresh_data->refreshed++;
        else
          sceIoRename(path, dlc_data.list[i]);
        SetProgress(++refresh_data->processed, refresh_data->count);
        free(dlc_data.list[i]);
      }
    }
  }
}

void patch_callback(void* data, const char* dir, const char* subdir) {
  refresh_data_t *refresh_data = (refresh_data_t*)data;
  char path[MAX_PATH_LENGTH];

  if (refresh_data->refresh_pass) {
    snprintf(path, MAX_PATH_LENGTH, "%s/%s", dir, subdir);
    if (refreshNeeded(path, "patch")) {
      // Move the directory to temp for installation
      removePath(PATCH_TEMP, NULL);
      sceIoRename(path, PATCH_TEMP);
      if (refreshApp(PATCH_TEMP) == 1)
        refresh_data->refreshed++;
      else
        // Restore folder on error
        sceIoRename(PATCH_TEMP, path);
    }
    SetProgress(++refresh_data->processed, refresh_data->count);
  } else {
    refresh_data->count++;
  }
}

void psm_callback(void* data, const char* dir, const char* subdir) {
  refresh_data_t *refresh_data = (refresh_data_t*)data;
  char path[MAX_PATH_LENGTH];

  if (strcasecmp(subdir, vitashell_titleid) == 0)
    return;

  if (refresh_data->refresh_pass) {
    snprintf(path, MAX_PATH_LENGTH, "%s/%s", dir, subdir);  
    if (refreshNeeded(path, "psm")) {        
    char contentid_path[MAX_PATH_LENGTH];
    snprintf(contentid_path, MAX_PATH_LENGTH, "%s/RW/System/content_id", path);
    
    char titleid[12];
    void *cidFile = NULL;
    
    // Initalize Bufer
    memset(titleid,0,12);

    // Get content id
    allocateReadFile(contentid_path, &cidFile);
  
    // Get title id from content id
    strncpy(titleid,cidFile+7,9);
    
    //free buffers
    free(cidFile);
    
    
    // Get promote path
    char promote_path[MAX_PATH_LENGTH];
    snprintf(promote_path,MAX_PATH_LENGTH,"%s/%s",PSM_TEMP,titleid);

    // Move the directory to temp for installation
    removePath(promote_path, NULL);
    sceIoRename(path, promote_path);

    // Finally call promote
    if (promotePsm(PSM_TEMP,titleid) == 0)
      refresh_data->refreshed++;
    else
      sceIoRename(promote_path, path); // Restore folder on error
    }
    SetProgress(++refresh_data->processed, refresh_data->count);
  } else {
    refresh_data->count++;
  }
}

int refresh_thread(SceSize args, void *argp)  {
  SceUID thid = -1;
  refresh_data_t refresh_data = { 0, 0, 0, 0 };

  // Lock power timers
  powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  // Get the app count
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:app", app_callback, &refresh_data) < 0)
    goto EXIT;

  // Get the dlc count
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:addcont", dlc_callback_outer, &refresh_data) < 0)
    goto EXIT;

  // Get the patch count
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:patch", patch_callback, &refresh_data) < 0)
    goto EXIT;

  // Get the psm count
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:psm", psm_callback, &refresh_data) < 0)
    goto EXIT;

  // Update thread
  thid = createStartUpdateThread(refresh_data.count, 0);

  // Make sure we have the temp directories we need
  sceIoMkdir("ux0:temp", 0006);
  sceIoMkdir(DLC_TEMP, 0006);
  sceIoMkdir(PATCH_TEMP, 0006);
  sceIoMkdir(PSM_TEMP, 0006);
  refresh_data.refresh_pass = 1;

  // Refresh apps
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:app", app_callback, &refresh_data) < 0)
    goto EXIT;

  // Refresh dlc
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:addcont", dlc_callback_outer, &refresh_data) < 0)
    goto EXIT;
  
  // Refresh patch
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:patch", patch_callback, &refresh_data) < 0)
    goto EXIT;

  // Refresh psm
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:psm", psm_callback, &refresh_data) < 0)
    goto EXIT;

  sceIoRmdir(DLC_TEMP);
  sceIoRmdir(PATCH_TEMP);
  sceIoRmdir(PSM_TEMP);

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);

  // Close
  closeWaitDialog();

  infoDialog(language_container[REFRESHED], refresh_data.refreshed);

EXIT:
  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);

  // Unlock power timers
  powerUnlock();

  return sceKernelExitDeleteThread(0);
}

// Note: This is currently not optimized AT ALL.
// Ultimately, we want to use a single transaction and avoid trying to
// re-insert rifs that are already present.
void license_file_callback(void* data, const char* dir, const char* file) {
  license_data_t *license_data = (license_data_t*)data;
  char path[MAX_PATH_LENGTH];

  // Ignore non rif content
  if ((strlen(file) < 4) || (strcasecmp(&file[strlen(file) - 4], ".rif") != 0))
    return;
  if (license_data->copy_pass) {
    snprintf(path, sizeof(path), "%s/%s", dir, file);
    SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0777);
    if (fd > 0) {
      int read = sceIoRead(fd, license_data->rif, RIF_SIZE);
      if (read == RIF_SIZE) {
        if (insert_rif(LICENSE_DB, license_data->rif) == 0)
          license_data->copied++;
      }
      sceIoClose(fd);
    }
    SetProgress(++license_data->processed, license_data->count);
  } else {
    license_data->count++;
  }
}

void license_dir_callback(void* data, const char* dir, const char* subdir) {
  license_data_t *license_data = (license_data_t*)data;
  char path[MAX_PATH_LENGTH];

  snprintf(path, sizeof(path), "%s/%s", dir, subdir);
  if (++license_data->cur_depth == license_data->max_depth)
    parse_dir_with_callback(SCE_S_IFREG, path, license_file_callback, data);
  else
    parse_dir_with_callback(SCE_S_IFDIR, path, license_dir_callback, data);
  license_data->cur_depth--;
}

int license_thread(SceSize args, void *argp) {
  SceUID thid = -1;
  license_data_t license_data = { 0, 0, 0, 0, 0, 1, malloc(RIF_SIZE) };

  if (license_data.rif == NULL)
    goto EXIT;

  // Lock power timers
  powerLock();

  // Set progress to 0%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
  sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

  // NB: ux0:license access requires elevated permisions
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:license/app", license_dir_callback, &license_data) < 0)
    goto EXIT;
  license_data.max_depth++;
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:license/addcont", license_dir_callback, &license_data) < 0)
    goto EXIT;

  // Update thread
  thid = createStartUpdateThread(license_data.count, 0);

  // Create the DB if needed
  SceUID fd = sceIoOpen(LICENSE_DB, SCE_O_RDONLY, 0777);
  if (fd > 0) {
    sceIoClose(fd);
  } else if (create_db(LICENSE_DB, LICENSE_DB_SCHEMA) != 0) {
    goto EXIT;
  }

  // Insert the licenses
  license_data.copy_pass = 1;
  license_data.max_depth = 1;
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:license/app", license_dir_callback, &license_data) < 0)
    goto EXIT;
  license_data.max_depth++;
  if (parse_dir_with_callback(SCE_S_IFDIR, "ux0:license/addcont", license_dir_callback, &license_data) < 0)
    goto EXIT;

  // Set progress to 100%
  sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
  sceKernelDelayThread(COUNTUP_WAIT);

  // Close
  closeWaitDialog();

  infoDialog(language_container[IMPORTED_LICENSES], license_data.copied);

EXIT:
  if (thid >= 0)
    sceKernelWaitThreadEnd(thid, NULL, NULL);

  // Unlock power timers
  powerUnlock();

  free(license_data.rif);
  return sceKernelExitDeleteThread(0);
}
