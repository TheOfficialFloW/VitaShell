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

#include <psp2/appmgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../sysmodule_internal.h"
#include "../libpromoter/promoterutil.h"

#define TRANSFER_SIZE 64 * 1024

#define PACKAGE_PARENT "ux0:ptmp"
#define PACKAGE_DIR PACKAGE_PARENT "/pkg"
#define HEAD_BIN PACKAGE_DIR "/sce_sys/package/head.bin"

typedef struct SfoHeader {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} __attribute__((packed)) SfoHeader;

typedef struct SfoEntry {
	uint16_t nameofs;
	uint8_t  alignment;
	uint8_t  type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} __attribute__((packed)) SfoEntry;

int launchAppByUriExit(char *titleid) {
	char uri[32];
	sprintf(uri, "psgm:play?titleid=%s", titleid);

	sceKernelDelayThread(10000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);
	sceKernelDelayThread(10000);
	sceAppMgrLaunchAppByUri(0xFFFFF, uri);

	sceKernelExitProcess(0);

	return 0;
}

int copyFile(char *src_path, char *dst_path) {
	SceUID fdsrc = sceIoOpen(src_path, SCE_O_RDONLY, 0);
	if (fdsrc < 0)
		return fdsrc;

	SceUID fddst = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fddst < 0) {
		sceIoClose(fdsrc);
		return fddst;
	}

	void *buf = malloc(TRANSFER_SIZE);

	int read;
	while ((read = sceIoRead(fdsrc, buf, TRANSFER_SIZE)) > 0) {
		sceIoWrite(fddst, buf, read);
	}

	free(buf);

	sceIoClose(fddst);
	sceIoClose(fdsrc);

	return 0;
}

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

char *get_title_id(const char *filename) {
	char *res = NULL;
	long size = 0;
	FILE *fin = NULL;
	char *buf = NULL;
	int i;

	SfoHeader *header;
	SfoEntry *entry;
	
	fin = fopen(filename, "rb");
	if (!fin)
		goto cleanup;
	if (fseek(fin, 0, SEEK_END) != 0)
		goto cleanup;
	if ((size = ftell(fin)) == -1)
		goto cleanup;
	if (fseek(fin, 0, SEEK_SET) != 0)
		goto cleanup;
	buf = calloc(1, size + 1);
	if (!buf)
		goto cleanup;
	if (fread(buf, size, 1, fin) != 1)
		goto cleanup;

	header = (SfoHeader*)buf;
	entry = (SfoEntry*)(buf + sizeof(SfoHeader));
	for (i = 0; i < header->count; ++i, ++entry) {
		const char *name = buf + header->keyofs + entry->nameofs;
		const char *value = buf + header->valofs + entry->dataofs;
		if (name >= buf + size || value >= buf + size)
			break;
		if (strcmp(name, "TITLE_ID") == 0)
			res = strdup(value);
	}

cleanup:
	if (buf)
		free(buf);
	if (fin)
		fclose(fin);

	return res;
}

int main(int argc, const char *argv[]) {
	char *titleid = get_title_id(PACKAGE_DIR "/sce_sys/param.sfo");
	if (titleid && strcmp(titleid, "VITASHELL") == 0) {
		copyFile(PACKAGE_DIR "/eboot.bin", "ux0:app/MLCL00001/eboot.bin");

		if (promote(PACKAGE_DIR) >= 0)
			launchAppByUriExit("VITASHELL");
	}

	return sceKernelExitProcess(0);
}