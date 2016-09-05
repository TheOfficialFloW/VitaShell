/*
	VitaShell
	Copyright (C) 2015-2016, TheFloW

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
#include "io_process.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"
#include "sha1.h"
#include "sysmodule_internal.h"
#include "libpromoter/promoterutil.h"

#include "resources/base_head_bin.h"

void loadScePaf() {
	uint32_t ptr[0x100] = { 0 };
	ptr[0] = 0;
	ptr[1] = (uint32_t)&ptr[0];
	uint32_t scepaf_argp[] = { 0x400000, 0xEA60, 0x40000, 0, 0 };
	sceSysmoduleLoadModuleInternalWithArg(0x80000008, sizeof(scepaf_argp), scepaf_argp, ptr);
}

int promote(char *path) {
	int res;

	loadScePaf();

	res = sceSysmoduleLoadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	if (res < 0)
		return res;

	res = scePromoterUtilityInit();
	if (res < 0)
		return res;

	res = scePromoterUtilityPromotePkg(path, 0);
	if (res < 0)
		return res;

	int state = 0;
	do {
		res = scePromoterUtilityGetState(&state);
		if (res < 0)
			return res;

		sceKernelDelayThread(100 * 1000);
	} while (state);

	int result = 0;
	res = scePromoterUtilityGetResult(&result);
	if (res < 0)
		return res;

	res = scePromoterUtilityExit();
	if (res < 0)
		return res;

	res = sceSysmoduleUnloadModuleInternal(SCE_SYSMODULE_PROMOTER_UTIL);
	if (res < 0)
		return res;

	return result;
}

void fpkg_hmac(const uint8_t *data, unsigned int len, uint8_t hmac[16]) {
	SHA1_CTX ctx;
	uint8_t sha1[20];
	uint8_t buf[64];

	sha1_init(&ctx);
	sha1_update(&ctx, data, len);
	sha1_final(&ctx, sha1);

	memset(buf, 0, 64);
	memcpy(&buf[0], &sha1[4], 8);
	memcpy(&buf[8], &sha1[4], 8);
	memcpy(&buf[16], &sha1[12], 4);
	buf[20] = sha1[16];
	buf[21] = sha1[1];
	buf[22] = sha1[2];
	buf[23] = sha1[3];
	memcpy(&buf[24], &buf[16], 8);

	sha1_init(&ctx);
	sha1_update(&ctx, buf, 64);
	sha1_final(&ctx, sha1);
	memcpy(hmac, sha1, 16);
}

int makeHeadBin() {
	uint8_t hmac[16];
	uint32_t off;
	uint32_t len;
	uint32_t out;

	// head.bin must not be present
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat(HEAD_BIN, &stat) >= 0)
		return -1;

	// Read param.sfo
	void *sfo_buffer = NULL;
	int res = allocateReadFile(PACKAGE_DIR "/sce_sys/param.sfo", &sfo_buffer);
	if (res < 0)
		return res;

	// Get title id
	char titleid[12];
	getSfoString(sfo_buffer, "TITLE_ID", titleid, sizeof(titleid));
	if (strlen(titleid) != 9) // Enforce TITLE_ID format
		return -2;

	// Free sfo buffer
	free(sfo_buffer);

	// TODO: check category for update installation
	// TODO: use real content_id

	// Allocate head.bin buffer
	uint8_t *head_bin = malloc(sizeof(base_head_bin));
	memcpy(head_bin, base_head_bin, sizeof(base_head_bin));

	// Write full titleid
	char full_title_id[128];
	snprintf(full_title_id, sizeof(full_title_id), "EP9000-%s_00-XXXXXXXXXXXXXXXX", titleid);
	strncpy((char *)&head_bin[0x30], full_title_id, 48);

	// hmac of pkg header
	len = ntohl(*(uint32_t *)&head_bin[0xD0]);
	fpkg_hmac(&head_bin[0], len, hmac);
	memcpy(&head_bin[len], hmac, 16);

	// hmac of pkg info
	off = ntohl(*(uint32_t *)&head_bin[0x8]);
	len = ntohl(*(uint32_t *)&head_bin[0x10]);
	out = ntohl(*(uint32_t *)&head_bin[0xD4]);
	fpkg_hmac(&head_bin[off], len - 64, hmac);
	memcpy(&head_bin[out], hmac, 16);

	// hmac of everything
	len = ntohl(*(uint32_t *)&head_bin[0xE8]);
	fpkg_hmac(&head_bin[0], len, hmac);
	memcpy(&head_bin[len], hmac, 16);

	// Make dir
	sceIoMkdir(PACKAGE_DIR "/sce_sys/package", 0777);

	// Write head.bin
	WriteFile(HEAD_BIN, head_bin, sizeof(base_head_bin));

	free(head_bin);

	return 0;
}

int install_thread(SceSize args_size, InstallArguments *args) {
	SceUID thid = -1;
	int assisted = args->assisted;
	int exit_result = 0;

	// Lock power timers
	powerLock();

	if (assisted) {
		// Set progress to 0%
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
		sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage
	}

	// Recursively clean up package_temp directory
	removePath(PACKAGE_PARENT, NULL, 0, NULL, NULL);
	sceIoMkdir(PACKAGE_PARENT, 0777);

	// Open archive
	int res = archiveOpen(args->file);
	if (res < 0) {
		if (assisted) {
			closeWaitDialog();
			errorDialog(res);
		}
		exit_result = -1;
		goto EXIT;
	}

	// Check permissions
	if (assisted) {
		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s/eboot.bin", args->file);
		SceUID fd = archiveFileOpen(path, SCE_O_RDONLY, 0);
		if (fd >= 0) {
			char buffer[0x88];
			archiveFileRead(fd, buffer, sizeof(buffer));
			archiveFileClose(fd);

			// Team molecule's request: Full permission access warning
			uint64_t authid = *(uint64_t *)(buffer + 0x80);
			if (authid == 0x2F00000000000001 || authid == 0x2F00000000000003) {
				closeWaitDialog();

				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_WARNING]);
				dialog_step = DIALOG_STEP_INSTALL_WARNING;

				// Wait for response
				while (dialog_step == DIALOG_STEP_INSTALL_WARNING) {
					sceKernelDelayThread(1000);
				}

				// Cancelled
				if (dialog_step == DIALOG_STEP_CANCELLED) {
					closeWaitDialog();
					exit_result = -2;
					goto EXIT;
				}

				// Init again
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
				dialog_step = DIALOG_STEP_INSTALLING;
			}
		}
	}

	// Src path
	char src_path[MAX_PATH_LENGTH];
	strcpy(src_path, args->file);
	addEndSlash(src_path);

	// Extract process
	if (assisted) {
		// Get archive path info
		uint64_t size = 0;
		uint32_t folders = 0, files = 0;
		getArchivePathInfo(src_path, &size, &folders, &files);

		// Update thread
		thid = createStartUpdateThread(size + folders);

		uint64_t value = 0;
		res = extractArchivePath(src_path, PACKAGE_DIR "/", &value, size + folders, SetProgress, cancelHandler);
	} else {
		res = extractArchivePath(src_path, PACKAGE_DIR "/", NULL, 0, NULL, NULL);
	}

	if (res <= 0) {
		if (assisted) {
			closeWaitDialog();
			dialog_step = DIALOG_STEP_CANCELLED;
			errorDialog(res);
		}
		exit_result = -3;
		goto EXIT;
	}

	// Close archive
	res = archiveClose();
	if (res < 0) {
		if (assisted) {
			closeWaitDialog();
			errorDialog(res);
		}
		exit_result = -4;
		goto EXIT;
	}

	// Make head.bin
	res = makeHeadBin();
	if (res < 0) {
		if (assisted) {
			closeWaitDialog();
			errorDialog(res);
		}
		exit_result = -5;
		goto EXIT;
	}

	// Promote
	res = promote(PACKAGE_DIR);
	if (res < 0) {
		if (assisted) {
			closeWaitDialog();
			errorDialog(res);
		}
		exit_result = -6;
		goto EXIT;
	}

	if (assisted) {
		// Set progress to 100%
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
		sceKernelDelayThread(COUNTUP_WAIT);

		// Close
		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_INSTALLED;
	}

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(exit_result);
}