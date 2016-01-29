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

#ifndef __FUNCTIONS_H__
#define __FUNCTIONS_H__

#define SCE_ERROR_ERRNO_EEXIST 0x80010011

typedef struct {
	int size;
	char version_string[28];
	uint32_t version_value;
	uint32_t unk;
} SwVersionParam;

int sceGenSyscall();

void _init_vita_newlib();
void _free_vita_newlib();

int sceKernelGetProcessId();

int sceKernelCreateLwMutex(void *work, const char *name, SceUInt attr, int initCount, void *option);
int sceKernelDeleteLwMutex(void *work);
int sceKernelUnlockLwMutex(void *work, int lockCount);

// 41-444MHz, default: 333Mhz
int scePowerSetArmClockFrequency(int freq);
int scePowerGetArmClockFrequency();

// 41-222MHz, default: 222Mhz
int scePowerSetBusClockFrequency(int freq);
int scePowerGetBusClockFrequency();

// 41-166MHz, default: 111Mhz
int scePowerSetGpuClockFrequency(int freq);
int scePowerGetGpuClockFrequency();

int _sceSysmoduleUnloadModuleInternalWithArg(int id, SceSize args, void *argp, uint32_t *entry);
int _sceSysmoduleLoadModuleInternalWithArg(int id, SceSize args, void *argp, uint32_t *entry);

/*
uint32_t param[4];
param[0] = sizeof(param);
int sceKernelGetFreeMemorySize(param);
*/
int sceKernelGetSystemSwVersion(SwVersionParam *param);
int sceKernelGetModelForCDialog();

#endif
