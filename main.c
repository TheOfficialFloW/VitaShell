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

/*
	TODO:
	- Hide mount points
	- Network update	
	- Context menu: 'More' entry
	- Inverse sort, sort by date, size
	- vita2dlib: Handle big images > 4096
	- Page skip for text viewer
	- Hex editor byte group size
	- Moving destination folder to subfolder of source folder prevention
	- Moving a folder to a location where the folder does already exit causes error, so move its content.
	- Duplicate when same location or same name. /lol to /lol - Backup. or overwrite question.
	- Shortcuts
	- CPU changement
	- Media player
*/

#include "main.h"
#include "init.h"
#include "io_process.h"
#include "package_installer.h"
#include "archive.h"
#include "photo.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "audioplayer.h"

int _newlib_heap_size_user = 32 * 1024 * 1024;

#define MAX_DIR_LEVELS 1024
#define lerp(value, from_max, to_max) ((((value*10) * (to_max*10))/(from_max*10))/10)

// File lists
static FileList file_list, mark_list, copy_list;

// Paths
static char cur_file[MAX_PATH_LENGTH], archive_path[MAX_PATH_LENGTH];

// Position
static int base_pos = 0, rel_pos = 0;
static int base_pos_list[MAX_DIR_LEVELS];
static int rel_pos_list[MAX_DIR_LEVELS];
static int dir_level = 0;

// Copy mode
static int copy_mode = COPY_MODE_NORMAL;

// Archive
static int is_in_archive = 0;
static int dir_level_archive = -1;

// Context menu
static int ctx_menu_mode = CONTEXT_MENU_CLOSED;
static int ctx_menu_pos = -1;
static float ctx_menu_width = 0;
static float ctx_menu_max_width = 0;

// Net info
static SceNetEtherAddr mac;
static char ip[16];

// FTP
static char vita_ip[16];
static unsigned short int vita_port;

// Enter and cancel buttons
int SCE_CTRL_ENTER = SCE_CTRL_CROSS, SCE_CTRL_CANCEL = SCE_CTRL_CIRCLE;

// Dialog step
int dialog_step = DIALOG_STEP_NONE;

// Use custom config
int use_custom_config = 1;

float width_item = 150.0f;
int height_item = 170.0f;
int length_row = 6;
float length_border = 5.0f;

float slide_value = 0;
float slide_value_limit = 0;
float slide_value_hold = 0;
float speed_slide = 10.0f;
int value_cur_pos = 0;
int x_rel_pos_old = 0;
int y_rel_pos_old = 0;
bool animate = false;
clock_t time_on_touch_start = 0;
int Position = 0;

bool touch_press = false;
SceTouchData touch;	
int countTouch = 0;
int pre_touch_y = 0;
int pre_touch_x = 0;
bool touch_nothing = false;

// State_Touch is global value to certain function TOUCH_FRONT alway return absolute one value for one time.
int State_Touch = -1;
bool saved_loc = false;
bool have_slide = false;
bool have_touch_hold = false;
bool have_touch = true;
int MIN_SLIDE_VERTTICAL = 40.0f;
int MIN_SLIDE_HORIZONTAL = 100.0f;
int AREA_TOUCH = 40.0f;
bool disable_touch = false;


// Output:
//	0 : Touch down select
//	1 : Touch up select
//	2 : Double touch (temporary disable)
//  3 : Hold Touch
//  4 : Empty slot
//	5 : Empty slot
//	6 : Touch slide up
//	7 : Touch slide down
//  8 : Touch slide left
//  9 : Touch slide right
int TOUCH_FRONT() {	
	sceTouchPeek(0, &touch, 1);
		
	if(touch_press & (touch.reportNum == 0) ) {
		saved_loc = false;			
		time_on_touch_start = 0;	
		//have_touch_hold = false;

		//Touch up Select Event
		//prevent event slide
		if (!have_slide) {			
			State_Touch = 1;					

		//Touch Double Event		
		//prevent event slide
		//if (!have_slide) 
			//if ((touch.report[0].x < pre_touch_x + AREA_TOUCH) & (touch.report[0].x > pre_touch_x - AREA_TOUCH) & (touch.report[0].y < pre_touch_y + AREA_TOUCH) & (touch.report[0].y > pre_touch_y - AREA_TOUCH)) {
				//countTouch++;			
			//}						
		}
		else {
			touch_nothing = false;			
		}


		if (countTouch >= 2) {			
			countTouch = 0;
			//State_Touch = 2;			
		}
				
		//reset listen event slide
		have_slide = false;	
	}

	if (touch.reportNum > 0) {
		touch_press = true;		
		
		if (!saved_loc) {
			if (touch.report[0].x > pre_touch_x + AREA_TOUCH || touch.report[0].x < pre_touch_x - AREA_TOUCH) {			 
				pre_touch_x = touch.report[0].x;
				countTouch = 0;
			}
			if (touch.report[0].y > pre_touch_y + AREA_TOUCH || touch.report[0].y < pre_touch_y - AREA_TOUCH) {
				pre_touch_y = touch.report[0].y;				
				countTouch = 0;
			}
			time_on_touch_start = clock();
			saved_loc = true;
			slide_value_hold = slide_value;

			//Touch down Select Event				
			State_Touch = 0;					
			
		}

		//Touch Slide Up Event
		if (touch.report[0].y < pre_touch_y - MIN_SLIDE_VERTTICAL) {			
			State_Touch = 6;							
			have_slide = true;									
			return State_Touch;
		}

		//Touch Slide Down Event
		if (touch.report[0].y > pre_touch_y + MIN_SLIDE_VERTTICAL) {			
			State_Touch = 7;												
			have_slide = true;				
			return State_Touch;
		}

		//Touch Slide Left Event
		if (touch.report[0].x < pre_touch_x - MIN_SLIDE_HORIZONTAL) {			
			State_Touch = 8;						
			have_slide = true;									
			return State_Touch;
		}

		//Touch Slide Right Event
		if (touch.report[0].x > pre_touch_x + MIN_SLIDE_HORIZONTAL) {			
			State_Touch = 9;										
			have_slide = true;				
			return State_Touch;
		}

		if (!have_slide) {												
			if (((float)(clock() - time_on_touch_start)/CLOCKS_PER_SEC  > 0.4f) & (time_on_touch_start > 0)) {
				if ((touch.report[0].x < pre_touch_x + AREA_TOUCH) & (touch.report[0].x > pre_touch_x - AREA_TOUCH) & (touch.report[0].y < pre_touch_y + AREA_TOUCH) & (touch.report[0].y > pre_touch_y - AREA_TOUCH)) {
					State_Touch = 3;							
					return State_Touch;			
				}
			}																
		}
	}
	else touch_press = false;

	return State_Touch;
}


float animate_border = 1;
bool reverse = false;
void DrawBorderRect(int x, int y, int w, int h, unsigned int color, int width_glow, bool enable_animate, bool half) {
	int i = 0;
	int a = 255;	
	int count = 0;
	if (enable_animate) {
	if (reverse) {
		if (animate_border <= 0.95) {			
				animate_border += 0.005f;
		}
		else reverse = false;
	}
	else {
		animate_border -= 0.01f;
		if (animate_border <= 0.51f)
			reverse = true;
	}
	}

	if (half) {
		for (i = 0; i <= width_glow; i ++) {		
			if  (i == 0) 
				count = 1;
			else 
				count = pow(2, i * animate_border); 
		

		vita2d_draw_line(x - i, y - i, x + w + i, y - i, COLOR_ALPHA(color, a/count));	
		vita2d_draw_line(x + w + i, y - i, x + w + i - 1, y + h + i - 1, COLOR_ALPHA(color, a/count));		
		vita2d_draw_line(x + w + i, y + h + i, x - i , y + h + i, COLOR_ALPHA(color, a/count));			
		vita2d_draw_line(x - i, y + h + i, x - i - 1, y - i - 1, COLOR_ALPHA(color, a/count));	
					
		}
	}
	else {
		for (i = - width_glow; i <= width_glow; i ++) {
			if (i < 0) 
				count = pow(2, - (i * animate_border)); 
			else 
				if  (i == 0) 
					count = 1;
				else 
					count = pow(2, i * animate_border); 
			

			vita2d_draw_line(x - i, y - i, x + w + i, y - i, COLOR_ALPHA(color, a/count));	
			vita2d_draw_line(x + w + i, y - i, x + w + i - 1, y + h + i - 1, COLOR_ALPHA(color, a/count));		
			vita2d_draw_line(x + w + i, y + h + i, x - i , y + h + i, COLOR_ALPHA(color, a/count));			
			vita2d_draw_line(x - i, y + h + i, x - i - 1, y - i - 1, COLOR_ALPHA(color, a/count));	
						
		}
	}
}


void return_current_pos() {
	have_touch = false;
	if ((rel_pos / length_row * (height_item + length_border) < -slide_value) || (rel_pos / length_row * (height_item + length_border) + height_item > -slide_value + (SCREEN_HEIGHT - START_Y))) {			
			animate = true;
			if (rel_pos /  length_row == file_list.length/ length_row)
				Position = - (rel_pos /  length_row * (height_item +  length_border)) - length_border;	
			else
				Position = - (rel_pos /  length_row * (height_item +  length_border));// - length_border;				
		}
}



void dirLevelUp() {
	base_pos_list[dir_level] = base_pos;
	rel_pos_list[dir_level] = rel_pos;
	dir_level++;
	base_pos_list[dir_level] = 0;
	rel_pos_list[dir_level] = 0;
	base_pos = 0;
	rel_pos = 0;
}

int isInArchive() {
	return is_in_archive;
}

void dirUpCloseArchive() {
	if (isInArchive() && dir_level_archive >= dir_level) {
		is_in_archive = 0;
		archiveClose();
		dir_level_archive = -1;
	}
}

void dirUp() {
	removeEndSlash(file_list.path);

	char *p;

	p = strrchr(file_list.path, '/');
	if (p) {
		p[1] = '\0';
		dir_level--;
		goto DIR_UP_RETURN;
	}

	p = strrchr(file_list.path, ':');
	if (p) {
		if (strlen(file_list.path) - ((p + 1) - file_list.path) > 0) {
			p[1] = '\0';
			dir_level--;
			goto DIR_UP_RETURN;
		}
	}

	strcpy(file_list.path, HOME_PATH);
	dir_level = 0;

DIR_UP_RETURN:
	base_pos = base_pos_list[dir_level];
	rel_pos = rel_pos_list[dir_level];
	dirUpCloseArchive();
}


int refreshFileList() {
	int ret = 0, res = 0;

	do {
		fileListEmpty(&file_list);

		res = fileListGetEntries(&file_list, file_list.path);

		if (res < 0) {
			ret = res;
			dirUp();
		}
	} while (res < 0);

	// Correct position after deleting the latest entry of the file list
	while ((base_pos + rel_pos) >= file_list.length) {
		if (base_pos > 0) {
			base_pos--;
		} else {
			if (rel_pos > 0) {
				rel_pos--;
			}
		}
	}

	// Correct position after deleting an entry while the scrollbar is on the bottom
	if (file_list.length >= MAX_POSITION) {
		while ((base_pos + MAX_POSITION - 1) >= file_list.length) {
			if (base_pos > 0) {
				base_pos--;
				rel_pos++;
			}
		}
	}

	return ret;
}

void refreshMarkList() {
	FileListEntry *entry = mark_list.head;

	int length = mark_list.length;

	int i;
	for (i = 0; i < length; i++) {
		// Get next entry already now to prevent crash after entry is removed
		FileListEntry *next = entry->next;

		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, entry->name);

		// Check if the entry still exits. If not, remove it from list
		SceIoStat stat;
		if (sceIoGetstat(path, &stat) < 0)
			fileListRemoveEntry(&mark_list, entry);

		// Next
		entry = next;
	}
}

void refreshCopyList() {
	FileListEntry *entry = copy_list.head;

	int length = copy_list.length;

	int i;
	for (i = 0; i < length; i++) {
		// Get next entry already now to prevent crash after entry is removed
		FileListEntry *next = entry->next;

		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s%s", copy_list.path, entry->name);

		// TODO: fix for archives
		// Check if the entry still exits. If not, remove it from list
		SceIoStat stat;
		if (sceIoGetstat(path, &stat) < 0)
			fileListRemoveEntry(&copy_list, entry);

		// Next
		entry = next;
	}
}

void resetFileLists() {
	memset(&file_list, 0, sizeof(FileList));
	memset(&mark_list, 0, sizeof(FileList));
	memset(&copy_list, 0, sizeof(FileList));

	// Home
	strcpy(file_list.path, HOME_PATH);

	refreshFileList();
}

int handleFile(char *file, FileListEntry *entry) {
	int res = 0;

	int type = getFileType(file);	
	switch (type) {
		case FILE_TYPE_VPK:
		case FILE_TYPE_ZIP:
			if (isInArchive())
				type = FILE_TYPE_UNKNOWN;

			break;
	}

	switch (type) {
		case FILE_TYPE_UNKNOWN:
			res = textViewer(file);
			break;
			
		case FILE_TYPE_BMP:
		case FILE_TYPE_PNG:
		case FILE_TYPE_JPEG:
			res = photoViewer(file, type, &file_list, entry, &base_pos, &rel_pos);
			break;

		case FILE_TYPE_MP3:
			res = audioPlayer(file, &file_list, entry, &base_pos, &rel_pos, SCE_AUDIODEC_TYPE_MP3, 1);
			break;

		case FILE_TYPE_VPK:
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			break;
			
		case FILE_TYPE_ZIP:
			res = archiveOpen(file);
			break;
			
		default:
			errorDialog(type);
			break;
	}

	if (res < 0) {
		errorDialog(res);
		return res;
	}

	return type;
}

void drawScrollBar2(int file_list_length) {
	if (file_list_length / length_row * (height_item + length_border) > SCREEN_HEIGHT - START_Y) {
		
		float view_size = SCREEN_HEIGHT - START_Y;
		float max_scroll = (file_list_length / length_row * (height_item + length_border)) + view_size - height_item;
		vita2d_draw_rectangle(SCROLL_BAR_X, START_Y + 10.0f , SCROLL_BAR_WIDTH, view_size - FONT_Y_SPACE - 15.0f, SCROLL_BAR_BG_COLOR);// COLOR_ALPHA(GRAY, 0xC8));	
		
		float height = ( (view_size - FONT_Y_SPACE - 15.0f) * view_size)/ max_scroll ;
		float y = ( (-slide_value ) * (view_size - FONT_Y_SPACE - 15.0f )  )  / max_scroll  + START_Y + 10.0f;
		if (y < START_Y + 10.0f) y = START_Y + 10.0f;
		//if (y > view_size - FONT_Y_SPACE - 15.0f) y = view_size - FONT_Y_SPACE - 15.0f;
				
		vita2d_draw_rectangle(SCROLL_BAR_X, y, SCROLL_BAR_WIDTH, height, SCROLL_BAR_COLOR);// COLOR_ALPHA(WHITE, 0x64));
	}
}

void drawScrollBar(int pos, int n) {
	if (n > MAX_POSITION) {
		vita2d_draw_rectangle(SCROLL_BAR_X, START_Y, SCROLL_BAR_WIDTH, MAX_ENTRIES * FONT_Y_SPACE, SCROLL_BAR_BG_COLOR);

		float y = START_Y + ((pos * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);
		float height = ((MAX_POSITION * FONT_Y_SPACE) / (n * FONT_Y_SPACE)) * (MAX_ENTRIES * FONT_Y_SPACE);

		vita2d_draw_rectangle(SCROLL_BAR_X, MIN(y, (START_Y + MAX_ENTRIES * FONT_Y_SPACE - height)), SCROLL_BAR_WIDTH, MAX(height, SCROLL_BAR_MIN_HEIGHT), SCROLL_BAR_COLOR);
	}
}

void drawShellInfo(char *path) {
	float SHELL_MARGIN_Y_CUSTOM = 2.0f;
	vita2d_draw_rectangle(0, 0, SCREEN_WIDTH , PATH_Y + FONT_Y_SPACE, COLOR_ALPHA(WHITE, 0xC8));
	vita2d_draw_texture(title_bar_bg_image, 0, 0);

	// Title
	pgf_draw_textf(SHELL_MARGIN_X, SHELL_MARGIN_Y_CUSTOM, WHITE, FONT_SIZE, "VitaShell %d.%d", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR);
	//pgf_draw_textf(SHELL_MARGIN_X, SHELL_MARGIN_Y_CUSTOM, WHITE, FONT_SIZE, "VitaShell %d.%d DEBUG : %s | %f| %f", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR, debug, debug2, debug3);

	// Battery
	float battery_x = ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_texture_get_width(battery_image));
	vita2d_draw_texture(battery_image, battery_x, SHELL_MARGIN_Y_CUSTOM + 3.0f);

	vita2d_texture *battery_bar_image = battery_bar_green_image;

	if (scePowerIsLowBattery())
		battery_bar_image = battery_bar_red_image;

	float percent = scePowerGetBatteryLifePercent() / 100.0f;

	float width = vita2d_texture_get_width(battery_bar_image);
	vita2d_draw_texture_part(battery_bar_image, battery_x + 3.0f + (1.0f - percent) * width, SHELL_MARGIN_Y_CUSTOM + 5.0f, (1.0f - percent) * width, 0.0f, percent * width, vita2d_texture_get_height(battery_bar_image));

	// Date & time
	SceDateTime time;
	sceRtcGetCurrentClock(&time,0);

	char date_string[16];
	getDateString(date_string, date_format, &time);

	char time_string[24];
	getTimeString(time_string, time_format, &time);

	char string[64];
	sprintf(string, "%s  %s", date_string, time_string);
	float date_time_x = ALIGN_LEFT(battery_x - 12.0f, vita2d_pgf_text_width(font, FONT_SIZE, string));
	pgf_draw_text(date_time_x, SHELL_MARGIN_Y_CUSTOM, WHITE, FONT_SIZE, string);

	// FTP
	if (ftpvita_is_initialized())
		vita2d_draw_texture(ftp_image, date_time_x - 30.0f, SHELL_MARGIN_Y_CUSTOM + 3.0f);

	// TODO: make this more elegant
	// Path
	int line_width = 0;

	int i;
	for (i = 0; i < strlen(path); i++) {
		char ch_width = font_size_cache[(int)path[i]];

		// Too long
		if ((line_width + ch_width) >= MAX_WIDTH)
			break;

		// Increase line width
		line_width += ch_width;
	}

	char path_first_line[256], path_second_line[256];

	strncpy(path_first_line, path, i);
	path_first_line[i] = '\0';

	strcpy(path_second_line, path + i);

	//vita2d_draw_rectangle(0, PATH_Y, SCREEN_WIDTH , FONT_Y_SPACE, COLOR_ALPHA(BLACK, 0xC8));
	pgf_draw_text(SHELL_MARGIN_X, PATH_Y, BLACK, FONT_SIZE, path_first_line);
	pgf_draw_text(SHELL_MARGIN_X, PATH_Y + FONT_Y_SPACE, BLACK, FONT_SIZE, path_second_line);

	char str[128];

	// Show numbers of files and folders
	// sprintf(str, "%d files and %d folders", file_list.files, file_list.folders);

	// Show memory card
/*
	uint64_t free_size = 0, max_size = 0;
	sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

	char free_size_string[16], max_size_string[16];
	getSizeString(free_size_string, free_size);
	getSizeString(max_size_string, max_size);

	sprintf(str, "%s/%s", free_size_string, max_size_string);
*/

	// Draw on bottom left
	// pgf_draw_textf(ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, str)), SCREEN_HEIGHT - SHELL_MARGIN_Y - FONT_Y_SPACE - 2.0f, LITEGRAY, FONT_SIZE, str);
}

enum MenuEntrys {
	MENU_ENTRY_MARK_UNMARK_ALL,
	MENU_ENTRY_EMPTY_1,
	MENU_ENTRY_MOVE,
	MENU_ENTRY_COPY,
	MENU_ENTRY_PASTE,
	MENU_ENTRY_EMPTY_3,
	MENU_ENTRY_DELETE,
	MENU_ENTRY_RENAME,
	MENU_ENTRY_EMPTY_4,
	MENU_ENTRY_NEW_FOLDER,
};

enum MenuVisibilities {
	VISIBILITY_UNUSED,
	VISIBILITY_INVISIBLE,
	VISIBILITY_VISIBLE,
};

typedef struct {
	int name;
	int visibility;
} MenuEntry;

MenuEntry menu_entries[] = {
	{ MARK_ALL, VISIBILITY_INVISIBLE },
	{ -1, VISIBILITY_UNUSED },
	{ MOVE, VISIBILITY_INVISIBLE },
	{ COPY, VISIBILITY_INVISIBLE },
	{ PASTE, VISIBILITY_INVISIBLE },
	{ -1, VISIBILITY_UNUSED },
	{ DELETE, VISIBILITY_INVISIBLE },
	{ RENAME, VISIBILITY_INVISIBLE },
	{ -1, VISIBILITY_UNUSED },
	{ NEW_FOLDER, VISIBILITY_INVISIBLE },
};

#define N_MENU_ENTRIES (sizeof(menu_entries) / sizeof(MenuEntry))

void initContextMenu() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == VISIBILITY_INVISIBLE)
			menu_entries[i].visibility = VISIBILITY_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_MOVE].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_COPY].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = VISIBILITY_INVISIBLE;
	}

	// Invisible 'Paste' if nothing is copied yet
	if (copy_list.length == 0)
		menu_entries[MENU_ENTRY_PASTE].visibility = VISIBILITY_INVISIBLE;

	// Invisble write operations in archives
	if (isInArchive()) { // TODO: read-only mount points
		menu_entries[MENU_ENTRY_MOVE].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_PASTE].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_NEW_FOLDER].visibility = VISIBILITY_INVISIBLE;
	}

	// TODO: Moving from one mount point to another is not possible

	// Mark/Unmark all text
	if (mark_list.length == (file_list.length - 1)) { // All marked
		menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
	} else { // Not all marked yet
		// On marked entry
		if (fileListFindEntry(&mark_list, file_entry->name)) {
			menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = UNMARK_ALL;
		} else {
			menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].name = MARK_ALL;
		}
	}

	// Go to first entry
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == VISIBILITY_VISIBLE) {
			ctx_menu_pos = i;
			break;
		}
	}

	if (i == N_MENU_ENTRIES)
		ctx_menu_pos = -1;
}


float easeOut(float x0, float x1, float a) {
	float dx = (x1 - x0);	
	if (dx > 0)
		return ((dx * a) > 0.91f) ? (dx * a) : dx;
	else
	 	return ((dx * a) < -1) ? (dx * a) : dx;	
}

void slide_to_location( float speed) {
	if (animate) {	
			if (slide_value < Position){
				slide_value -= easeOut(Position , slide_value , speed);
				if (slide_value >= Position) {					
					animate = false;
					Position = 0;
				}
			}
			else {
				slide_value -= easeOut(Position , slide_value , speed);
				if (slide_value <= Position) {					
					animate = false;
					Position = 0;
				}
			}
	}
}

void drawContextMenu() {
	// Easing out
	if (ctx_menu_mode == CONTEXT_MENU_CLOSING) {
		if (ctx_menu_width > 0.0f) {
			ctx_menu_width -= easeOut(0.0f, ctx_menu_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_CLOSED;
		}
	}

	if (ctx_menu_mode == CONTEXT_MENU_OPENING) {
		if (ctx_menu_width < ctx_menu_max_width) {
			ctx_menu_width += easeOut(ctx_menu_width, ctx_menu_max_width, 0.375f);
		} else {
			ctx_menu_mode = CONTEXT_MENU_OPENED;
		}
	}

	// Draw context menu
	if (ctx_menu_mode != CONTEXT_MENU_CLOSED) {
		vita2d_draw_texture_part(context_image, SCREEN_WIDTH - ctx_menu_width, 0.0f, 0.0f, 0.0f, ctx_menu_width, SCREEN_HEIGHT);
		int i;
		for (i = 0; i < N_MENU_ENTRIES; i++) {
			if (menu_entries[i].visibility == VISIBILITY_UNUSED)
				continue;

			float y = START_Y + (i * FONT_Y_SPACE);

			uint32_t color = WHITE;

			if (i == ctx_menu_pos)
				color = GREEN;

			if (menu_entries[i].visibility == VISIBILITY_INVISIBLE)
				color = DARKGRAY;

			pgf_draw_text(SCREEN_WIDTH - ctx_menu_width + CONTEXT_MENU_MARGIN, y, color, FONT_SIZE, language_container[menu_entries[i].name]);
		}
	}
}

void contextMenuCtrl() {
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
		int i;
		for (i = N_MENU_ENTRIES - 1; i >= 0; i--) {
			if (menu_entries[i].visibility == VISIBILITY_VISIBLE) {
				if (i < ctx_menu_pos) {
					ctx_menu_pos = i;
					break;
				}
			}
		}
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN) {
		int i;
		for (i = 0; i < N_MENU_ENTRIES; i++) {
			if (menu_entries[i].visibility == VISIBILITY_VISIBLE) {
				if (i > ctx_menu_pos) {
					ctx_menu_pos = i;
					break;
				}
			}
		}
	}

	// Back
	if (pressed_buttons & SCE_CTRL_TRIANGLE || pressed_buttons & SCE_CTRL_CANCEL || (TOUCH_FRONT() == 9)) {
		//Reset state everytime TOUCH_FRONT called
		State_Touch = -1;
		ctx_menu_mode = CONTEXT_MENU_CLOSING;
	}

	// Handle
	if (pressed_buttons & SCE_CTRL_ENTER) {
		switch (ctx_menu_pos) {
			case MENU_ENTRY_MARK_UNMARK_ALL:
			{
				int on_marked_entry = 0;
				int length = mark_list.length;

				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
				if (fileListFindEntry(&mark_list, file_entry->name))
					on_marked_entry = 1;

				// Empty mark list
				fileListEmpty(&mark_list);

				// Mark all if not all entries are marked yet and we are not focusing on a marked entry
				if (length != (file_list.length - 1) && !on_marked_entry) {
					FileListEntry *file_entry = file_list.head->next; // Ignore '..'

					int i;
					for (i = 0; i < file_list.length - 1; i++) {
						FileListEntry *mark_entry = malloc(sizeof(FileListEntry));
						memcpy(mark_entry, file_entry, sizeof(FileListEntry));
						fileListAddEntry(&mark_list, mark_entry, SORT_NONE);

						// Next
						file_entry = file_entry->next;
					}
				}

				break;
			}
			
			case MENU_ENTRY_MOVE:
			case MENU_ENTRY_COPY:
			{
				// Mode
				if (ctx_menu_pos == MENU_ENTRY_MOVE) {
					copy_mode = COPY_MODE_MOVE;
				} else {
					copy_mode = isInArchive() ? COPY_MODE_EXTRACT : COPY_MODE_NORMAL;
				}

				// Empty copy list at first
				if (copy_list.length > 0)
					fileListEmpty(&copy_list);

				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

				// Paths
				if (fileListFindEntry(&mark_list, file_entry->name)) { // On marked entry
					// Copy mark list to copy list
					FileListEntry *mark_entry = mark_list.head;

					int i;
					for (i = 0; i < mark_list.length; i++) {
						FileListEntry *copy_entry = malloc(sizeof(FileListEntry));
						memcpy(copy_entry, mark_entry, sizeof(FileListEntry));
						fileListAddEntry(&copy_list, copy_entry, SORT_NONE);

						// Next
						mark_entry = mark_entry->next;
					}
				} else {
					FileListEntry *copy_entry = malloc(sizeof(FileListEntry));
					memcpy(copy_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&copy_list, copy_entry, SORT_NONE);
				}

				strcpy(copy_list.path, file_list.path);

				char *message;

				// On marked entry
				if (fileListFindEntry(&copy_list, file_entry->name)) {
					if (copy_list.length == 1) {
						message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
					} else {
						message = language_container[COPIED_FILES_FOLDERS];
					}
				} else {
					message = language_container[file_entry->is_folder ? COPIED_FOLDER : COPIED_FILE];
				}

				// Copy message
				infoDialog(message, copy_list.length);

				break;
			}

			case MENU_ENTRY_PASTE:
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[copy_mode == COPY_MODE_MOVE ? MOVING : COPYING]);
				dialog_step = DIALOG_STEP_PASTE;
				break;

			case MENU_ENTRY_DELETE:
			{
				char *message;

				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

				// On marked entry
				if (fileListFindEntry(&mark_list, file_entry->name)) {
					if (mark_list.length == 1) {
						message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
					} else {
						message = language_container[DELETE_FILES_FOLDERS_QUESTION];
					}
				} else {
					message = language_container[file_entry->is_folder ? DELETE_FOLDER_QUESTION : DELETE_FILE_QUESTION];
				}

				initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
				dialog_step = DIALOG_STEP_DELETE_QUESTION;
				break;
			}

			case MENU_ENTRY_RENAME:
			{
				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

				char name[MAX_NAME_LENGTH];
				strcpy(name, file_entry->name);
				removeEndSlash(name);

				initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH);

				dialog_step = DIALOG_STEP_RENAME;
				break;
			}
			
			case MENU_ENTRY_NEW_FOLDER:
			{
				// Find a new folder name
				char path[MAX_PATH_LENGTH];

				int count = 1;
				while (1) {
					if (count == 1) {
						snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, language_container[NEW_FOLDER]);
					} else {
						snprintf(path, MAX_PATH_LENGTH, "%s%s (%d)", file_list.path, language_container[NEW_FOLDER], count);
					}

					SceIoStat stat;
					if (sceIoGetstat(path, &stat) < 0)
						break;

					count++;
				}

				initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path), MAX_NAME_LENGTH);
				dialog_step = DIALOG_STEP_NEW_FOLDER;
				break;
			}
		}

		ctx_menu_mode = CONTEXT_MENU_CLOSING;
	}
}


int dialogSteps() {
	int refresh = 0;

	int msg_result = updateMessageDialog();
	int ime_result = updateImeDialog();

	switch (dialog_step) {
		// Without refresh
		case DIALOG_STEP_ERROR:
		case DIALOG_STEP_INFO:
		case DIALOG_STEP_SYSTEM:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		// With refresh
		case DIALOG_STEP_COPIED:
		case DIALOG_STEP_DELETED:
		case DIALOG_STEP_INSTALLED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_CANCELLED:
			refresh = 1;
			dialog_step = DIALOG_STEP_NONE;
			break;
			
		case DIALOG_STEP_MOVED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				fileListEmpty(&copy_list);
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_FTP:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				dialog_step = DIALOG_STEP_NONE;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				powerUnlock();
				ftpvita_fini();
				refresh = 1;
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_PASTE:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				CopyArguments args;
				args.file_list = &file_list;
				args.copy_list = &copy_list;
				args.archive_path = archive_path;
				args.copy_mode = copy_mode;

				SceUID thid = sceKernelCreateThread("copy_thread", (SceKernelThreadEntry)copy_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(CopyArguments), &args);

				dialog_step = DIALOG_STEP_COPYING;
			}

			break;
			
		case DIALOG_STEP_DELETE_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DELETING]);
				dialog_step = DIALOG_STEP_DELETE_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_DELETE_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				DeleteArguments args;
				args.file_list = &file_list;
				args.mark_list = &mark_list;
				args.index = base_pos + rel_pos;

				SceUID thid = sceKernelCreateThread("delete_thread", (SceKernelThreadEntry)delete_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(DeleteArguments), &args);

				dialog_step = DIALOG_STEP_DELETING;
			}

			break;
			
		case DIALOG_STEP_INSTALL_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
				dialog_step = DIALOG_STEP_INSTALL_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_INSTALL_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				InstallArguments args;
				args.file = cur_file;

				SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(InstallArguments), &args);

				dialog_step = DIALOG_STEP_INSTALLING;
			}

			break;
			
		case DIALOG_STEP_INSTALL_WARNING:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				dialog_step = DIALOG_STEP_INSTALL_WARNING_AGREED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_CANCELLED;
			}

			break;
			
		case DIALOG_STEP_RENAME:
			if (ime_result == IME_DIALOG_RESULT_FINISHED) {
				char *name = (char *)getImeDialogInputTextUTF8();
				if (name[0] == '\0') {
					dialog_step = DIALOG_STEP_NONE;
				} else {
					FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

					char old_name[MAX_NAME_LENGTH];
					strcpy(old_name, file_entry->name);
					removeEndSlash(old_name);

					if (strcmp(old_name, name) == 0) { // No change
						dialog_step = DIALOG_STEP_NONE;
					} else {
						char old_path[MAX_PATH_LENGTH];
						char new_path[MAX_PATH_LENGTH];

						snprintf(old_path, MAX_PATH_LENGTH, "%s%s", file_list.path, old_name);
						snprintf(new_path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

						int res = sceIoRename(old_path, new_path);
						if (res < 0) {
							errorDialog(res);
						} else {
							refresh = 1;
							dialog_step = DIALOG_STEP_NONE;
						}
					}
				}
			} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_NEW_FOLDER:
			if (ime_result == IME_DIALOG_RESULT_FINISHED) {
				char *name = (char *)getImeDialogInputTextUTF8();
				if (name[0] == '\0') {
					dialog_step = DIALOG_STEP_NONE;
				} else {
					char path[MAX_PATH_LENGTH];
					snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, name);

					int res = sceIoMkdir(path, 0777);
					if (res < 0) {
						errorDialog(res);
					} else {
						refresh = 1;
						dialog_step = DIALOG_STEP_NONE;
					}
				}
			} else if (ime_result == IME_DIALOG_RESULT_CANCELED) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
	}

	return refresh;
}


void fileBrowserMenuCtrl() {
	// Hidden trigger
	if (current_buttons & SCE_CTRL_LTRIGGER && current_buttons & SCE_CTRL_RTRIGGER && current_buttons & SCE_CTRL_START) {
		SceSystemSwVersionParam sw_ver_param;
		sw_ver_param.size = sizeof(SceSystemSwVersionParam);
		sceKernelGetSystemSwVersion(&sw_ver_param);

		char mac_string[32];
		sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X", mac.data[0], mac.data[1], mac.data[2], mac.data[3], mac.data[4], mac.data[5]);

		uint64_t free_size = 0, max_size = 0;
		sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

		char free_size_string[16], max_size_string[16];
		getSizeString(free_size_string, free_size);
		getSizeString(max_size_string, max_size);

		initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, "System software: %s\nModel: 0x%08X\nMAC address: %s\nIP address: %s\nMemory card: %s/%s", sw_ver_param.version_string, sceKernelGetModelForCDialog(), mac_string, ip, free_size_string, max_size_string);
		dialog_step = DIALOG_STEP_SYSTEM;
	}

	// FTP
	if (pressed_buttons & SCE_CTRL_SELECT) {
		// Init FTP
		if (!ftpvita_is_initialized()) {
			int res = ftpvita_init(vita_ip, &vita_port);
			if (res < 0) {
				infoDialog(language_container[WIFI_ERROR]);
			} else {
				// Add all the current mountpoints to ftpvita
				int i;
				for (i = 0; i < getNumberMountPoints(); i++) {
					char **mount_points = getMountPoints();
					if (mount_points[i]) {
						ftpvita_add_device(mount_points[i]);
					}
				}
			}
		}

		// Dialog
		if (ftpvita_is_initialized()) {
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
			dialog_step = DIALOG_STEP_FTP;
		}
	}

	
		if ((TOUCH_FRONT() == 0) & (ctx_menu_mode == CONTEXT_MENU_CLOSED)) {
			//Reset state everytime TOUCH_FRONT called
		  State_Touch = -1;
		  have_touch = true;
		  int i = 0;
		  int column = 0;
		  int row = 0;
		  float t_y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT) - height_item - slide_value;
		  float t_x = lerp(touch.report[0].x, 1920, SCREEN_WIDTH) - width_item;		  
		  	while (true) {			 
			  float x = SHELL_MARGIN_X + (column * width_item);				  		  
			  float y = START_Y + (row * height_item) ;
			  if ((t_x <= x) & (t_y <= y)) {
				  
				  if (i < file_list.length) {					
					rel_pos = i;
				  }
				  else touch_nothing = true;							
				break;								  
			  }
			  else {
					i++;				  
					column++;
					if (column >= length_row) {
						column = 0;						
						row ++;
						
					}
			  }			
		  }		  
  	}


	//Slide up
	if ((TOUCH_FRONT() == 6) & (ctx_menu_mode == CONTEXT_MENU_CLOSED)) {
		//Reset state everytime TOUCH_FRONT called		
		State_Touch = -1;		
		if ((slide_value  > -slide_value_limit) ) {
			if (slide_value_limit > SCREEN_HEIGHT - START_Y) {	
				slide_value -=  slide_value - (-(pre_touch_y - touch.report[0].y) + slide_value_hold);										
			}
		}
		else {
			animate = true;
			Position = -slide_value_limit;			
		}
	}

	//Slide down
	if ((TOUCH_FRONT() == 7) & (ctx_menu_mode == CONTEXT_MENU_CLOSED)) {
		//Reset state everytime TOUCH_FRONT called
		State_Touch = -1;		
		if((slide_value < 0)) {	
			slide_value -= slide_value - ((touch.report[0].y - pre_touch_y) + slide_value_hold);						
		}
		else {
			animate = true;
			Position = 0;			
		}
	}

	// Move
	if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {				
		if ((rel_pos - length_row) > 0) {			
			rel_pos-= length_row;
		}
		else {
			rel_pos = 0;
			if ((base_pos - length_row) > 0) {
					base_pos -= length_row;
				} 
				else {
					base_pos = 0;
				}
		}
		return_current_pos();	
	} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN ) {									
		if ((rel_pos + length_row) < file_list.length) {
			rel_pos += length_row;
			if ((rel_pos + length_row) > MAX_POSITION - length_row) {
				if ((base_pos + rel_pos + length_row) < file_list.length) {
					if ((base_pos + length_row) < MAX_POSITION) {
						base_pos += length_row;	
					}										
				}
				
			}
		}
		else {
					rel_pos = file_list.length - base_pos -1;
				}
		return_current_pos();		
	} else if (hold_buttons & SCE_CTRL_RIGHT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
		if ((rel_pos + 1) < file_list.length) {
			if ((rel_pos + 1) < MAX_POSITION) {
				rel_pos++;
			} else {
				if ((base_pos + rel_pos + length_row) < file_list.length) {
					base_pos += length_row;	
				}
			}
		}
		return_current_pos();
	} else if (hold_buttons & SCE_CTRL_LEFT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
		if (rel_pos > 0) {
			rel_pos--;
		} else {
			if (base_pos > 0) {
				base_pos -= length_row;
			}
		}
		return_current_pos();
	}

	// Not at 'home'
	if (dir_level > 0) {
		// Context menu trigger
		if ((pressed_buttons & SCE_CTRL_TRIANGLE) || (TOUCH_FRONT() == 8)) {
			State_Touch = -1;
			if (ctx_menu_mode == CONTEXT_MENU_CLOSED) {
				initContextMenu();
				ctx_menu_mode = CONTEXT_MENU_OPENING;
			}
		}

		// Mark entry
		if ((pressed_buttons & SCE_CTRL_SQUARE) || ((TOUCH_FRONT() == 3) & !have_touch_hold & !touch_nothing & (ctx_menu_mode == CONTEXT_MENU_CLOSED))) {
			State_Touch = -1;

			have_touch_hold = true;
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
			if (strcmp(file_entry->name, DIR_UP) != 0) {
				if (!fileListFindEntry(&mark_list, file_entry->name)) {
					FileListEntry *mark_entry = malloc(sizeof(FileListEntry));
					memcpy(mark_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&mark_list, mark_entry, SORT_NONE);
				} else {
					fileListRemoveEntryByName(&mark_list, file_entry->name);
				}
			}
		}

		// Back
		if (pressed_buttons & SCE_CTRL_CANCEL || (TOUCH_FRONT() == 9)) {
			State_Touch = -1;
			//reset position to top
			slide_value = 0;
			Position = 0;

			fileListEmpty(&mark_list);
			dirUp();
			refreshFileList();
		}
	}

	// Handle 
	if (pressed_buttons & SCE_CTRL_ENTER || ((TOUCH_FRONT() == 1) & (ctx_menu_mode == CONTEXT_MENU_CLOSED))) {	
		State_Touch = -1;
				
		if (touch_nothing || have_touch_hold) {			
			touch_nothing = false;
			have_touch_hold = false;
			return;
		}

		

		fileListEmpty(&mark_list);

		// Handle file or folder
		FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
		if (file_entry->is_folder) {

			//reset position to top
			slide_value = 0;
			Position = 0;

			if (strcmp(file_entry->name, DIR_UP) == 0) {
				dirUp();
			} else {
				if (dir_level == 0) {
					strcpy(file_list.path, file_entry->name);
				} else {
					if (dir_level > 1)
						addEndSlash(file_list.path);
					strcat(file_list.path, file_entry->name);
				}

				dirLevelUp();
			}

			// Open folder
			int res = refreshFileList();
			if (res < 0)
				errorDialog(res);
		} else {
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			int type = handleFile(cur_file, file_entry);
			
			// Archive mode
			if (type == FILE_TYPE_ZIP) {
				is_in_archive = 1;
				dir_level_archive = dir_level;

				snprintf(archive_path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				strcat(file_list.path, file_entry->name);
				addEndSlash(file_list.path);

				dirLevelUp();
				refreshFileList();
			}
		}
	}
}


int shellMain() {
	// Position
	memset(base_pos_list, 0, sizeof(base_pos_list));
	memset(rel_pos_list, 0, sizeof(rel_pos_list));

	// Paths
	memset(cur_file, 0, sizeof(cur_file));
	memset(archive_path, 0, sizeof(archive_path));

	// Reset file lists
	resetFileLists();
	
	while (1) {
		readPad();
		
		int refresh = 0;				
		
		// Control
		if (dialog_step == DIALOG_STEP_NONE) {
			if (ctx_menu_mode != CONTEXT_MENU_CLOSED) {
				contextMenuCtrl();
			} else {
				fileBrowserMenuCtrl();
			}
		} else {
			refresh = dialogSteps();
		}

		// Receive system event
 		SceAppMgrSystemEvent event;
 		sceAppMgrReceiveSystemEvent(&event);
 
 		// Refresh on app resume
 		if (event.systemEvent == SCE_APPMGR_SYSTEMEVENT_ON_RESUME) {
 			refresh = 1;
 		}

		if (refresh) {
			// Refresh lists
			refreshFileList();
			refreshMarkList();
			refreshCopyList();
		}

		// Start drawing
		startDrawing();
		vita2d_draw_texture_part(default_wallpaper, 0, START_Y - 1 , 0 , START_Y - 1, SCREEN_WIDTH, vita2d_texture_get_height(default_wallpaper));	
		//vita2d_draw_texture(bg_down_image, 0, START_Y - 1);		

		// Draw scroll bar
		drawScrollBar2(file_list.length);

		// Draw
		FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos);

		int i;
		int w = -1;
		int h = 0;
		int cur_pos = 0; // Current position
		
		for (i = 0; i < file_list.length && (base_pos + i) < file_list.length; i++) {
			
			if ((i % length_row == 0) & (i != 0)) {
				w = 0;
				h++;							
			}
			else {
				w++;				
			}

			float x = SHELL_MARGIN_X + (width_item * w);
			float y = START_Y + (h * height_item);			

			x += length_border * w;
			y += length_border * (h + 1);													
			
			//if item out user view is NEXT_FILE
			if ((y + slide_value + height_item > 0) & (y + slide_value < SCREEN_HEIGHT))
			{
				//vita2d_texture *icon = NULL;			
				int icon = 0;
					
				//Special Folder
				if (strcmp(file_entry->name, "gro0:") == 0) {				
					icon = 1;										
				} else
				if (strcmp(file_entry->name, "grw0:") == 0) {				
					icon = 2;										
				} else
				if (strcmp(file_entry->name, "ux0:") == 0) {				
					icon = 3;							
				} else
				if (strcmp(file_entry->name, "os0:") == 0) {				
					icon = 4;				
				} else
				if (strcmp(file_entry->name, "sa0:") == 0) {				
					icon = 5;				
				} else
				if (strcmp(file_entry->name, "ur0:") == 0) {				
					icon = 6;				
				} else
				if (strcmp(file_entry->name, "vd0:") == 0) {				
					icon = 7;				
				} else
				if (strcmp(file_entry->name, "vs0:") == 0) {				
					icon = 8;				
				} else
				if (strcmp(file_entry->name, "savedata0:") == 0) {				
					icon = 9;				
				} else
				if (strcmp(file_entry->name, "pd0:") == 0) {				
					icon = 10;				
				} else
				if (strcmp(file_entry->name, "app0:") == 0) {				
					icon = 11;				
				} else
				if (strcmp(file_entry->name, "ud0:") == 0) {				
					icon = 12;				
				} 
				
				else {
						// Folder default
						if (file_entry->is_folder) {							
							/*strcpy(path_icon,file_list.path);
							addEndSlash(path_icon);
							strcat(path_icon,file_entry->name);
							addEndSlash(path_icon);
							strcat(path_icon,"sce_sys");
							addEndSlash(path_icon);
							strcat(path_icon,"icon0.png");
							if( access(path_icon , F_OK ) != -1 ) {								
								icon = 99;
								//sceKernelStartThread(thid2, sizeof(path_icon), path_icon);
							}*/
															
								icon = 15;
							}

							// Images
						if (file_entry->type == FILE_TYPE_BMP || file_entry->type == FILE_TYPE_PNG || file_entry->type == FILE_TYPE_JPEG) {
								icon = 16;								
						} 

							// Archives
						if (!isInArchive()) {
								if (file_entry->type == FILE_TYPE_VPK || file_entry->type == FILE_TYPE_ZIP) {
									icon = 17;									
								}
						}

						if (file_entry->type == FILE_TYPE_MP3) {
								icon = 18;								
						}  								
				}
				
				// File name
				int length = strlen(file_entry->name);
				int line_width = 0;

				int j;
				for (j = 0; j < length; j++) {
					char ch_width = font_size_cache[(int)file_entry->name[j]];

					// Too long
					if ((line_width + ch_width) >= MAX_NAME_WIDTH_TILE)
						break;

					// Increase line width
					line_width += ch_width;
				}
						
				vita2d_draw_rectangle(x , y + slide_value , width_item, height_item , COLOR_ALPHA(DARKGRAY, 0xC8));
												
				switch(icon) {
					case 1  :
						vita2d_draw_texture_scale(game_card_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);											
						break;
					
					case 2  :
						vita2d_draw_texture_scale(game_card_storage_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);											
						break;
						
					case 3  :					
						vita2d_draw_texture_scale(memory_card_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);										
						break;  

					case 4  :
						vita2d_draw_texture_scale(os0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);															
						break;

					case 5  :
						vita2d_draw_texture_scale(sa0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;  	  
					
					case 6  :
						vita2d_draw_texture_scale(ur0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;  	  
					
					case 7  :
						vita2d_draw_texture_scale(vd0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;  	  
					
					case 8  :
						vita2d_draw_texture_scale(vs0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break; 

					case 9  :
						vita2d_draw_texture_scale(savedata0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;

					case 10  :
						vita2d_draw_texture_scale(pd0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;

					case 11  :
						vita2d_draw_texture_scale(app0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;

					case 12  :
						vita2d_draw_texture_scale(ud0_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break;
							
					
					case 15  :
						vita2d_draw_texture_scale(folder_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);															
						break; 

					case 16  :
						vita2d_draw_texture_scale(img_file_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break; 
					
					case 17  :
						vita2d_draw_texture_scale(run_file_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break; 

					case 18  :
						vita2d_draw_texture_scale(music_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);														
						break; 

					case 99  :
																									
						break; 

					default  :						
						vita2d_draw_texture_scale(unknown_file_image,  x + 10.0f, y + slide_value + 10.0f, 1.2f, 1.2f);																				
						break; 
														
					break;
					
				}
			
				char ch = 0;

				if (j != length) {
					ch = file_entry->name[j];
					file_entry->name[j] = '\0';
				}

				// Draw shortened file name			
				pgf_draw_text(x + 2.0f, y + height_item - FONT_Y_SPACE + slide_value, WHITE, FONT_SIZE, file_entry->name);
				

				if (j != length)
					file_entry->name[j] = ch;
			}
			else 
				goto NEXT_FILE;

			 vita2d_draw_texture_part(default_wallpaper, 0, 0 ,0 , 0, SCREEN_WIDTH, START_Y - 1);	
		
			//vita2d_draw_texture(bg_up_image, 0, 0);	

			// Draw shell info
			drawShellInfo(file_list.path);

			// Current position
			if (i == rel_pos) {	
				x_rel_pos_old -= easeOut(x, x_rel_pos_old, 0.2f);
				y_rel_pos_old -= easeOut(y, y_rel_pos_old, 0.2f);																
				cur_pos = i;				
			}

			// Marked
			if (fileListFindEntry(&mark_list, file_entry->name)) {
				vita2d_draw_rectangle(x , y + slide_value , width_item, height_item , COLOR_ALPHA(BLUE, 0x40));
				vita2d_draw_texture(mark_image, x + width_item - vita2d_texture_get_width(mark_image) - length_border, y + slide_value + length_border );									
			}			
			
NEXT_FILE:			
			// Next
			file_entry = file_entry->next;
		}

		//Draw Border
		if (!have_touch) {			
			DrawBorderRect (x_rel_pos_old, y_rel_pos_old + slide_value , width_item, height_item, GREEN, 6, true, true);							
		
			//Redraw ShellInfo to cover border
			if (y_rel_pos_old + slide_value < START_Y) {
				 vita2d_draw_texture_part(default_wallpaper, 0, 0 ,0 , 0, vita2d_texture_get_width(default_wallpaper), START_Y - 1);	

				// Draw shell info
				drawShellInfo(file_list.path);
			}
		}
					
		FileListEntry *file_entry2 = fileListGetNthEntry(&file_list, cur_pos + base_pos);
		vita2d_draw_rectangle(0, SCREEN_HEIGHT - FONT_Y_SPACE, SCREEN_WIDTH, FONT_Y_SPACE, COLOR_ALPHA(BLACK, 0xC8));
		// Draw shortened file name
		pgf_draw_text(5.0f , SCREEN_HEIGHT - FONT_Y_SPACE, WHITE, FONT_SIZE, file_entry2->name);

		// File information
		if (strcmp(file_entry2->name, DIR_UP) != 0) {
			// Folder/size
			char size_string[16];
			getSizeString(size_string, file_entry2->size);

			char *str = file_entry2->is_folder ? language_container[FOLDER] : size_string;

			pgf_draw_text(ALIGN_LEFT(INFORMATION_X, vita2d_pgf_text_width(font, FONT_SIZE, str)), SCREEN_HEIGHT - FONT_Y_SPACE, WHITE, FONT_SIZE, str);

			// Date
			char date_string[16];
			getDateString(date_string, date_format, &file_entry2->time);

			char time_string[24];
			getTimeString(time_string, time_format, &file_entry2->time);

			char string[64];
			sprintf(string, "%s %s", date_string, time_string);

			pgf_draw_text(ALIGN_LEFT(SCREEN_WIDTH - SHELL_MARGIN_X, vita2d_pgf_text_width(font, FONT_SIZE, string)), SCREEN_HEIGHT - FONT_Y_SPACE, WHITE, FONT_SIZE, string);
		}		
			
	

		if ( h > 0)
			slide_value_limit = h * (height_item + length_border ) + length_border;
		else 
			slide_value_limit = h * (height_item + length_border );
		
		//Animation fot multi purpose 
		slide_to_location( 0.1f);	
		

		// Draw context menu
		drawContextMenu();

		// End drawing
		endDrawing();
	}

	

	// Empty lists
	fileListEmpty(&copy_list);
	fileListEmpty(&mark_list);
	fileListEmpty(&file_list);

	return 0;
}

void initShell() {
	int i;
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility != VISIBILITY_UNUSED)
			ctx_menu_max_width = MAX(ctx_menu_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_entries[i].name]));

		if (menu_entries[i].name == MARK_ALL) {
			menu_entries[i].name = UNMARK_ALL;
			i--;
		}
	}

	ctx_menu_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	ctx_menu_max_width = MAX(ctx_menu_max_width, CONTEXT_MENU_MIN_WIDTH);

	x_rel_pos_old = SHELL_MARGIN_X;
	y_rel_pos_old = START_Y + length_border;	
}

void getNetInfo() {
	static char memory[16 * 1024];

	SceNetInitParam param;
	param.memory = memory;
	param.size = sizeof(memory);
	param.flags = 0;

	int net_init = sceNetInit(&param);
	int netctl_init = sceNetCtlInit();

	// Get mac address
	sceNetGetMacAddress(&mac, 0);

	// Get IP
	SceNetCtlInfo info;
	if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) < 0) {
		strcpy(ip, "-");
	} else {
		strcpy(ip, info.ip_address);
	}

	if (netctl_init >= 0)
		sceNetCtlTerm();

	if (net_init >= 0)
		sceNetTerm();
}

int main(int argc, const char *argv[]) {
	// Init VitaShell
	initVitaShell();

	// Get net info
	getNetInfo();

	// No custom config, in case they are damaged or unuseable
	readPad();
	if (current_buttons & SCE_CTRL_LTRIGGER)
		use_custom_config = 0;

	// Load theme
	loadTheme();

	// Load language
	loadLanguage(language);

	// Main
	initShell();
	shellMain();

	// Finish VitaShell
	finishVitaShell();
	
	return 0;
}
