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

extern unsigned char _binary_resources_ftp_png_start;
extern unsigned char _binary_resources_battery_png_start;
extern unsigned char _binary_resources_battery_bar_red_png_start;
extern unsigned char _binary_resources_battery_bar_green_png_start;

vita2d_pgf *font = NULL;
char font_size_cache[256];

vita2d_texture *ftp_image = NULL, *battery_image = NULL, *battery_bar_red_image = NULL, *battery_bar_green_image = NULL;

// System params
int language = 0, enter_button = 0, date_format = 0, time_format = 0;

void initSceAppUtil() {
	// Init SceAppUtil
	SceAppUtilInitParam init_param;
	SceAppUtilBootParam boot_param;
	memset(&init_param, 0, sizeof(SceAppUtilInitParam));
	memset(&boot_param, 0, sizeof(SceAppUtilBootParam));
	sceAppUtilInit(&init_param, &boot_param);

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

	ftp_image = vita2d_load_PNG_buffer(&_binary_resources_ftp_png_start);
	battery_image = vita2d_load_PNG_buffer(&_binary_resources_battery_png_start);
	battery_bar_red_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_red_png_start);
	battery_bar_green_image = vita2d_load_PNG_buffer(&_binary_resources_battery_bar_green_png_start);
}

void finishVita2dLib() {
	vita2d_free_texture(battery_bar_green_image);
	vita2d_free_texture(battery_bar_red_image);
	vita2d_free_texture(battery_image);
	vita2d_free_texture(ftp_image);
	vita2d_free_pgf(font);
	vita2d_fini();

	battery_bar_green_image = NULL;
	battery_bar_red_image = NULL;
	battery_image = NULL;
	font = NULL;
}

void initVitaShell() {
	// Set sampling mode
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	// Load modules
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_NET);

	// Init
	initSceAppUtil();
	initVita2dLib();

	// Init netdbg
	netdbg_init();
}

void finishVitaShell() {
	// Finish netdbg
	netdbg_fini();

	// Finish
	finishVita2dLib();
	finishSceAppUtil();
	
	// Unload modules
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_NET) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_NET);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_PGF);
}