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
#include "utils.h"

INCLUDE_EXTERN_RESOURCE(changeinfo_txt);

INCLUDE_EXTERN_RESOURCE(folder_icon_png);
INCLUDE_EXTERN_RESOURCE(file_icon_png);
INCLUDE_EXTERN_RESOURCE(archive_icon_png);
INCLUDE_EXTERN_RESOURCE(image_icon_png);
INCLUDE_EXTERN_RESOURCE(audio_icon_png);
INCLUDE_EXTERN_RESOURCE(sfo_icon_png);
INCLUDE_EXTERN_RESOURCE(text_icon_png);
INCLUDE_EXTERN_RESOURCE(ftp_png);
INCLUDE_EXTERN_RESOURCE(battery_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_red_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_green_png);
INCLUDE_EXTERN_RESOURCE(battery_bar_charge_png);

INCLUDE_EXTERN_RESOURCE(cover_png);
INCLUDE_EXTERN_RESOURCE(play_png);
INCLUDE_EXTERN_RESOURCE(pause_png);
INCLUDE_EXTERN_RESOURCE(fastforward_png);
INCLUDE_EXTERN_RESOURCE(fastrewind_png);

INCLUDE_EXTERN_RESOURCE(theme_txt);
INCLUDE_EXTERN_RESOURCE(colors_txt);
INCLUDE_EXTERN_RESOURCE(english_us_txt);

#define DEFAULT_FILE(path, name) { path, (void *)&_binary_resources_##name##_start, (int)&_binary_resources_##name##_size }

static DefaultFile default_files[] = {
	DEFAULT_FILE("ux0:VitaShell/language/english_us.txt", english_us_txt),

	DEFAULT_FILE("ux0:VitaShell/theme/theme.txt", theme_txt),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/colors.txt", colors_txt),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/folder_icon.png", folder_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/file_icon.png", file_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/archive_icon.png", archive_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/image_icon.png", image_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/audio_icon.png", audio_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/sfo_icon.png", sfo_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/text_icon.png", text_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/ftp.png", ftp_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/battery.png", battery_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/battery_bar_red.png", battery_bar_red_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/battery_bar_green.png", battery_bar_green_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/battery_bar_charge.png", battery_bar_charge_png),

	DEFAULT_FILE("ux0:VitaShell/theme/Default/cover.png", cover_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/play.png", play_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/pause.png", pause_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/fastforward.png", fastforward_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/fastrewind.png", fastrewind_png),
};

vita2d_pgf *font = NULL;
char font_size_cache[256];

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

	// Set common dialog config
	SceCommonDialogConfigParam config;
	sceCommonDialogConfigParamInit(&config);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_LANG, (int *)&config.language);
	sceAppUtilSystemParamGetInt(SCE_SYSTEM_PARAM_ID_ENTER_BUTTON, (int *)&config.enterButtonAssign);
	sceCommonDialogSetConfigParam(&config);
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
}

void finishVita2dLib() {
	vita2d_free_pgf(font);
	vita2d_fini();

	font = NULL;
}

void initNet() {
	static char memory[16 * 1024];

	SceNetInitParam param;
	param.memory = memory;
	param.size = sizeof(memory);
	param.flags = 0;

	sceNetInit(&param);
	sceNetCtlInit();

	sceSslInit(300 * 1024);
	sceHttpInit(40 * 1024);
}

void finishNet() {
	sceSslTerm();
	sceHttpTerm();
	sceNetCtlTerm();
	sceNetTerm();	
}

void initVitaShell() {
	// Init random number generator
	srand(time(NULL));

	// Set sampling mode
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	// Enable front touchscreen
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

	// Load modules
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_HTTPS) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_HTTPS);

	// Init
	initSceAppUtil();
	initVita2dLib();
	initNet();

	// Init power tick thread
	initPowerTickThread();

	// Make VitaShell folders
	sceIoMkdir("ux0:VitaShell", 0777);
	sceIoMkdir("ux0:VitaShell/internal", 0777);
	sceIoMkdir("ux0:VitaShell/language", 0777);
	sceIoMkdir("ux0:VitaShell/theme", 0777);
	sceIoMkdir("ux0:VitaShell/theme/Default", 0777);

	// Write default files if they don't exist
	int i;
	for (i = 0; i < (sizeof(default_files) / sizeof(DefaultFile)); i++) {
		SceIoStat stat;
		memset(&stat, 0, sizeof(stat));
		if (sceIoGetstat(default_files[i].path, &stat) < 0)
			WriteFile(default_files[i].path, default_files[i].buffer, default_files[i].size);
	}

	// Write changeinfo.xml file to patch
	SceIoStat stat;
	memset(&stat, 0, sizeof(stat));
	if (sceIoGetstat("ux0:patch/VITASHELL/sce_sys/changeinfo/changeinfo.xml", &stat) < 0 || (int)stat.st_size != (int)&_binary_resources_changeinfo_txt_size) {
		sceIoMkdir("ux0:patch", 0777);
		sceIoMkdir("ux0:patch/VITASHELL", 0777);
		sceIoMkdir("ux0:patch/VITASHELL/sce_sys", 0777);
		sceIoMkdir("ux0:patch/VITASHELL/sce_sys/changeinfo", 0777);
		WriteFile("ux0:patch/VITASHELL/sce_sys/changeinfo/changeinfo.xml", (void *)&_binary_resources_changeinfo_txt_start, (int)&_binary_resources_changeinfo_txt_size);
	}
}

void finishVitaShell() {
	// Finish
	finishNet();
	finishVita2dLib();
	finishSceAppUtil();
	
	// Unload modules
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_HTTPS) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_HTTPS);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_PGF);
}
