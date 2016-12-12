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
#include "package_installer.h"
#include "utils.h"

INCLUDE_EXTERN_RESOURCE(changeinfo_txt);
/*
INCLUDE_EXTERN_RESOURCE(vita_game_card_png);
INCLUDE_EXTERN_RESOURCE(vita_game_card_storage_png);
INCLUDE_EXTERN_RESOURCE(memory_card_png);
INCLUDE_EXTERN_RESOURCE(os0_png);
INCLUDE_EXTERN_RESOURCE(sa0_png);
INCLUDE_EXTERN_RESOURCE(ur0_png);
INCLUDE_EXTERN_RESOURCE(vd0_png);
INCLUDE_EXTERN_RESOURCE(vs0_png);
INCLUDE_EXTERN_RESOURCE(savedata0_png);
INCLUDE_EXTERN_RESOURCE(pd0_png);
INCLUDE_EXTERN_RESOURCE(app0_png);
INCLUDE_EXTERN_RESOURCE(ud0_png);

INCLUDE_EXTERN_RESOURCE(bg_wallpaper_png);
INCLUDE_EXTERN_RESOURCE(folder_png);
INCLUDE_EXTERN_RESOURCE(mark_png);
INCLUDE_EXTERN_RESOURCE(run_file_png);
INCLUDE_EXTERN_RESOURCE(image_file_png);
INCLUDE_EXTERN_RESOURCE(unknown_file_png);
INCLUDE_EXTERN_RESOURCE(music_file_png);
INCLUDE_EXTERN_RESOURCE(zip_file_png);
INCLUDE_EXTERN_RESOURCE(txt_file_png);
INCLUDE_EXTERN_RESOURCE(music_file_png);
INCLUDE_EXTERN_RESOURCE(title_bar_bg_png);
INCLUDE_EXTERN_RESOURCE(updir_png);
*/
INCLUDE_EXTERN_RESOURCE(folder_icon_png);
INCLUDE_EXTERN_RESOURCE(file_icon_png);
INCLUDE_EXTERN_RESOURCE(archive_icon_png);
INCLUDE_EXTERN_RESOURCE(image_icon_png);
INCLUDE_EXTERN_RESOURCE(audio_icon_png);
INCLUDE_EXTERN_RESOURCE(sfo_icon_png);
INCLUDE_EXTERN_RESOURCE(text_icon_png);
INCLUDE_EXTERN_RESOURCE(wifi_png);
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
/*
	DEFAULT_FILE("ux0:VitaShell/theme/Default/vita_game_card.png", vita_game_card_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/vita_game_card_storage.png", vita_game_card_storage_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/memory_card.png", memory_card_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/os0.png", os0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/sa0.png", sa0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/ur0.png", ur0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/vd0.png", vd0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/vs0.png", vs0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/savedata0.png", savedata0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/pd0.png", pd0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/app0.png", app0_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/ud0.png", ud0_png),

	DEFAULT_FILE("ux0:VitaShell/theme/Default/bg_wallpaper.png", bg_wallpaper_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/folder.png", folder_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/mark.png", mark_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/run_file.png", run_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/image_file.png", image_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/unknown_file.png", unknown_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/music_file.png", music_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/zip_file.png", zip_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/txt_file.png", txt_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/music_file.png", music_file_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/title_bar_bg.png", title_bar_bg_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/updir.png", updir_png),
*/
	DEFAULT_FILE("ux0:VitaShell/theme/theme.txt", theme_txt),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/colors.txt", colors_txt),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/folder_icon.png", folder_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/file_icon.png", file_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/archive_icon.png", archive_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/image_icon.png", image_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/audio_icon.png", audio_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/sfo_icon.png", sfo_icon_png),
	DEFAULT_FILE("ux0:VitaShell/theme/Default/text_icon.png", text_icon_png),
	// DEFAULT_FILE("ux0:VitaShell/theme/Default/wifi.png", wifi_png),
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

int isKoreanChar(unsigned int c) {
    unsigned short ch = c;

    // hangul compatibility jamo block
    if (0x3130 <= ch && ch <= 0x318f) {
        return 1;
    }

    // hangul syllables block
    if (0xac00 <= ch && ch <= 0xd7af) {
        return 1;
    }

    // korean won sign
    if (ch == 0xffe6) {
        return 1;
    }

    return 0;
}

int isLatinChar(unsigned int c) {
    unsigned short ch = c;

    // basic latin block + latin-1 supplement block
    if (ch <= 0x00ff) {
        return 1;
    }

    // cyrillic block
    if (0x0400 <= ch && ch <= 0x04ff) {
        return 1;
    }

    return 0;
}

vita2d_pgf *loadSystemFonts() {
    vita2d_system_pgf_config configs[] = {
        { SCE_FONT_LANGUAGE_KOREAN, isKoreanChar },
        { SCE_FONT_LANGUAGE_LATIN, isLatinChar },
        { SCE_FONT_LANGUAGE_DEFAULT, NULL },
    };

    return vita2d_load_system_pgf(3, configs);
}

void initVita2dLib() {
	vita2d_init();
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
	SceIoStat stat;

	// Init random number generator
	srand(time(NULL));

	// Set sampling mode
	sceCtrlSetSamplingMode(SCE_CTRL_MODE_ANALOG);

	// Enable front touchscreen
	sceTouchSetSamplingState(SCE_TOUCH_PORT_FRONT, 1);

	// Enable back touchscreen
	sceTouchSetSamplingState(SCE_TOUCH_PORT_BACK, 1);
	
	// Load modules
	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_PGF);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PHOTO_EXPORT) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_PHOTO_EXPORT);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_MUSIC_EXPORT) != SCE_SYSMODULE_LOADED)
		sceSysmoduleLoadModule(SCE_SYSMODULE_MUSIC_EXPORT);

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
	memset(&stat, 0, sizeof(stat));
	if (sceIoGetstat("ux0:patch/VITASHELL/sce_sys/changeinfo/changeinfo.xml", &stat) < 0 || (int)stat.st_size != (int)&_binary_resources_changeinfo_txt_size) {
		sceIoMkdir("ux0:patch", 0777);
		sceIoMkdir("ux0:patch/VITASHELL", 0777);
		sceIoMkdir("ux0:patch/VITASHELL/sce_sys", 0777);
		sceIoMkdir("ux0:patch/VITASHELL/sce_sys/changeinfo", 0777);
		WriteFile("ux0:patch/VITASHELL/sce_sys/changeinfo/changeinfo.xml", (void *)&_binary_resources_changeinfo_txt_start, (int)&_binary_resources_changeinfo_txt_size);
	}

	// Delete VitaShell updater if available
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat("ux0:app/VSUPDATER", &stat) >= 0) {
		deleteApp("VSUPDATER");
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

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PHOTO_EXPORT) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_PHOTO_EXPORT);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_MUSIC_EXPORT) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_MUSIC_EXPORT);

	if (sceSysmoduleIsLoaded(SCE_SYSMODULE_PGF) == SCE_SYSMODULE_LOADED)
		sceSysmoduleUnloadModule(SCE_SYSMODULE_PGF);
}
