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
#include "network_update.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"

#define BASE_ADDRESS "https://github.com/TheOfficialFloW/VitaShell/releases/download"
#define VITASHELL_UPDATE_FILE "ux0:VitaShell/VitaShell.vpk"

extern unsigned char _binary_resources_updater_eboot_bin_start;
extern unsigned char _binary_resources_updater_eboot_bin_size;
extern unsigned char _binary_resources_updater_param_bin_start;
extern unsigned char _binary_resources_updater_param_bin_size;

int getDownloadFileSize(char *src, uint64_t *size) {
	int tpl = sceHttpCreateTemplate("VitaShell/1.00 libhttp/1.1", SCE_HTTP_VERSION_1_1, 1);
	if (tpl < 0)
		return tpl;

	int conn = sceHttpCreateConnectionWithURL(tpl, src, 0);
	if (conn < 0)
		return conn;

	int req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, src, 0);
	if (req < 0)
		return req;

	int res = sceHttpSendRequest(req, NULL, 0);
	if (res < 0)
		return res;

	sceHttpGetResponseContentLength(req, size);

	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);
	sceHttpDeleteTemplate(tpl);

	return 0;
}

int downloadFile(char *src, char *dst, uint64_t *value, uint64_t max, void (* SetProgress)(uint64_t value, uint64_t max), int (* cancelHandler)()) {
	int ret = 1;

	int tpl = sceHttpCreateTemplate("VitaShell/1.00 libhttp/1.1", SCE_HTTP_VERSION_1_1, 1);
	if (tpl < 0)
		return tpl;

	int conn = sceHttpCreateConnectionWithURL(tpl, src, 0);
	if (conn < 0)
		return conn;

	int req = sceHttpCreateRequestWithURL(conn, SCE_HTTP_METHOD_GET, src, 0);
	if (req < 0)
		return req;

	int res = sceHttpSendRequest(req, NULL, 0);
	if (res < 0)
		return res;

	SceUID fd = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0)
		return fd;

	uint8_t buf[4096];

	int read;
	while ((read = sceHttpReadData(req, buf, sizeof(buf))) > 0) {
		int res = sceIoWrite(fd, buf, read);
		if (res < 0) {
			ret = res;
			break;
		}

		if (value)
			(*value) += read;

		if (SetProgress)
			SetProgress(value ? *value : 0, max);

		if (cancelHandler && cancelHandler()) {
			sceIoClose(fd);
			ret = 0;
			break;
		}
	}

	sceIoClose(fd);

	sceHttpDeleteRequest(req);
	sceHttpDeleteConnection(conn);
	sceHttpDeleteTemplate(tpl);

	return ret;
}

int downloadProcess(char *version_string) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	// Download url
	char url[128];
	sprintf(url, BASE_ADDRESS "/%s/VitaShell.vpk", version_string);

	// File size
	uint64_t size = 0;
	getDownloadFileSize(url, &size);

	// Update thread
	thid = createStartUpdateThread(size);

	// Download
	uint64_t value = 0;
	int res = downloadFile(url, VITASHELL_UPDATE_FILE, &value, size, SetProgress, cancelHandler);
	if (res <= 0) {
		closeWaitDialog();
		dialog_step = DIALOG_STEP_CANCELLED;
		errorDialog(res);
		goto EXIT;
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_DOWNLOADED;

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}

int network_update_thread(SceSize args, void *argp) {
	sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);

	if (downloadFile(BASE_ADDRESS "/0.0/version.bin", "ux0:VitaShell/version.bin", NULL, 0, NULL, NULL) > 0) {
		uint32_t version = 0;
		ReadFile("ux0:VitaShell/version.bin", &version, sizeof(uint32_t));
		sceIoRemove("ux0:VitaShell/version.bin");

		// Only show update question if no dialog is running
		if (dialog_step == DIALOG_STEP_NONE) {
			// New update available
			if (version > VITASHELL_VERSION) {
				int major = version >> 0x18;
				int minor = version >> 0x10;

				char version_string[8];
				sprintf(version_string, "%X.%X", major, minor);
				if (version_string[3] == '0')
					version_string[3] = '\0';

				// Update question
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[UPDATE_QUESTION], version_string);
				dialog_step = DIALOG_STEP_UPDATE_QUESTION;

				// Wait for response
				while (dialog_step == DIALOG_STEP_UPDATE_QUESTION) {
					sceKernelDelayThread(1000);
				}

				// No
				if (dialog_step == DIALOG_STEP_NONE) {
					return 0;
				}

				// Yes
				return downloadProcess(version_string);
			}
		}
	}

	return sceKernelExitDeleteThread(0);
}

void installUpdater() {
	// Recursively clean up package_temp directory
	removePath(PACKAGE_PARENT, NULL, 0, NULL, NULL);
	sceIoMkdir(PACKAGE_PARENT, 0777);

	sceIoMkdir("ux0:ptmp", 0777);
	sceIoMkdir("ux0:ptmp/pkg", 0777);
	sceIoMkdir("ux0:ptmp/pkg/sce_sys", 0777);

	WriteFile("ux0:ptmp/pkg/eboot.bin", (void *)&_binary_resources_updater_eboot_bin_start, (int)&_binary_resources_updater_eboot_bin_size);
	WriteFile("ux0:ptmp/pkg/sce_sys/param.sfo", (void *)&_binary_resources_updater_param_bin_start, (int)&_binary_resources_updater_param_bin_size);

	// Make head.bin
	makeHeadBin();

	// Promote
	promote(PACKAGE_DIR);
}

int update_extract_thread(SceSize args, void *argp) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	// Install updater
	installUpdater();

	// Recursively clean up package_temp directory
	removePath(PACKAGE_PARENT, NULL, 0, NULL, NULL);
	sceIoMkdir(PACKAGE_PARENT, 0777);

	// Open archive
	int res = archiveOpen(VITASHELL_UPDATE_FILE);
	if (res < 0) {
		closeWaitDialog();
		errorDialog(res);
		goto EXIT;
	}

	// Src path
	char *src_path = VITASHELL_UPDATE_FILE "/";

	// Get archive path info
	uint64_t size = 0;
	uint32_t folders = 0, files = 0;
	getArchivePathInfo(src_path, &size, &folders, &files);

	// Update thread
	thid = createStartUpdateThread(size + folders);

	// Extract process
	uint64_t value = 0;

	res = extractArchivePath(src_path, PACKAGE_DIR "/", &value, size + folders, SetProgress, cancelHandler);
	if (res <= 0) {
		closeWaitDialog();
		dialog_step = DIALOG_STEP_CANCELLED;
		errorDialog(res);
		goto EXIT;
	}

	// Remove update file
	sceIoRemove(VITASHELL_UPDATE_FILE);

	// Make head.bin
	res = makeHeadBin();
	if (res < 0) {
		closeWaitDialog();
		errorDialog(res);
		goto EXIT;
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_EXTRACTED;

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}