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
#include "file.h"
#include "utils.h"

int getDownloadFileSize(char *src, uint64_t *size) {
	int tpl = sceHttpCreateTemplate("VitaShell", 2, 1);
	if (tpl < 0)
		return tpl;

	int conn = sceHttpCreateConnectionWithURL(tpl, src, 0);
	if (conn < 0)
		return conn;

	int req = sceHttpCreateRequestWithURL(conn, 0, src, 0);
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

int download_thread(SceSize args_size, void *args) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	// do stuff

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	//dialog_step = DIALOG_STEP_DOWNLOADED;

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}

int automaticNetworkUpdate() {
	sceHttpsDisableOption(SCE_HTTPS_FLAG_SERVER_VERIFY);

	if (downloadFile("http://github.com/TheOfficialFloW/VitaShell/releases/download/0.0/version.bin", "ux0:VitaShell/version.bin", NULL, 0, NULL, NULL) > 0) {
		uint32_t version = 0;
		ReadFile("ux0:VitaShell/version.bin", &version, sizeof(uint32_t));
		sceIoRemove("ux0:VitaShell/version.bin");

		debugPrintf("newest: 0x%08X, current: 0x%08X\n", version, VITASHELL_VERSION);

		if (version >= VITASHELL_VERSION) { // TODO, only >
			int major = version >> 0x18;
			int minor = version >> 0x10;

			char version_string[8];
			sprintf(version_string, "%X.%X", major, minor);
			if (version_string[3] == '0')
				version_string[3] = '\0';

			char url[128];
			sprintf(url, "https://github.com/TheOfficialFloW/VitaShell/releases/download/%s/VitaShell.vpk", version_string);
			downloadFile(url, "ux0:VitaShell/Vitashell.vpk", NULL, 0, NULL, NULL);
		}
	}
}