/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

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

int isCustomHomebrew() {
	uint32_t work[512/4];
	
	if (ReadFile("ux0:temp/game/sce_sys/package/work.bin", work, sizeof(work)) != sizeof(work))
		return 0;
	
	int i;
	for (i = 0; i < sizeof(work) / sizeof(uint32_t); i++)
		if (work[i] != 0)
			return 0;
		
	return 1;
}

int refreshApp(const char *name) {
	char app_path[MAX_PATH_LENGTH], param_path[MAX_PATH_LENGTH], license_path[MAX_PATH_LENGTH];
	int res;

	// Path
	snprintf(app_path, MAX_PATH_LENGTH, "ux0:app/%s", name);
	snprintf(param_path, MAX_PATH_LENGTH, "ux0:app/%s/sce_sys/param.sfo", name);

	// Read param.sfo
	void *sfo_buffer = NULL;
	int sfo_size = allocateReadFile(param_path, &sfo_buffer);
	if (sfo_size < 0)
		return sfo_size;

	// Get titleid
	char titleid[12];
	getSfoString(sfo_buffer, "TITLE_ID", titleid, sizeof(titleid));

	// Free sfo buffer
	free(sfo_buffer);

	// Check if app exists
	if (checkAppExist(titleid)) {
		char rif_name[48];

		uint64_t aid;
		sceRegMgrGetKeyBin("/CONFIG/NP", "account_id", &aid, sizeof(uint64_t));

		// Check if bounded rif file exits
		_sceNpDrmGetRifName(rif_name, 0, aid);
		snprintf(license_path, MAX_PATH_LENGTH, "ux0:license/app/%s/%s", titleid, rif_name);
		if (checkFileExist(license_path))
			return 0;

		// Check if fixed rif file exits
		_sceNpDrmGetFixedRifName(rif_name, 0, 0);
		snprintf(license_path, MAX_PATH_LENGTH, "ux0:license/app/%s/%s", titleid, rif_name);
		if (checkFileExist(license_path))
			return 0;
	}

	// Clean
	removePath("ux0:temp/game", NULL);
	sceIoMkdir("ux0:temp", 0006);

	// Rename app
	res = sceIoRename(app_path, "ux0:temp/game");
	if (res < 0)
		return res;

	// Remove work.bin for custom homebrews
	if (isCustomHomebrew())
		sceIoRemove("ux0:temp/game/sce_sys/package/work.bin");

	// Promote app
	res = promoteApp("ux0:temp/game");

	// Rename back if it failed
	if (res < 0) {
		sceIoRename("ux0:temp/game", app_path);
		return res;
	}

	// Return success
	return 1;
}

int refresh_thread(SceSize args, void *argp) {
	SceUID thid = -1;
	SceUID dfd = -1;
	int folders = 0;
	int count = 0;	
	int items = 0;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	// Get number of folders
	dfd = sceIoDopen("ux0:app");
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					if (strcmp(dir.d_name, vitashell_titleid) == 0)
						continue;
					
					// Count
					folders++;
					
					if (cancelHandler()) {
						closeWaitDialog();
						setDialogStep(DIALOG_STEP_CANCELLED);
						goto EXIT;
					}
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	}

	// Update thread
	thid = createStartUpdateThread(folders, 0);

	dfd = sceIoDopen("ux0:app");
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					if (strcmp(dir.d_name, vitashell_titleid) == 0)
						continue;

					// Refresh app
					if (refreshApp(dir.d_name) == 1)
						items++;
					
					SetProgress(++count, folders);
					
					if (cancelHandler()) {
						closeWaitDialog();
						setDialogStep(DIALOG_STEP_CANCELLED);
						goto EXIT;
					}
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	closeWaitDialog();

	infoDialog(language_container[REFRESHED], items);

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}