/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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

int mountGamecardUx0() {
  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  // Copy VitaShell
  copyPath("ux0:app/VITASHELL", "ur0:temp/app", NULL);
  copyPath("ux0:appmeta/VITASHELL", "ur0:temp/appmeta", NULL);
  copyPath("ux0:license/app/VITASHELL", "ur0:temp/license", NULL);

  // Redirect ux0: to gamecard
  shellUserRedirectUx0("sdstor0:gcd-lp-ign-entire", "sdstor0:gcd-lp-ign-entire");

  // Remount Memory Card
  remount(0x800);

  // Create dirs
  sceIoMkdir("ux0:app", 0006);
  sceIoMkdir("ux0:appmeta", 0006);
  sceIoMkdir("ux0:license", 0006);
  sceIoMkdir("ux0:license/app", 0006);
  sceIoMkdir("ux0:app/VITASHELL", 0006);
  sceIoMkdir("ux0:appmeta/VITASHELL", 0006);
  sceIoMkdir("ux0:license/app/VITASHELL", 0006);

  // Create important dirs
  sceIoMkdir("ux0:data", 0777);
  sceIoMkdir("ux0:temp", 0006);

  // Remove lastdir.txt file
  sceIoRemove("ux0:VitaShell/internal/lastdir.txt");

  // Copy VitaShell
  copyPath("ur0:temp/app", "ux0:app/VITASHELL", NULL);
  copyPath("ur0:temp/appmeta", "ux0:appmeta/VITASHELL", NULL);
  copyPath("ur0:temp/license", "ux0:license/app/VITASHELL", NULL);

  return 0;
}

int umountGamecardUx0() {
  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  // Restore ux0: patch
  if (checkFileExist("sdstor0:xmc-lp-ign-userext"))
    shellUserRedirectUx0("sdstor0:xmc-lp-ign-userext", "sdstor0:xmc-lp-ign-userext");
  else
    shellUserRedirectUx0("sdstor0:int-lp-ign-userext", "sdstor0:int-lp-ign-userext");

  // Remount Memory Card
  remount(0x800);

  return 0;
}

int mountUsbUx0() {
  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  // Create dirs
  sceIoMkdir("uma0:app", 0006);
  sceIoMkdir("uma0:appmeta", 0006);
  sceIoMkdir("uma0:license", 0006);
  sceIoMkdir("uma0:license/app", 0006);
  sceIoMkdir("uma0:app/VITASHELL", 0006);
  sceIoMkdir("uma0:appmeta/VITASHELL", 0006);
  sceIoMkdir("uma0:license/app/VITASHELL", 0006);

  // Copy VitaShell
  copyPath("ux0:app/VITASHELL", "uma0:app/VITASHELL", NULL);
  copyPath("ux0:appmeta/VITASHELL", "uma0:appmeta/VITASHELL", NULL);
  copyPath("ux0:license/app/VITASHELL", "uma0:license/app/VITASHELL", NULL);

  // Create important dirs
  sceIoMkdir("uma0:data", 0777);
  sceIoMkdir("uma0:temp", 0006);

  // Remove lastdir.txt file
  sceIoRemove("uma0:VitaShell/internal/lastdir.txt");

  // Redirect ux0: to uma0:
  shellUserRedirectUx0("sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire");

  // Umount uma0:
  vshIoUmount(0xF00, 0, 0, 0);

  // Remount Memory Card
  remount(0x800);

  return 0;
}

int umountUsbUx0() {
  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  // Restore ux0: patch
  if (checkFileExist("sdstor0:xmc-lp-ign-userext"))
    shellUserRedirectUx0("sdstor0:xmc-lp-ign-userext", "sdstor0:xmc-lp-ign-userext");
  else
    shellUserRedirectUx0("sdstor0:int-lp-ign-userext", "sdstor0:int-lp-ign-userext");

  // Remount Memory Card
  remount(0x800);

  // Remount uma0:
  remount(0xF00);

  return 0;
}

SceUID startUsb(const char *usbDevicePath, const char *imgFilePath, int type) {
  SceUID modid = -1;
  int res;

  // Destroy other apps
  sceAppMgrDestroyOtherApp();

  // Load and start usbdevice module
  res = taiLoadStartKernelModule(usbDevicePath, 0, NULL, 0);
  if (res < 0)
    goto ERROR_LOAD_MODULE;

  modid = res;

  // Stop MTP driver
  res = sceMtpIfStopDriver(1);
  if (res < 0 && res != 0x8054360C)
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

  // Remount Memory Card
  remount(0x800);

  // Remount imc0:
  if (checkFolderExist("imc0:"))
    remount(0xD00);

  // Remount xmc0:
  if (checkFolderExist("xmc0:"))
    remount(0xE00);

  // Remount uma0:
  if (checkFolderExist("uma0:"))
    remount(0xF00);

  return 0;
}
