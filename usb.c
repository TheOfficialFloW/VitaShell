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
#include "usb.h"
#include "file.h"
#include "utils.h"

int mountUsbUx0() {
	sceAppMgrDestroyOtherApp();

	// Create dirs
	sceIoMkdir("uma0:app", 0006);
	sceIoMkdir("uma0:appmeta", 0006);
	sceIoMkdir("uma0:license", 0006);
	sceIoMkdir("uma0:license/app", 0006);
	sceIoMkdir("uma0:app/MLCL00001", 0006);
	sceIoMkdir("uma0:appmeta/MLCL00001", 0006);
	sceIoMkdir("uma0:license/app/MLCL00001", 0006);
	sceIoMkdir("uma0:app/VITASHELL", 0006);
	sceIoMkdir("uma0:appmeta/VITASHELL", 0006);
	sceIoMkdir("uma0:license/app/VITASHELL", 0006);

	// Copy molecularShell and VitaShell
	copyPath("ux0:app/MLCL00001", "uma0:app/MLCL00001", NULL);
	copyPath("ux0:appmeta/MLCL00001", "uma0:appmeta/MLCL00001", NULL);
	copyPath("ux0:license/app/MLCL00001", "uma0:license/app/MLCL00001", NULL);
	copyPath("ux0:app/VITASHELL", "uma0:app/VITASHELL", NULL);
	copyPath("ux0:appmeta/VITASHELL", "uma0:appmeta/VITASHELL", NULL);
	copyPath("ux0:license/app/VITASHELL", "uma0:license/app/VITASHELL", NULL);

	// Create important dirs
	sceIoMkdir("uma0:data", 0777);
	sceIoMkdir("uma0:temp", 0777);
	sceIoMkdir("uma0:temp/app_work/", 0777);
	sceIoMkdir("uma0:temp/app_work/MLCL00001", 0777);
	sceIoMkdir("uma0:temp/app_work/MLCL00001/rec", 0777);

	// Copy important files
	copyPath("ux0:temp/app_work/MLCL00001/rec/config.bin", "uma0:temp/app_work/MLCL00001/rec/config.bin", NULL);
	copyPath("ux0:iconlayout.ini", "uma0:iconlayout.ini", NULL);
	copyPath("ux0:id.dat", "uma0:id.dat", NULL);

	// Remove lastdir.txt file
	sceIoRemove("uma0:VitaShell/internal/lastdir.txt");

	// Redirect ux0: to uma0:
	redirectUx0();

	// Change to lowest priority
	sceKernelChangeThreadPriority(sceKernelGetThreadId(), 191);

	// Mount USB ux0:
	vshIoUmount(0xF00, 0, 0, 0);
	remount(0x800);

	// Trick
	char * const argv[] = { "mount", NULL };
	sceAppMgrLoadExec("app0:eboot.bin", argv, NULL);

	return 0;
}

int umountUsbUx0() {
	sceAppMgrDestroyOtherApp();

	// Restore ux0: patch
	unredirectUx0();

	// Change to lowest priority
	sceKernelChangeThreadPriority(sceKernelGetThreadId(), 191);

	// Remount ux0:
	remount(0x800);

	// Trick
	char * const argv[] = { "umount", NULL };
	sceAppMgrLoadExec("app0:eboot.bin", argv, NULL);

	return 0;
}

SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type) {
	int res;

	// Load and start usbdevice module
	SceUID modid = taiLoadStartKernelModule(usbDevicePath, 0, NULL, 0);
	if (modid < 0)
		return modid;

	// Stop MTP driver
	res = sceMtpIfStopDriver(1);
	if (res < 0)
		return res;

	// Set device information
	res = sceUsbstorVStorSetDeviceInfo("\"PS Vita\" MC", "1.00");
	if (res < 0)
		return res;

	// Set image file path
	res = sceUsbstorVStorSetImgFilePath(imgFilePath);
	if (res < 0)
		return res;

	// Start USB storage
	res = sceUsbstorVStorStart(type);
	if (res < 0)
		return res;

	return modid;
}

int stopUsb(SceUID modid) {
	int res;

	// Stop USB storage
	res = sceUsbstorVStorStop();
	if (res < 0)
		return res;

	// Start MTP driver
	res = sceMtpIfStartDriver(1);
	if (res < 0)
		return res;

	// Stop and unload usbdevice module
	res = taiStopUnloadKernelModule(modid, 0, NULL, 0, NULL, NULL);
	if (res < 0)
		return res;

	// Change to lowest priority
	sceKernelChangeThreadPriority(sceKernelGetThreadId(), 191);

	// Remount
	remount(0x800);

	// Trick
	const char * const argv[] = { "restart", NULL };
	sceAppMgrLoadExec("app0:eboot.bin", NULL, NULL);

	return 0;
}