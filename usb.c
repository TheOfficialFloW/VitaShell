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

static int remount_thread(SceSize args, void *argp) {
	remount(0x800);
	return sceKernelExitDeleteThread(0);
}

void remountRelaunch(char * const argv[]) {
	SceUID thid = sceKernelCreateThread("remount_thread", (SceKernelThreadEntry)remount_thread, 0x40, 0x1000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, 0, NULL);
	
	sceAppMgrLoadExec("app0:eboot.bin", argv, NULL);
}

int mountUsbUx0() {
	// Destroy other apps
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
	sceIoMkdir("uma0:temp", 0006);
	sceIoMkdir("uma0:temp/app_work/", 0006);
	sceIoMkdir("uma0:temp/app_work/MLCL00001", 0006);
	sceIoMkdir("uma0:temp/app_work/MLCL00001/rec", 0006);

	// Copy important files
	copyPath("ux0:temp/app_work/MLCL00001/rec/config.bin", "uma0:temp/app_work/MLCL00001/rec/config.bin", NULL);
	copyPath("ux0:iconlayout.ini", "uma0:iconlayout.ini", NULL);
	copyPath("ux0:id.dat", "uma0:id.dat", NULL);

	// Remove lastdir.txt file
	sceIoRemove("uma0:VitaShell/internal/lastdir.txt");

	// Redirect ux0: to uma0:
	shellUserRedirectUx0();

	// Mount USB ux0:
	vshIoUmount(0xF00, 0, 0, 0);
	
	// Remount and relaunch
	char * const argv[] = { "mount", NULL };
	remountRelaunch(argv);

	return 0;
}

int umountUsbUx0() {
	// Destroy other apps
	sceAppMgrDestroyOtherApp();

	// Restore ux0: patch
	shellUserUnredirectUx0();

	// Remount and relaunch
	char * const argv[] = { "umount", NULL };
	remountRelaunch(argv);

	return 0;
}

SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type) {
	int res;

	// Destroy other apps
	sceAppMgrDestroyOtherApp();

	// Load and start usbdevice module
	SceUID modid = taiLoadStartKernelModule(usbDevicePath, 0, NULL, 0);
	if (modid < 0)
		goto ERROR_LOAD_MODULE;

	// Stop MTP driver
	res = sceMtpIfStopDriver(1);
	if (res < 0)
		goto ERROR_STOP_DRIVER;

	// Set device information
	res = sceUsbstorVStorSetDeviceInfo("\"PS Vita\" MC", "1.00");
	if (res < 0)
		goto ERROR_USBSTOR_VSTOR;

	// Set image file path
	res = sceUsbstorVStorSetImgFilePath(imgFilePath);
	if (res < 0)
		goto ERROR_USBSTOR_VSTOR;

	// Start USB storage
	res = sceUsbstorVStorStart(type);
	if (res < 0)
		goto ERROR_USBSTOR_VSTOR;

	return modid;

ERROR_USBSTOR_VSTOR:
	sceMtpIfStartDriver(1);

ERROR_STOP_DRIVER:
	taiStopUnloadKernelModule(modid, 0, NULL, 0, NULL, NULL);

ERROR_LOAD_MODULE:
	return res;
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

	// Remount and relaunch
	char * const argv[] = { "restart", NULL };
	remountRelaunch(argv);

	return 0;
}