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
#include "io_process.h"
#include "network_update.h"
#include "package_installer.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"

#define BASE_ADDRESS "https://github.com/TheOfficialFloW/VitaShell/releases/download"
#define VERSION_URL "/0.2/version.bin"
#define VITASHELL_UPDATE_FILE "ux0:VitaShell/internal/VitaShell.vpk"
#define VITASHELL_VERSION_FILE "ux0:VitaShell/internal/version.bin"

#define VITASHELL_USER_AGENT "VitaShell/1.00 libhttp/1.1"

extern unsigned char _binary_resources_updater_eboot_bin_start;
extern unsigned char _binary_resources_updater_eboot_bin_size;
extern unsigned char _binary_resources_updater_param_bin_start;
extern unsigned char _binary_resources_updater_param_bin_size;

int getDownloadFileSize(const char *src, uint64_t *size) {
	int res;
	int statusCode;
	int tmplId = -1, connId = -1, reqId = -1;

	res = sceHttpCreateTemplate(VITASHELL_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
	if (res < 0)
		goto ERROR_EXIT;

	tmplId = res;

	res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
	if (res < 0)
		goto ERROR_EXIT;

	connId = res;

	res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
	if (res < 0)
		goto ERROR_EXIT;

	reqId = res;

	res = sceHttpSendRequest(reqId, NULL, 0);
	if (res < 0)
		goto ERROR_EXIT;

	res = sceHttpGetStatusCode(reqId, &statusCode);
	if (res < 0)
		goto ERROR_EXIT;

	if (statusCode == 200) {
		res = sceHttpGetResponseContentLength(reqId, size);
	}

ERROR_EXIT:
	if (reqId >= 0)
		sceHttpDeleteRequest(reqId);

	if (connId >= 0)
		sceHttpDeleteConnection(connId);

	if (tmplId >= 0)
		sceHttpDeleteTemplate(tmplId);

	return res;
}

int downloadFile(const char *src, const char *dst, FileProcessParam *param) {
	int res;
	int statusCode;
	int tmplId = -1, connId = -1, reqId = -1;
	SceUID fd = -1;
	int ret = 1;

	res = sceHttpCreateTemplate(VITASHELL_USER_AGENT, SCE_HTTP_VERSION_1_1, SCE_TRUE);
	if (res < 0)
		goto ERROR_EXIT;

	tmplId = res;

	res = sceHttpCreateConnectionWithURL(tmplId, src, SCE_TRUE);
	if (res < 0)
		goto ERROR_EXIT;

	connId = res;

	res = sceHttpCreateRequestWithURL(connId, SCE_HTTP_METHOD_GET, src, 0);
	if (res < 0)
		goto ERROR_EXIT;

	reqId = res;

	res = sceHttpSendRequest(reqId, NULL, 0);
	if (res < 0)
		goto ERROR_EXIT;

	res = sceHttpGetStatusCode(reqId, &statusCode);
	if (res < 0)
		goto ERROR_EXIT;

	if (statusCode == 200) {		
		res = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
		if (res < 0)
			goto ERROR_EXIT;

		fd = res;

		uint8_t buf[4096];

		while (1) {
			int read = sceHttpReadData(reqId, buf, sizeof(buf));
			
			if (read < 0) {
				res = read;
				break;
			}
			
			if (read == 0)
				break;

			int written = sceIoWrite(fd, buf, read);
			
			if (written < 0) {
				res = written;
				break;
			}

			if (param) {
				if (param->value)
					(*param->value) += read;

				if (param->SetProgress)
					param->SetProgress(param->value ? *param->value : 0, param->max);

				if (param->cancelHandler && param->cancelHandler()) {
					ret = 0;
					break;
				}
			}
		}
	}

ERROR_EXIT:
	if (fd >= 0)
		sceIoClose(fd);

	if (reqId >= 0)
		sceHttpDeleteRequest(reqId);

	if (connId >= 0)
		sceHttpDeleteConnection(connId);

	if (tmplId >= 0)
		sceHttpDeleteTemplate(tmplId);

	if (res < 0)
		return res;

	return ret;
}

static int downloadProcess(char *version_string) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	// Download url
	char url[128];
	snprintf(url, sizeof(url), BASE_ADDRESS "/%s/VitaShell.vpk", version_string);

	// File size
	uint64_t size = 0;
	getDownloadFileSize(url, &size);

	// Update thread
	thid = createStartUpdateThread(size, 1);

	// Download
	uint64_t value = 0;
	
	FileProcessParam param;
	param.value = &value;
	param.max = size;
	param.SetProgress = SetProgress;
	param.cancelHandler = cancelHandler;

	int res = downloadFile(url, VITASHELL_UPDATE_FILE, &param);
	if (res <= 0) {
		closeWaitDialog();
		setDialogStep(DIALOG_STEP_CANCELLED);
		errorDialog(res);
		goto EXIT;
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	setDialogStep(DIALOG_STEP_DOWNLOADED);

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}

int network_update_thread(SceSize args, void *argp) {
	sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);

	uint64_t size = 0;
	if (getDownloadFileSize(BASE_ADDRESS VERSION_URL, &size) >= 0 && size == sizeof(uint32_t)) {
		int res = downloadFile(BASE_ADDRESS VERSION_URL, VITASHELL_VERSION_FILE, NULL);
		if (res <= 0)
			goto EXIT;

		// Read version
		uint32_t version = 0;
		ReadFile(VITASHELL_VERSION_FILE, &version, sizeof(uint32_t));
		sceIoRemove(VITASHELL_VERSION_FILE);

		// Only show update question if no dialog is running
		if (getDialogStep() == DIALOG_STEP_NONE) {
			// New update available
			if (version > VITASHELL_VERSION) {
				int major = (version >> 0x18) & 0xFF;
				int minor = (version >> 0x10) & 0xFF;

				char version_string[8];
				sprintf(version_string, "%X.%02X", major, minor);
				if (version_string[3] == '0')
					version_string[3] = '\0';

				// Update question
				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[UPDATE_QUESTION], version_string);
				setDialogStep(DIALOG_STEP_UPDATE_QUESTION);

				// Wait for response
				while (getDialogStep() == DIALOG_STEP_UPDATE_QUESTION) {
					sceKernelDelayThread(10 * 1000);
				}

				// No
				if (getDialogStep() == DIALOG_STEP_NONE) {
					goto EXIT;
				}

				// Yes
				return downloadProcess(version_string);
			}
		}
	}

EXIT:
	return sceKernelExitDeleteThread(0);
}

void installUpdater() {
	// Recursively clean up pkg directory
	removePath(PACKAGE_DIR, NULL);
	sceIoMkdir(PACKAGE_DIR, 0777);

	// Make dir
	sceIoMkdir("ux0:data/pkg/sce_sys", 0777);

	// Write VitaShell updater files
	WriteFile("ux0:data/pkg/eboot.bin", (void *)&_binary_resources_updater_eboot_bin_start, (int)&_binary_resources_updater_eboot_bin_size);
	WriteFile("ux0:data/pkg/sce_sys/param.sfo", (void *)&_binary_resources_updater_param_bin_start, (int)&_binary_resources_updater_param_bin_size);

	// Make head.bin
	makeHeadBin();

	// Promote app
	promoteApp(PACKAGE_DIR);
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

	// Recursively clean up pkg directory
	removePath(PACKAGE_DIR, NULL);
	sceIoMkdir(PACKAGE_DIR, 0777);

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
	thid = createStartUpdateThread(size + folders*DIRECTORY_SIZE, 1);

	// Extract process
	uint64_t value = 0;

	FileProcessParam param;
	param.value = &value;
	param.max = size + folders * DIRECTORY_SIZE;
	param.SetProgress = SetProgress;
	param.cancelHandler = cancelHandler;

	res = extractArchivePath(src_path, PACKAGE_DIR "/", &param);
	if (res <= 0) {
		closeWaitDialog();
		setDialogStep(DIALOG_STEP_CANCELLED);
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

	setDialogStep(DIALOG_STEP_EXTRACTED);

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}
