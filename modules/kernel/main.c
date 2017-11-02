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

#include <psp2kern/kernel/cpu.h>
#include <psp2kern/kernel/modulemgr.h>
#include <psp2kern/kernel/sysmem.h>
#include <psp2kern/io/fcntl.h>

#include <stdio.h>
#include <string.h>

#include <taihen.h>

#define MOUNT_POINT_ID 0x800

int module_get_offset(SceUID pid, SceUID modid, int segidx, size_t offset, uintptr_t *addr);

typedef struct {
  const char *dev;
  const char *dev2;
  const char *blkdev;
  const char *blkdev2;
  int id;
} SceIoDevice;

typedef struct {
  int id;
  const char *dev_unix;
  int unk;
  int dev_major;
  int dev_minor;
  const char *dev_filesystem;
  int unk2;
  SceIoDevice *dev;
  int unk3;
  SceIoDevice *dev2;
  int unk4;
  int unk5;
  int unk6;
  int unk7;
} SceIoMountPoint;

static SceIoDevice uma_ux0_dev = { "ux0:", "exfatux0", "sdstor0:uma-pp-act-a", "sdstor0:uma-lp-act-entire", MOUNT_POINT_ID };

static SceIoMountPoint *(* sceIoFindMountPoint)(int id) = NULL;

static SceIoDevice *ori_dev = NULL, *ori_dev2 = NULL;

static SceUID hookid = -1;

static tai_hook_ref_t ksceSysrootIsSafeModeRef;

static tai_hook_ref_t ksceSblAimgrIsDolceRef;

static int ksceSysrootIsSafeModePatched() {
  return 1;
}

static int ksceSblAimgrIsDolcePatched() {
  return 1;
}

int shellKernelIsUx0Redirected() {
  uint32_t state;
  ENTER_SYSCALL(state);

  SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
  if (!mount) {
    EXIT_SYSCALL(state);
    return -1;
  }

  if (mount->dev == &uma_ux0_dev && mount->dev2 == &uma_ux0_dev) {
    EXIT_SYSCALL(state);
    return 1;
  }

  EXIT_SYSCALL(state);
  return 0;
}

int shellKernelRedirectUx0() {
  uint32_t state;
  ENTER_SYSCALL(state);

  SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
  if (!mount) {
    EXIT_SYSCALL(state);
    return -1;
  }

  if (mount->dev != &uma_ux0_dev && mount->dev2 != &uma_ux0_dev) {
    ori_dev = mount->dev;
    ori_dev2 = mount->dev2;
  }

  mount->dev = &uma_ux0_dev;
  mount->dev2 = &uma_ux0_dev;

  EXIT_SYSCALL(state);
  return 0;
}

int shellKernelUnredirectUx0() {
  uint32_t state;
  ENTER_SYSCALL(state);

  SceIoMountPoint *mount = sceIoFindMountPoint(MOUNT_POINT_ID);
  if (!mount) {
    EXIT_SYSCALL(state);
    return -1;
  }

  if (ori_dev && ori_dev2) {
    mount->dev = ori_dev;
    mount->dev2 = ori_dev2;

    ori_dev = NULL;
    ori_dev2 = NULL;
  }

  EXIT_SYSCALL(state);
  return 0;
}

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  SceUID tmp1, tmp2;
  // Get tai module info
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceIofilemgr", &info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Get important function
  module_get_offset(KERNEL_PID, info.modid, 0, 0x138C1, (uintptr_t *)&sceIoFindMountPoint);

  // Fake safe mode so that SceUsbMass can be loaded
  tmp1 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceSysmem", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);
  if (tmp1 < 0)
    return SCE_KERNEL_START_SUCCESS;
  // this patch is only needed on handheld units
  tmp2 = taiHookFunctionExportForKernel(KERNEL_PID, &ksceSblAimgrIsDolceRef, "SceSysmem", 0xFD00C69A, 0x71608CA3, ksceSblAimgrIsDolcePatched);
  if (tmp2 < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Load SceUsbMass
  SceUID modid = ksceKernelLoadStartModule("ux0:VitaShell/module/umass.skprx", 0, NULL, 0, NULL, NULL);

  // Release patch
  taiHookReleaseForKernel(tmp1, ksceSysrootIsSafeModeRef);
  taiHookReleaseForKernel(tmp2, ksceSblAimgrIsDolceRef);

  // Check result
  if (modid < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Fake safe mode in SceUsbServ
  hookid = taiHookFunctionImportForKernel(KERNEL_PID, &ksceSysrootIsSafeModeRef, "SceUsbServ", 0x2ED7F97A, 0x834439A7, ksceSysrootIsSafeModePatched);

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hookid >= 0)
    taiHookReleaseForKernel(hookid, ksceSysrootIsSafeModeRef);

  return SCE_KERNEL_STOP_SUCCESS;
}