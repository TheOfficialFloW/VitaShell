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
#include "init.h"
#include "file.h"
#include "module.h"

extern unsigned char _binary_resources_battery_png_start;
extern unsigned char _binary_resources_battery_bar_red_png_start;
extern unsigned char _binary_resources_battery_bar_green_png_start;

vita2d_pgf *font = NULL;
char font_size_cache[256];

vita2d_texture *battery_image = NULL, *battery_bar_red_image = NULL, *battery_bar_green_image = NULL;

// Mount points
char pspemu_path[MAX_MOUNT_POINT_LENGTH], data_external_path[MAX_MOUNT_POINT_LENGTH], sys_external_path[MAX_MOUNT_POINT_LENGTH];
char mms_400_path[MAX_MOUNT_POINT_LENGTH], mms_401_path[MAX_MOUNT_POINT_LENGTH], mms_402_path[MAX_MOUNT_POINT_LENGTH];
char friends_path[MAX_MOUNT_POINT_LENGTH], messages_path[MAX_MOUNT_POINT_LENGTH], near_path[MAX_MOUNT_POINT_LENGTH], calendar_path[MAX_MOUNT_POINT_LENGTH];
char app_106_path[MAX_MOUNT_POINT_LENGTH], app_107_path[MAX_MOUNT_POINT_LENGTH];
char work_200_path[MAX_MOUNT_POINT_LENGTH], work_201_path[MAX_MOUNT_POINT_LENGTH], work_203_path[MAX_MOUNT_POINT_LENGTH], work_204_path[MAX_MOUNT_POINT_LENGTH], work_206_path[MAX_MOUNT_POINT_LENGTH];
char work_207_path[MAX_MOUNT_POINT_LENGTH];

// System params
int language = 0, enter_button = 0, date_format = 0, time_format = 0;

void initSceAppUtil() {
	// Init SceAppUtil
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

	// Mount
	sceAppUtilMusicMount();
	sceAppUtilPhotoMount();

	// System params
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, &language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, &enter_button);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_DATE_FORMAT, &date_format);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_TIME_FORMAT, &time_format);

	if (enter_button == SCE_SYSTEM_PARAM_ENTER_BUTTON_CIRCLE) {
		SCE_CTRL_ENTER = SCE_CTRL_CIRCLE;
		SCE_CTRL_CANCEL = SCE_CTRL_CROSS;
	}
}

void finishSceAppUtil() {
	// Unmount
	sceAppUtilPhotoUmount();
	sceAppUtilMusicUmount();

	// Shutdown AppUtil
	sceAppUtilShutdown();
}

void initVita2dLib() {
	vita2d_init();
	font = vita2d_load_default_pgf();

	// Font size cache
	int i;
	for (i = 0; i < 256; i++) {
		char character[2];
		character[0] = i;
		character[1] = '\0';

		font_size_cache[i] = vita2d_pgf_text_width(font, FONT_SIZE, character);
	}

	battery_image = vita2d_load_PNG_buffer(&_binary_resources_battery_png_start);
	battery_bar_red_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_red_png_start);
	battery_bar_green_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_green_png_start);
}

void finishVita2dLib() {
	vita2d_free_texture(battery_bar_green_image);
	vita2d_free_texture(battery_bar_red_image);
	vita2d_free_texture(battery_image);
	vita2d_free_pgf(font);
	vita2d_fini();
}

void addMountPoints() {
	// Get titleid
	char titleid[10];
	memset(titleid, 0, sizeof(titleid));
	sceAppMgrAppParamGetString(sceKernelGetProcessId(), 12, titleid, sizeof(titleid));

	// Add pspemu mountpoint
	memset(pspemu_path, 0, sizeof(pspemu_path));
	sceAppMgrPspSaveDataRootMount(pspemu_path);
	addMountPoint(pspemu_path);

	// Add vs0 mounts
	memset(data_external_path, 0, sizeof(data_external_path));
	sceAppMgrConvertVs0UserDrivePath("vs0:data/external", data_external_path, 1024);
	addMountPoint(data_external_path);

	memset(sys_external_path, 0, sizeof(sys_external_path));
	sceAppMgrConvertVs0UserDrivePath("vs0:sys/external", sys_external_path, 1024);
	addMountPoint(sys_external_path);
/*
	// Add mms photo mount
	memset(mms_400_path, 0, sizeof(mms_400_path));
	sceAppMgrMmsMount(400, mms_400_path);
	addMountPoint(mms_400_path);

	memset(mms_401_path, 0, sizeof(mms_401_path));
	sceAppMgrMmsMount(401, mms_401_path);
	addMountPoint(mms_401_path);

	memset(mms_402_path, 0, sizeof(mms_402_path));
	sceAppMgrMmsMount(402, mms_402_path);
	addMountPoint(mms_402_path);
*/
/*
	// Add app data mounts
	memset(friends_path, 0, sizeof(friends_path));
	sceAppMgrAppDataMount(101, friends_path);
	addMountPoint(friends_path);

	memset(messages_path, 0, sizeof(messages_path));
	sceAppMgrAppDataMount(102, messages_path);
	addMountPoint(messages_path);

	memset(near_path, 0, sizeof(near_path));
	sceAppMgrAppDataMount(103, near_path);
	addMountPoint(near_path);

	memset(calendar_path, 0, sizeof(calendar_path));
	sceAppMgrAppDataMount(108, calendar_path);
	addMountPoint(calendar_path);

	memset(app_106_path, 0, sizeof(app_106_path));
	sceAppMgrAppDataMountById(106, titleid, app_106_path);
	addMountPoint(app_106_path);

	memset(app_107_path, 0, sizeof(app_107_path));
	sceAppMgrAppDataMountById(107, titleid, app_107_path);
	addMountPoint(app_107_path);
*/
/*
	// Add work dir mounts
	memset(work_200_path, 0, sizeof(work_200_path));
	sceAppMgrWorkDirMount(200, work_200_path);
	addMountPoint(work_200_path);

	memset(work_201_path, 0, sizeof(work_201_path));
	sceAppMgrWorkDirMount(201, work_201_path);
	addMountPoint(work_201_path);

	memset(work_203_path, 0, sizeof(work_203_path));
	sceAppMgrWorkDirMount(203, work_203_path);
	addMountPoint(work_203_path);

	memset(work_204_path, 0, sizeof(work_204_path));
	sceAppMgrWorkDirMount(204, work_204_path);
	addMountPoint(work_204_path);

	memset(work_206_path, 0, sizeof(work_206_path));
	sceAppMgrWorkDirMount(206, work_206_path);
	addMountPoint(work_206_path);

	memset(work_207_path, 0, sizeof(work_207_path));
	sceAppMgrWorkDirMountById(207, titleid, work_207_path);
	addMountPoint(work_207_path);
*/
}

int findSceSysmoduleFunctions() {
	char modname[27];
	uint32_t text_addr = 0, text_size = 0;

	char path[MAX_PATH_LENGTH];
	sprintf(path, "%s%s", sys_external_path, "libcdlg.suprx");

	SceUID mod = sceKernelLoadModule(path, 0, NULL);
	if (mod < 0)
		return mod;

	if (getModuleInfo(mod, modname, &text_addr, &text_size) == 0)
		return -1;

	SceModuleInfo *mod_info = findModuleInfo(modname, text_addr, text_size);

	uint32_t _sceSysmoduleUnloadModuleInternalWithArgAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmodule", 0xA2F40C4C) - text_addr;
	uint32_t _sceSysmoduleLoadModuleInternalWithArgAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmodule", 0xC3C26339) - text_addr;
	uint32_t sceKernelGetMemBlockInfoByRangeAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmem", 0x006F3DB4) - text_addr;

	sceKernelUnloadModule(mod, 0, NULL);

	if (findModuleByName(modname, &text_addr, &text_size) == 0)
		return -2;

	copyStub((uint32_t)&_sceSysmoduleUnloadModuleInternalWithArg, (void *)text_addr + _sceSysmoduleUnloadModuleInternalWithArgAddr);
	copyStub((uint32_t)&_sceSysmoduleLoadModuleInternalWithArg, (void *)text_addr + _sceSysmoduleLoadModuleInternalWithArgAddr);
	copyStub((uint32_t)&sceKernelGetMemBlockInfoByRange, (void *)text_addr + sceKernelGetMemBlockInfoByRangeAddr);

	return 0;
}

int findScePafFunctions() {
	char modname[27];
	uint32_t text_addr = 0, text_size = 0;

	char path[MAX_PATH_LENGTH];
	sprintf(path, "%s%s", sys_external_path, "libpaf.suprx");

	SceUID mod = sceKernelLoadModule(path, 0, NULL);
	if (mod < 0)
		return mod;

	if (getModuleInfo(mod, modname, &text_addr, &text_size) == 0)
		return -1;

	SceModuleInfo *mod_info = findModuleInfo(modname, text_addr, text_size);

	uint32_t scePowerSetGpuClockFrequencyAddr = findModuleImportByInfo(mod_info, text_addr, "ScePower", 0x717DB06C) - text_addr;

	uint32_t sceKernelOpenMemBlockAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmem", 0x8EB8DFBB) - text_addr;
	uint32_t sceKernelCloseMemBlockAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmem", 0xB680E3A0) - text_addr;

	uint32_t sceKernelGetSystemSwVersionAddr = findModuleImportByInfo(mod_info, text_addr, "SceModulemgr", 0x5182E212) - text_addr;
	uint32_t sceKernelGetModelForCDialogAddr = findModuleImportByInfo(mod_info, text_addr, "SceSysmem", 0xA2CB322F) - text_addr;

	sceKernelUnloadModule(mod, 0, NULL);

	static uint32_t entry;

	static uint32_t argp[5];
	argp[0] = 0x00180000;
	argp[1] = 0x00000000;
	argp[2] = 0x00000000;
	argp[3] = 0x00000001;
	argp[4] = 0x00000000;

	_sceSysmoduleLoadModuleInternalWithArg(0x80000008, sizeof(argp), argp, &entry);

	if (findModuleByName(modname, &text_addr, &text_size) == 0)
		return -2;

	copyStub((uint32_t)&sceKernelOpenMemBlock, (void *)text_addr + sceKernelOpenMemBlockAddr);
	copyStub((uint32_t)&sceKernelCloseMemBlock, (void *)text_addr + sceKernelCloseMemBlockAddr);

	copyStub((uint32_t)&sceKernelGetSystemSwVersion, (void *)text_addr + sceKernelGetSystemSwVersionAddr);
	copyStub((uint32_t)&sceKernelGetModelForCDialog, (void *)text_addr + sceKernelGetModelForCDialogAddr);

	// Resolve syscalls
	int syscall = decode_arm_inst(*(uint32_t *)(text_addr + scePowerSetGpuClockFrequencyAddr), NULL);

	makeSyscallStub((uint32_t)&scePowerGetBatteryLifePercent, syscall - 10);

	_sceSysmoduleUnloadModuleInternalWithArg(0x80000008, 0, NULL, &entry);

	return 0;
}

int initSceLibPgf() {
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);

	uint32_t text_addr = 0, text_size = 0;
	findModuleByName("SceLibPgf", &text_addr, &text_size);

	SceModuleInfo *mod_info = findModuleInfo("SceLibPgf", text_addr, text_size);

	makeFunctionStub((uint32_t)&sceFontDoneLib, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0x07EE1733));
	makeFunctionStub((uint32_t)&sceFontNewLib, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0x1055ABA3));
	makeFunctionStub((uint32_t)&sceFontClose, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0x4A7293E9));
	makeFunctionStub((uint32_t)&sceFontGetCharInfo, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0x6FD1BA65));
	makeFunctionStub((uint32_t)&sceFontGetCharGlyphImage, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0xAB45AAD3));
	makeFunctionStub((uint32_t)&sceFontOpen, (void *)findModuleExportByInfo(mod_info, text_addr, "ScePgf", 0xBD2DFCFF));

	return 0;
}

void VitaShellInit() {
	// Init
	initSceLibPgf();
	initSceAppUtil();
	initVita2dLib();

	// Add mount points
	addMountPoints();

	// Find Sysmodule functions
	findSceSysmoduleFunctions();

	// Find ScePaf functions
	findScePafFunctions();

	// Init netdbg
	netdbg_init();
}