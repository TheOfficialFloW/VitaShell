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

/*
	// both have 7 args
	int sceKernelOpenModule();
	int sceKernelCloseModule();
*/

typedef struct SceKernelMemBlockInfo {
	SceSize size;
	void *mappedBase;
	SceSize mappedSize;
	int memoryType;
	SceUInt32 access;
	SceKernelMemBlockType type;
} SceKernelMemBlockInfo;

#define SCE_KERNEL_MEMBLOCK_TYPE_USER_SYSTEM_RX 0x0320D050
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_SYSTEM_RW 0x0390D060
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_RX 0x0E20D050
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_SHARED_RW 0x0E20D060
#define SCE_KERNEL_MEMBLOCK_TYPE_USER_RX 0x0C20D050

/*
	Shared memory: 0xD0000000-0xD0900000
	System memory: 0xE0000000-?
*/

#define SCE_KERNEL_MEMORY_TYPE_NORMAL_NC 0x80
#define SCE_KERNEL_MEMORY_TYPE_NORMAL 0xD0

#define SCE_KERNEL_MEMORY_ACCESS_X 0x01
#define SCE_KERNEL_MEMORY_ACCESS_W 0x02
#define SCE_KERNEL_MEMORY_ACCESS_R 0x04

typedef struct {
	int size;
	char version_string[28];
	uint32_t version_value;
	uint32_t unk;
} SwVersionParam;

int sceGenSyscall();

void _init_vita_heap();
void _free_vita_heap();

int sceKernelGetProcessId();

int sceAppMgrLaunchAppByUri(int unk, char *uri);

// id: 100 (photo0), 101 (friends), 102 (messages), 103 (near), 105 (music), 108 (calendar)
int sceAppMgrAppDataMount(int id, char *mount_point);

// id: 106 (ad), 107 (ad)
int sceAppMgrAppDataMountById(int id, char *titleid, char *mount_point);

// param: 12 (titleid)
int sceAppMgrAppParamGetString(int pid, int param, char *string, int length);

int sceAppMgrConvertVs0UserDrivePath(char *path, char *mount_point, int unk);

// dev: ux0:
int sceAppMgrGetDevInfo(char *dev, uint64_t *max_size, uint64_t *free_size);

int sceAppMgrGetRawPath(char *path, char *mount_point, char *unk);

// id: 400 (ad), 401 (ad), 402 (ad)
int sceAppMgrMmsMount(int id, char *mount_point);

// ms
int sceAppMgrPspSaveDataRootMount(char *mount_point);

// id: 200 (td), 201 (td), 203 (td), 204 (td), 206 (td)
int sceAppMgrWorkDirMount(int id, char *mount_point);

// id: 205 (cache0), 207 (td)
int sceAppMgrWorkDirMountById(int id, char *titleid, char *mount_point);

// 41-444MHz, default: 333Mhz
int scePowerSetArmClockFrequency(int freq);
int scePowerGetArmClockFrequency();

// 41-222MHz, default: 222Mhz
int scePowerSetBusClockFrequency(int freq);
int scePowerGetBusClockFrequency();

// 41-166MHz, default: 111Mhz
int scePowerSetGpuClockFrequency(int freq);
int scePowerGetGpuClockFrequency();

int _sceSysmoduleUnloadModuleInternalWithArg(int id, int argc, void *argp, uint32_t *entry);
int _sceSysmoduleLoadModuleInternalWithArg(int id, int argc, void *argp, uint32_t *entry);
int sceKernelGetMemBlockInfoByRange(void *vbase, SceSize vsize, SceKernelMemBlockInfo *pInfo);

int sceKernelGetMemBlockInfoByAddr(void *vbase, SceKernelMemBlockInfo *pInfo);
int sceKernelOpenMemBlock(const char *name, int flags);
int sceKernelCloseMemBlock(SceUID uid);
/*
uint32_t param[4];
param[0] = sizeof(param);
int sceKernelGetFreeMemorySize(param);
*/
int sceKernelGetSystemSwVersion(SwVersionParam *param);
int sceKernelGetModelForCDialog();

#endif