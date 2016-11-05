
#ifndef __UI2_H__
#define __UI2_H__


#include <stdbool.h>
#include <psp2/appmgr.h>
#include <psp2/apputil.h>
#include <psp2/audioout.h>
#include <psp2/audiodec.h>
#include <psp2/ctrl.h>
#include <psp2/display.h>
#include <psp2/libssl.h>
#include <psp2/ime_dialog.h>
#include <psp2/message_dialog.h>
#include <psp2/moduleinfo.h>
#include <psp2/musicexport.h>
#include <psp2/photoexport.h>
#include <psp2/pgf.h>
#include <psp2/power.h>
#include <psp2/rtc.h>
#include <psp2/sysmodule.h>
#include <psp2/system_param.h>
#include <psp2/touch.h>
#include <psp2/types.h>
#include <psp2/kernel/modulemgr.h>
#include <psp2/kernel/processmgr.h>
#include <psp2/io/dirent.h>
#include <psp2/io/fcntl.h>
#include <psp2/net/http.h>
#include <psp2/net/net.h>
#include <psp2/net/netctl.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include <malloc.h>

#include <math.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <vita2d.h>
#include <ftpvita.h>

#include "file.h"

#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)
#define HEIGHT_TITLE_BAR 31.0f
#define FONT_Y_SPACE2 28.0f
#define SHELL_MARGIN_Y_CUSTOM 2.0f	
#define SHELL_MARGIN_X_CUSTOM 20.0f

enum Colors
{
    // Primary colors
    RED = 0xFF0000FF,
    GREEN = 0xFF00FF00,
    BLUE = 0xFFFF0000,
    // Secondary colors
    CYAN = 0xFFFFFF00,
    MAGENTA = 0xFFFF00FF,
    YELLOW = 0xFF00FFFF,
    // Tertiary colors
    AZURE = 0xFFFF7F00,
    VIOLET = 0xFFFF007F,
    ROSE = 0xFF7F00FF,
    ORANGE = 0xFF007FFF,
    CHARTREUSE = 0xFF00FF7F,
    SPRING_GREEN = 0xFF7FFF00,
    // Grayscale
    WHITE = 0xFFFFFFFF,
    LITEGRAY = 0xFFBFBFBF,
    GRAY = 0xFF7F7F7F,
    DARKGRAY = 0xFF3F3F3F,
    BLACK = 0xFF000000
};

extern vita2d_texture *headphone_image, *audio_previous_image, *audio_pause_image, *audio_play_image, *audio_next_image;
extern vita2d_texture *default_wallpaper, *game_card_storage_image, *game_card_image, *memory_card_image;
extern vita2d_texture *run_file_image, *img_file_image, *unknown_file_image, *sa0_image, *ur0_image, *vd0_image, *vs0_image;
extern vita2d_texture *savedata0_image, *folder_image, *pd0_image, *app0_image, *ud0_image, *mark_image, *music_image, *os0_image;
extern vita2d_texture *zip_file_image, *txt_file_image, *title_bar_bg_image, *updir_image ;

extern bool Change_UI;

extern int length_row_items;
extern float length_border;
extern float width_item;
extern float height_item;

int UI2();


#endif
