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

extern unsigned char _binary_resources_changeinfo_txt_start;
extern unsigned char _binary_resources_changeinfo_txt_size;

extern unsigned char _binary_resources_folder_icon_png_start;
extern unsigned char _binary_resources_folder_icon_png_size;
extern unsigned char _binary_resources_file_icon_png_start;
extern unsigned char _binary_resources_file_icon_png_size;
extern unsigned char _binary_resources_archive_icon_png_start;
extern unsigned char _binary_resources_archive_icon_png_size;
extern unsigned char _binary_resources_image_icon_png_start;
extern unsigned char _binary_resources_image_icon_png_size;
extern unsigned char _binary_resources_audio_icon_png_start;
extern unsigned char _binary_resources_audio_icon_png_size;
extern unsigned char _binary_resources_sfo_icon_png_start;
extern unsigned char _binary_resources_sfo_icon_png_size;
extern unsigned char _binary_resources_text_icon_png_start;
extern unsigned char _binary_resources_text_icon_png_size;
extern unsigned char _binary_resources_ftp_png_start;
extern unsigned char _binary_resources_ftp_png_size;
extern unsigned char _binary_resources_battery_png_start;
extern unsigned char _binary_resources_battery_png_size;
extern unsigned char _binary_resources_battery_bar_red_png_start;
extern unsigned char _binary_resources_battery_bar_red_png_size;
extern unsigned char _binary_resources_battery_bar_green_png_start;
extern unsigned char _binary_resources_battery_bar_green_png_size;
extern unsigned char _binary_resources_battery_bar_charge_png_start;
extern unsigned char _binary_resources_battery_bar_charge_png_size;
 
extern unsigned char _binary_resources_theme_txt_start;
extern unsigned char _binary_resources_theme_txt_size;

extern unsigned char _binary_resources_colors_txt_start;
extern unsigned char _binary_resources_colors_txt_size;

extern unsigned char _binary_resources_english_us_txt_start;
extern unsigned char _binary_resources_english_us_txt_size;

extern unsigned char _binary_resources_headphone_png_start;
extern unsigned char _binary_resources_audio_previous_png_start;
extern unsigned char _binary_resources_audio_pause_png_start;
extern unsigned char _binary_resources_audio_play_png_start;
extern unsigned char _binary_resources_audio_next_png_start;

static DefaultFile default_files[] = {
	{ "ux0:VitaShell/language/english_us.txt", (void *)&_binary_resources_english_us_txt_start, (int)&_binary_resources_english_us_txt_size },
	{ "ux0:VitaShell/theme/theme.txt", (void *)&_binary_resources_theme_txt_start, (int)&_binary_resources_theme_txt_size },
	{ "ux0:VitaShell/theme/Default/colors.txt", (void *)&_binary_resources_colors_txt_start, (int)&_binary_resources_colors_txt_size },
	{ "ux0:VitaShell/theme/Default/folder_icon.png", (void *)&_binary_resources_folder_icon_png_start, (int)&_binary_resources_folder_icon_png_size },
	{ "ux0:VitaShell/theme/Default/file_icon.png", (void *)&_binary_resources_file_icon_png_start, (int)&_binary_resources_file_icon_png_size },
	{ "ux0:VitaShell/theme/Default/archive_icon.png", (void *)&_binary_resources_archive_icon_png_start, (int)&_binary_resources_archive_icon_png_size },
	{ "ux0:VitaShell/theme/Default/image_icon.png", (void *)&_binary_resources_image_icon_png_start, (int)&_binary_resources_image_icon_png_size },
	{ "ux0:VitaShell/theme/Default/audio_icon.png", (void *)&_binary_resources_audio_icon_png_start, (int)&_binary_resources_audio_icon_png_size },
	{ "ux0:VitaShell/theme/Default/sfo_icon.png", (void *)&_binary_resources_sfo_icon_png_start, (int)&_binary_resources_sfo_icon_png_size },
	{ "ux0:VitaShell/theme/Default/text_icon.png", (void *)&_binary_resources_text_icon_png_start, (int)&_binary_resources_text_icon_png_size },
	{ "ux0:VitaShell/theme/Default/ftp.png", (void *)&_binary_resources_ftp_png_start, (int)&_binary_resources_ftp_png_size },
	{ "ux0:VitaShell/theme/Default/battery.png", (void *)&_binary_resources_battery_png_start, (int)&_binary_resources_battery_png_size },
	{ "ux0:VitaShell/theme/Default/battery_bar_red.png", (void *)&_binary_resources_battery_bar_red_png_start, (int)&_binary_resources_battery_bar_red_png_size },
	{ "ux0:VitaShell/theme/Default/battery_bar_green.png", (void *)&_binary_resources_battery_bar_green_png_start, (int)&_binary_resources_battery_bar_green_png_size },
	{ "ux0:VitaShell/theme/Default/battery_bar_charge.png", (void *)&_binary_resources_battery_bar_charge_png_start, (int)&_binary_resources_battery_bar_charge_png_size },
};

vita2d_pgf *font = NULL;
char font_size_cache[256];

vita2d_texture *headphone_image = NULL, *audio_previous_image = NULL, *audio_pause_image = NULL, *audio_play_image = NULL, *audio_next_image = NULL;

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

	headphone_image = vita2d_load_PNG_buffer(&_binary_resources_headphone_png_start);
	audio_previous_image = vita2d_load_PNG_buffer(&_binary_resources_audio_previous_png_start);
	audio_pause_image = vita2d_load_PNG_buffer(&_binary_resources_audio_pause_png_start);
	audio_play_image = vita2d_load_PNG_buffer(&_binary_resources_audio_play_png_start);
	audio_next_image = vita2d_load_PNG_buffer(&_binary_resources_audio_next_png_start);
}

void finishVita2dLib() {
	vita2d_free_texture(headphone_image);
	vita2d_free_texture(audio_previous_image);
	vita2d_free_texture(audio_pause_image);
	vita2d_free_texture(audio_play_image);
	vita2d_free_texture(audio_next_image);
	vita2d_free_pgf(font);
	vita2d_fini();

	font = NULL;
	headphone_image = NULL;
	audio_previous_image = NULL;
	audio_pause_image = NULL;
	audio_play_image = NULL;
	audio_next_image = NULL;
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
	if (sceIoGetstat("ux0:patch/VITASHELL/sce_sys/changeinfo/changeinfo.xml", &stat) < 0 && (int)stat.st_size != (int)&_binary_resources_changeinfo_txt_size) {
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
