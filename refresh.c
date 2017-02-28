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

int refreshApp(const char *titleid) {
	int res;
	int ret;

	res = scePromoterUtilityInit();
	if (res < 0)
		goto ERROR_EXIT;

	res = scePromoterUtilityCheckExist(titleid, &ret);
	if (res >= 0)
		goto ERROR_EXIT;

	// Clean
	removePath("ux0:temp/game", NULL);
	sceIoMkdir("ux0:temp", 0006);

	// Path
	char path[MAX_PATH_LENGTH];
	snprintf(path, MAX_PATH_LENGTH, "ux0:app/%s", titleid);

	// Rename app
	res = sceIoRename(path, "ux0:temp/game");
	if (res < 0)
		goto ERROR_EXIT;

	// Remove work.bin for custom homebrews
	if (isCustomHomebrew())
		sceIoRemove("ux0:temp/game/sce_sys/package/work.bin");

	res = scePromoterUtilityPromotePkgWithRif("ux0:temp/game", 1);

	// Rename back if it failed, or set to 1 if succeeded
	if (res < 0)
		sceIoRename("ux0:temp/game", path);
	else
		res = 1;

ERROR_EXIT:
	scePromoterUtilityExit();

	return res;
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