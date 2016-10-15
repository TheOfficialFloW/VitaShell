
#include "UI2.h"
#include "touch_shell.h"
#include "main.h"
#include "init.h"
#include "io_process.h"
#include "package_installer.h"
#include "network_update.h"
#include "context_menu.h"
#include "archive.h"
#include "photo.h"
#include "audioplayer.h"
#include "file.h"
#include "text.h"
#include "hex.h"
#include "message_dialog.h"
#include "ime_dialog.h"
#include "theme.h"
#include "language.h"
#include "utils.h"
#include "sfo.h"
#include "list_dialog.h"

bool Change_UI = false;

static int _newlib_heap_size_user = 64 * 1024 * 1024;
 //bool Change_UI = false;

#define MAX_DIR_LEVELS 1024

// Context menu
static float ctx_menu_max_width = 0.0f, ctx_menu_more_max_width = 0.0f;

// File lists
static FileList file_list, mark_list, copy_list, install_list;

// Paths
static char cur_file[MAX_PATH_LENGTH], archive_path[MAX_PATH_LENGTH], install_path[MAX_PATH_LENGTH];

// Position
static int base_pos = 0, rel_pos = 0;
static int base_pos_list[MAX_DIR_LEVELS];
static int rel_pos_list[MAX_DIR_LEVELS];
static int dir_level = 0;

// Copy mode
static int copy_mode = COPY_MODE_NORMAL;

// Archive
int is_in_archive = 0;
static int dir_level_archive = -1;

// FTP
static char vita_ip[16];
static unsigned short int vita_port;

static List toolbox_list;

int MAX_NAME_WIDTH_TILE = 0;
int length_row_items = 7;
float length_border = 5.0f;
float width_item = 0;
float height_item = 0;
float max_extend_item_value = 10;
int animate_extend_item = 0;
int animate_extend_item_pos = 0;

//notification value
float slide_value_notification = HEIGHT_TITLE_BAR;
bool notification_open = false;
bool notification_close = false;
bool notification_start = false;
bool notification_on = false;
float width_item_notification = 300.0f;
float height_item_notification = 60.0f;
bool toggle_item_notification[10];

float slide_value_limit = 0;
float speed_slide = 10.0f;
int value_cur_pos = 0;
bool animate_slide = false;
int Position = 0;

void DrawBorderRectSimple(int x, int y, int w, int h, unsigned int color, int lengthBorder) {
	int i = 0;
	int a = 255;	
	int count = 0;

	for (i = 0; i <= lengthBorder; i ++) {		
			if  (i == 0) 
				count = 1;
			else 
				count = pow(2, i * lengthBorder); 		

		vita2d_draw_line(x - i , y - i, x + w + i , y - i, COLOR_ALPHA(color, a/count));	
		vita2d_draw_line(x + w + i, y - i , x + w + i - 1, y + h + i - 1 , COLOR_ALPHA(color, a/count));		
		vita2d_draw_line(x + w + i , y + h + i, x - i , y + h + i, COLOR_ALPHA(color, a/count));			
		vita2d_draw_line(x - i, y + h + i , x - i - 1, y - i - 1 , COLOR_ALPHA(color, a/count));		
					
		}
}


void return_current_pos() {
	have_touch = false;
	notification_close = true;
	if ((rel_pos / length_row_items * (height_item + length_border) < -slide_value) || (rel_pos / length_row_items * (height_item + length_border) + height_item > -slide_value + (SCREEN_HEIGHT - START_Y))) {			
			animate_slide = true;
			if (rel_pos /  length_row_items == file_list.length/ length_row_items)
				Position = - (rel_pos /  length_row_items * (height_item +  length_border)) - length_border;	
			else
				Position = - (rel_pos /  length_row_items * (height_item +  length_border));// - length_border;				
		}
}

float easeOut_T(float x0, float x1, float a) {
	float dx = (x1 - x0);	
	if (dx > 0)
		return ((dx * a) > 0.91f) ? (dx * a) : dx;
	else
	 	return ((dx * a) < -1) ? (dx * a) : dx;	
}

bool ftp_on = false;
void drawnotification() {
	float y = - SCREEN_HEIGHT + slide_value_notification;
	if ( y < 0)		
		vita2d_draw_rectangle(0, y , SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
	else
		vita2d_draw_rectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLACK);
	
	//Example 1: FTP
	
	vita2d_draw_rectangle(SHELL_MARGIN_X_CUSTOM, y + START_Y  + 10, width_item_notification, height_item_notification, VIOLET);

	// Dialog	
	if (TOUCH_FRONT() == 1) {
		if (toggle_item_notification[0]) {				
			ftp_on = true;		
		}
		else  {
			ftp_on = false;
			if (ftpvita_is_initialized()) {
				powerUnlock();
				ftpvita_fini();		
				vita2d_draw_rectangle(SHELL_MARGIN_X_CUSTOM + width_item_notification - 20, y + START_Y + 10 , 10, height_item_notification, RED);		
				dialog_step = DIALOG_STEP_NONE;
			}

			
			
		}
	}
			


		
	

	if (!ftpvita_is_initialized() & ftp_on) {
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
				ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);
			}

			vita2d_draw_rectangle(SHELL_MARGIN_X_CUSTOM + width_item_notification - 20, y + START_Y + 10 , 10, height_item_notification, VIOLET);	

			// Lock power timers
			powerLock();
		}		

	
	

	if (ftpvita_is_initialized()) {			
		char vita_ip_and_port[40];
		//Not show vita port?
		sprintf(vita_ip_and_port, vita_ip, &vita_port );
		pgf_draw_text(SHELL_MARGIN_X_CUSTOM + 5.0f, y + START_Y  + 10 + FONT_Y_SPACE2  , WHITE, FONT_SIZE, vita_ip_and_port);

		//Draw light green when ftp on
		vita2d_draw_rectangle(SHELL_MARGIN_X_CUSTOM + width_item_notification - 20, y + START_Y + 10 , 10, height_item_notification, GREEN);		
	}
	else {
		//Draw light red when ftp off
			vita2d_draw_rectangle(SHELL_MARGIN_X_CUSTOM + width_item_notification - 20, y + START_Y + 10 , 10, height_item_notification, RED);	
	}
	pgf_draw_text(SHELL_MARGIN_X_CUSTOM + 5.0f, y + START_Y  + 10  , WHITE, FONT_SIZE, "FTP");


	//End Example 1
}

void initnotification() {		
		drawnotification();

		if (notification_open){
				slide_value_notification += easeOut_T(slide_value_notification, SCREEN_HEIGHT,  0.375f);
				if (slide_value_notification >= (SCREEN_HEIGHT * 98)/ 100) {
					slide_value_notification = SCREEN_HEIGHT;
					notification_on = true;
					//notification_start = false;
					notification_open = false;									
				}			
			}
		else
		if (notification_close){
				slide_value_notification -= easeOut_T(HEIGHT_TITLE_BAR, slide_value_notification, 0.375f);
				if (slide_value_notification <= HEIGHT_TITLE_BAR + 10){
					slide_value_notification = HEIGHT_TITLE_BAR;
					notification_on = false;
					notification_start = false;
					notification_close = false;					
				}					
		}

		if (touch.reportNum == 0) {
			if (!notification_on){
				if(slide_value_notification >= (SCREEN_HEIGHT * 33) / 100)  {										
						notification_open = true;				
					}
					else {
						notification_close = true;
					}					
			}
			else {
				if(slide_value_notification <= (SCREEN_HEIGHT * 66) / 100)  {
					notification_close = true;
				}
				else {
					notification_open = true;
				}
			}
			
		
		}							
			//if (TOUCH_FRONT() == 1) {
				
						
			//}									
}

void draw_length_free_size(uint64_t free_size, uint64_t max_size, int x, int y) {		
	int length_free_size = (width_item * free_size) / max_size;								
	//return length_free_size;														
	if ((100 * length_free_size) / width_item >= 90)
		vita2d_draw_rectangle(x , y + slide_value , length_free_size , 5.0f , RED);
	else
		vita2d_draw_rectangle(x , y + slide_value , length_free_size , 5.0f , GREEN);				
}

void slide_to_location( float speed) {
	if (animate_slide) {	
			if (slide_value < Position){
				slide_value -= easeOut_T(Position , slide_value , speed);
				if (slide_value >= Position) {					
					animate_slide = false;
					Position = 0;
				}
			}
			else {
				slide_value -= easeOut_T(Position, slide_value , speed);
				if (slide_value <= Position) {					
					animate_slide = false;
					Position = 0;
				}
			}
	}
}

void refreshUI2(){
	slide_value = 0;
	rel_pos = 0;
	//x_rel_pos_old = SHELL_MARGIN_X_CUSTOM;
	//y_rel_pos_old = START_Y + length_border;
	width_item = ((SCREEN_WIDTH - SHELL_MARGIN_X_CUSTOM) / length_row_items) - length_border - max_extend_item_value/length_row_items;
	height_item = (17 * width_item) / 15;
	MAX_NAME_WIDTH_TILE = width_item;	
}

/////////////////////////////////////////////////////////////////////////////////////////
static void dirLevelUp() {
	base_pos_list[dir_level] = base_pos;
	rel_pos_list[dir_level] = rel_pos;
	dir_level++;
	base_pos_list[dir_level] = 0;
	rel_pos_list[dir_level] = 0;
	base_pos = 0;
	rel_pos = 0;
}

static void dirUpCloseArchive() {
	if (isInArchive() && dir_level_archive >= dir_level) {
		is_in_archive = 0;
		archiveClose();
		dir_level_archive = -1;
	}
}

static void dirUp() {
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

static void focusOnFilename(char *name) {
	int name_pos = fileListGetNumberByName(&file_list, name);
	if (name_pos < file_list.length) {
		while (1) {
			int index = base_pos + rel_pos;
			if (index == name_pos)
				break;

			if (index > name_pos) {
				if (rel_pos > 0) {
					rel_pos--;
				} else {
					if (base_pos > 0) {
						base_pos--;
					}
				}
			}

			if (index < name_pos) {
				if ((rel_pos + 1) < file_list.length) {
					if ((rel_pos + 1) < MAX_POSITION) {
						rel_pos++;
					} else {
						if ((base_pos + rel_pos + 1) < file_list.length) {
							base_pos++;
						}
					}
				}
			}
		}
	}
}

static int refreshFileList() {
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

static void refreshMarkList() {
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
		memset(&stat, 0, sizeof(SceIoStat));
		if (sceIoGetstat(path, &stat) < 0)
			fileListRemoveEntry(&mark_list, entry);

		// Next
		entry = next;
	}
}

static void refreshCopyList() {
	FileListEntry *entry = copy_list.head;

	int length = copy_list.length;

	int i;
	for (i = 0; i < length; i++) {
		// Get next entry already now to prevent crash after entry is removed
		FileListEntry *next = entry->next;

		char path[MAX_PATH_LENGTH];
		snprintf(path, MAX_PATH_LENGTH, "%s%s", copy_list.path, entry->name);

		// Check if the entry still exits. If not, remove it from list
		SceIoStat stat;
		memset(&stat, 0, sizeof(SceIoStat));
		int res = sceIoGetstat(path, &stat);
		if (res < 0 && res != 0x80010014)
			fileListRemoveEntry(&copy_list, entry);

		// Next
		entry = next;
	}
}

static int handleFile(char *file, FileListEntry *entry) {
	int res = 0;

	int type = getFileType(file);
	switch (type) {
		case FILE_TYPE_MP3:
		case FILE_TYPE_OGG:
		case FILE_TYPE_VPK:
		case FILE_TYPE_ZIP:
			if (isInArchive())
				type = FILE_TYPE_UNKNOWN;

			break;
	}

	switch (type) {
		case FILE_TYPE_INI:
		case FILE_TYPE_TXT:
		case FILE_TYPE_XML:
		case FILE_TYPE_UNKNOWN:
			res = textViewer(file);
			break;
			
		case FILE_TYPE_BMP:
		case FILE_TYPE_PNG:
		case FILE_TYPE_JPEG:
			res = photoViewer(file, type, &file_list, entry, &base_pos, &rel_pos);
			break;

		case FILE_TYPE_MP3:
		case FILE_TYPE_OGG:
			res = audioPlayer(file, type, &file_list, entry, &base_pos, &rel_pos);
			break;

		case FILE_TYPE_VPK:
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			break;
			
		case FILE_TYPE_ZIP:					
			res = archiveOpen(file);							
			break;

		case FILE_TYPE_SFO:
			res = SFOReader(file);
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
	MENU_ENTRY_EMPTY_5,
	MENU_ENTRY_MORE,
};

static MenuEntry menu_entries[] = {
	{ MARK_ALL, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ MOVE, CTX_VISIBILITY_INVISIBLE },
	{ COPY, CTX_VISIBILITY_INVISIBLE },
	{ PASTE, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ DELETE, CTX_VISIBILITY_INVISIBLE },
	{ RENAME, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ NEW_FOLDER, CTX_VISIBILITY_INVISIBLE },
	{ -1, CTX_VISIBILITY_UNUSED },
	{ MORE, CTX_VISIBILITY_INVISIBLE }
};

#define N_MENU_ENTRIES (sizeof(menu_entries) / sizeof(MenuEntry))

enum MenuMoreEntrys {
	MENU_MORE_ENTRY_INSTALL_ALL,
	MENU_MORE_ENTRY_INSTALL_FOLDER,
	MENU_MORE_ENTRY_EXPORT_MEDIA,
	MENU_MORE_ENTRY_CALCULATE_SHA1,
};

static MenuEntry menu_more_entries[] = {
	{ INSTALL_ALL, CTX_VISIBILITY_INVISIBLE },
	{ INSTALL_FOLDER, CTX_VISIBILITY_INVISIBLE },
	{ EXPORT_MEDIA, CTX_VISIBILITY_INVISIBLE },
	{ CALCULATE_SHA1, CTX_VISIBILITY_INVISIBLE },
};

#define N_MENU_MORE_ENTRIES (sizeof(menu_more_entries) / sizeof(MenuEntry))

enum ListToolboxEntrys {
	LIST_TOOLBOX_ENTRY_SYSINFO,
};

static ListEntry list_toolbox_entries[] = {
	{ SYSINFO_TITLE },
};

#define N_LIST_TOOLBOX_ENTRIES (sizeof(list_toolbox_entries) / sizeof(ListEntry))

static void setContextMenuVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
			menu_entries[i].visibility = CTX_VISIBILITY_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_entries[MENU_ENTRY_MARK_UNMARK_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_MOVE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_COPY].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Invisible 'Paste' if nothing is copied yet
	if (copy_list.length == 0)
		menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;

	// Invisible 'Paste' if the files to move are not from the same partition
	if (copy_mode == COPY_MODE_MOVE) {
		char *p = strchr(file_list.path, ':');
		char *q = strchr(copy_list.path, ':');
		if (p && q) {
			*p = '\0';
			*q = '\0';

			if (strcasecmp(file_list.path, copy_list.path) != 0) {
				menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
			}

			*q = ':';
			*p = ':';
		} else {
			menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
		}
	}

	// Invisble write operations in archives
	if (isInArchive()) { // TODO: read-only mount points
		menu_entries[MENU_ENTRY_MOVE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_PASTE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_DELETE].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_RENAME].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_entries[MENU_ENTRY_NEW_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
	}

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
		if (menu_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
			setContextMenuPos(i);
			break;
		}
	}

	if (i == N_MENU_ENTRIES)
		setContextMenuPos(-1);
}

static void setContextMenuMoreVisibilities() {
	int i;

	// All visible
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_VISIBILITY_INVISIBLE)
			menu_more_entries[i].visibility = CTX_VISIBILITY_VISIBLE;
	}

	FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

	// Invisble entries when on '..'
	if (strcmp(file_entry->name, DIR_UP) == 0) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Invisble operations in archives
	if (isInArchive()) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_VISIBILITY_INVISIBLE;
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	if (file_entry->is_folder) {
		menu_more_entries[MENU_MORE_ENTRY_CALCULATE_SHA1].visibility = CTX_VISIBILITY_INVISIBLE;
		do {
			char check_path[MAX_PATH_LENGTH];
			SceIoStat stat;
			int statres;
			if (strcmp(file_list.path, "ux0:app/") == 0 || strcmp(file_list.path, "ux0:patch/") == 0) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
				break;
			}
			snprintf(check_path, MAX_PATH_LENGTH, "%s%s/eboot.bin", file_list.path, file_entry->name);
			statres = sceIoGetstat(check_path, &stat);
			if (statres < 0 || SCE_S_ISDIR(stat.st_mode)) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
				break;
			}
			snprintf(check_path, MAX_PATH_LENGTH, "%s%s/sce_sys/param.sfo", file_list.path, file_entry->name);
			statres = sceIoGetstat(check_path, &stat);
			if (statres < 0 || SCE_S_ISDIR(stat.st_mode)) {
				menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
				break;
			}
		} while(0);
	} else {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_FOLDER].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	if(file_entry->type != FILE_TYPE_VPK) {
		menu_more_entries[MENU_MORE_ENTRY_INSTALL_ALL].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Invisible export for non-media files
	if (!file_entry->is_folder && file_entry->type != FILE_TYPE_BMP && file_entry->type != FILE_TYPE_JPEG && file_entry->type != FILE_TYPE_PNG && file_entry->type != FILE_TYPE_MP3) {
		menu_more_entries[MENU_MORE_ENTRY_EXPORT_MEDIA].visibility = CTX_VISIBILITY_INVISIBLE;
	}

	// Go to first entry
	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility == CTX_VISIBILITY_VISIBLE) {
			setContextMenuMorePos(i);
			break;
		}
	}

	if (i == N_MENU_MORE_ENTRIES)
		setContextMenuMorePos(-1);
}

static int listToolboxEnterCallback(int pos) {
	switch (pos) {
		case LIST_TOOLBOX_ENTRY_SYSINFO:
		{
			// System software version
			SceSystemSwVersionParam sw_ver_param;
			sw_ver_param.size = sizeof(SceSystemSwVersionParam);
			sceKernelGetSystemSwVersion(&sw_ver_param);

			// MAC address
			SceNetEtherAddr mac;
			sceNetGetMacAddress(&mac, 0);

			char mac_string[32];
			sprintf(mac_string, "%02X:%02X:%02X:%02X:%02X:%02X", mac.data[0], mac.data[1], mac.data[2], mac.data[3], mac.data[4], mac.data[5]);

			// Get IP
			char ip[16];

			SceNetCtlInfo info;
			if (sceNetCtlInetGetInfo(SCE_NETCTL_INFO_GET_IP_ADDRESS, &info) < 0) {
				strcpy(ip, "-");
			} else {
				strcpy(ip, info.ip_address);
			}

			// Memory card
			uint64_t free_size = 0, max_size = 0;
			sceAppMgrGetDevInfo("ux0:", &max_size, &free_size);

			char free_size_string[16], max_size_string[16];
			getSizeString(free_size_string, free_size);
			getSizeString(max_size_string, max_size);

			// System information dialog
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[SYS_INFO], sw_ver_param.version_string, sceKernelGetModelForCDialog(), mac_string, ip, free_size_string, max_size_string, scePowerGetBatteryLifePercent());
			dialog_step = DIALOG_STEP_SYSTEM;
		}
	}

	return LIST_DIALOG_CLOSE;
}

static int contextMenuEnterCallback(int pos, void* context) {
	switch (pos) {
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
			if (pos == MENU_ENTRY_MOVE) {
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
		{
			int copy_text = 0;

			switch (copy_mode) {
				case COPY_MODE_NORMAL:
					copy_text = COPYING;
					break;
					
				case COPY_MODE_MOVE:
					copy_text = MOVING;
					break;
					
				case COPY_MODE_EXTRACT:
					copy_text = EXTRACTING;
					break;
			}

			initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[copy_text]);
			dialog_step = DIALOG_STEP_PASTE;
			break;
		}

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

			initImeDialog(language_container[RENAME], name, MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);

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

			initImeDialog(language_container[NEW_FOLDER], path + strlen(file_list.path), MAX_NAME_LENGTH, SCE_IME_TYPE_BASIC_LATIN, 0);
			dialog_step = DIALOG_STEP_NEW_FOLDER;
			break;
		}
		
		case MENU_ENTRY_MORE:
		{
			setContextMenuMoreVisibilities();
			return CONTEXT_MENU_MORE_OPENING;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

static int contextMenuMoreEnterCallback(int pos, void* context) {
	switch (pos) {
		case MENU_MORE_ENTRY_INSTALL_ALL:
		{
			// Empty install list
			fileListEmpty(&install_list);

			FileListEntry *file_entry = file_list.head->next; // Ignore '..'

			int i;
			for (i = 0; i < file_list.length - 1; i++) {
				char path[MAX_PATH_LENGTH];
				snprintf(path, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				int type = getFileType(path);
				if (type == FILE_TYPE_VPK) {
					FileListEntry *install_entry = malloc(sizeof(FileListEntry));
					memcpy(install_entry, file_entry, sizeof(FileListEntry));
					fileListAddEntry(&install_list, install_entry, SORT_NONE);
				}

				// Next
				file_entry = file_entry->next;
			}

			strcpy(install_list.path, file_list.path);

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_ALL_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			
			break;
		}

		case MENU_MORE_ENTRY_INSTALL_FOLDER:
		{
			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[INSTALL_FOLDER_QUESTION]);
			dialog_step = DIALOG_STEP_INSTALL_QUESTION;
			break;
		}
		
		case MENU_MORE_ENTRY_EXPORT_MEDIA:
		{
			char *message;

			FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

			// On marked entry
			if (fileListFindEntry(&mark_list, file_entry->name)) {
				if (mark_list.length == 1) {
					message = language_container[file_entry->is_folder ? EXPORT_FOLDER_QUESTION : EXPORT_FILE_QUESTION];
				} else {
					message = language_container[EXPORT_FILES_FOLDERS_QUESTION];
				}
			} else {
				message = language_container[file_entry->is_folder ? EXPORT_FOLDER_QUESTION : EXPORT_FILE_QUESTION];
			}

			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, message);
			dialog_step = DIALOG_STEP_EXPORT_QUESTION;
			break;
		}
		
		case MENU_MORE_ENTRY_CALCULATE_SHA1:
		{
			// Ensure user wants to actually take the hash
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_YESNO, language_container[HASH_FILE_QUESTION]);
			dialog_step = DIALOG_STEP_HASH_QUESTION;
			break;
		}
	}

	return CONTEXT_MENU_CLOSING;
}

static void initFtp() {
	// Add all the current mountpoints to ftpvita
	int i;
	for (i = 0; i < getNumberMountPoints(); i++) {
		char **mount_points = getMountPoints();
		if (mount_points[i]) {
			ftpvita_add_device(mount_points[i]);
		}
	}

	ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);	
}

static int dialogSteps() {
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
			
		case DIALOG_STEP_FTP_WAIT:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				int state = 0;
				sceNetCtlInetGetState(&state);
				if (state == 3) {
					int res = ftpvita_init(vita_ip, &vita_port);
					if (res >= 0) {
						initFtp();
						sceMsgDialogClose();
					}
				}
			} else {
				if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
					dialog_step = DIALOG_STEP_NONE;

					// Dialog
					if (ftpvita_is_initialized()) {
						initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
						dialog_step = DIALOG_STEP_FTP;
					}
				}
			}
			
			break;
			
		case DIALOG_STEP_FTP:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				refresh = 1;
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

				dialog_step = DIALOG_STEP_COPYING;

				SceUID thid = sceKernelCreateThread("copy_thread", (SceKernelThreadEntry)copy_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(CopyArguments), &args);
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

				dialog_step = DIALOG_STEP_DELETING;

				SceUID thid = sceKernelCreateThread("delete_thread", (SceKernelThreadEntry)delete_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(DeleteArguments), &args);
			}

			break;
			
		case DIALOG_STEP_EXPORT_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[EXPORTING]);
				dialog_step = DIALOG_STEP_EXPORT_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}

			break;
			
		case DIALOG_STEP_EXPORT_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				ExportArguments args;
				args.file_list = &file_list;
				args.mark_list = &mark_list;
				args.index = base_pos + rel_pos;

				dialog_step = DIALOG_STEP_EXPORTING;

				SceUID thid = sceKernelCreateThread("export_thread", (SceKernelThreadEntry)export_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(ExportArguments), &args);
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

					if (strcasecmp(old_name, name) == 0) { // No change
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

		case DIALOG_STEP_HASH_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				// Throw up the progress bar, enter hashing state
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[HASHING]);
				dialog_step = DIALOG_STEP_HASH_CONFIRMED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				// Quit
				dialog_step = DIALOG_STEP_NONE;
			}

			break;

		case DIALOG_STEP_HASH_CONFIRMED:
			if (msg_result == MESSAGE_DIALOG_RESULT_RUNNING) {
				// User has confirmed desire to hash, get requested file entry
				FileListEntry *file_entry = fileListGetNthEntry(&file_list, base_pos + rel_pos);

				// Place the full file path in cur_file
				snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);

				HashArguments args;
				args.file_path = cur_file;

				dialog_step = DIALOG_STEP_HASHING;

				// Create a thread to run out actual sum
				SceUID thid = sceKernelCreateThread("hash_thread", (SceKernelThreadEntry)hash_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(HashArguments), &args);
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

				if (install_list.length > 0) {
					FileListEntry *entry = install_list.head;
					snprintf(install_path, MAX_PATH_LENGTH, "%s%s", install_list.path, entry->name);
					args.file = install_path;

					// Focus
					focusOnFilename(entry->name);

					// Remove entry
					fileListRemoveEntry(&install_list, entry);
				} else {
					args.file = cur_file;
				}

				dialog_step = DIALOG_STEP_INSTALLING;

				SceUID thid = sceKernelCreateThread("install_thread", (SceKernelThreadEntry)install_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, sizeof(InstallArguments), &args);
			}

			break;
			
		case DIALOG_STEP_INSTALL_WARNING:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				dialog_step = DIALOG_STEP_INSTALL_WARNING_AGREED;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_CANCELLED;
			}

			break;
			
		case DIALOG_STEP_INSTALLED:
			if (msg_result == MESSAGE_DIALOG_RESULT_NONE || msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				if (install_list.length > 0) {
					initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);
					dialog_step = DIALOG_STEP_INSTALL_CONFIRMED;
					break;
				}

				dialog_step = DIALOG_STEP_NONE;
				refresh = 1;
			}

			break;
			
		case DIALOG_STEP_UPDATE_QUESTION:
			if (msg_result == MESSAGE_DIALOG_RESULT_YES) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[DOWNLOADING]);
				dialog_step = DIALOG_STEP_DOWNLOADING;
			} else if (msg_result == MESSAGE_DIALOG_RESULT_NO) {
				dialog_step = DIALOG_STEP_NONE;
			}
			
		case DIALOG_STEP_DOWNLOADED:
			if (msg_result == MESSAGE_DIALOG_RESULT_FINISHED) {
				initMessageDialog(MESSAGE_DIALOG_PROGRESS_BAR, language_container[INSTALLING]);

				dialog_step = DIALOG_STEP_EXTRACTING;

				SceUID thid = sceKernelCreateThread("update_extract_thread", (SceKernelThreadEntry)update_extract_thread, 0x40, 0x10000, 0, 0, NULL);
				if (thid >= 0)
					sceKernelStartThread(thid, 0, NULL);
			}

			break;
			
		case DIALOG_STEP_EXTRACTED:
			launchAppByUriExit("VSUPDATER");
			dialog_step = DIALOG_STEP_NONE;
			break;
	}

	return refresh;
}

static void fileBrowserMenuCtrl2() {	
	if(!notification_on){

	// Show toolbox list dialog
	if (current_buttons & SCE_CTRL_START) {
		if (getListDialogMode() == LIST_DIALOG_CLOSE) {
			toolbox_list.title = TOOLBOX;
			toolbox_list.list_entries = list_toolbox_entries;
			toolbox_list.n_list_entries = N_LIST_TOOLBOX_ENTRIES;
			toolbox_list.can_escape = 1;
			toolbox_list.listSelectCallback = listToolboxEnterCallback;

			loadListDialog(&toolbox_list);
		}
	}

	// Change UI
	if (pressed_buttons & SCE_CTRL_RTRIGGER) {		
		Change_UI = false;
	}

	if (current_buttons & SCE_CTRL_LTRIGGER && pressed_buttons & SCE_CTRL_UP ) {
		if (length_row_items + 1 < 9) {
			length_row_items++;
			refreshUI2();
		}
	}

	if (current_buttons & SCE_CTRL_LTRIGGER && pressed_buttons & SCE_CTRL_DOWN ) {
		if (length_row_items - 1 > 5) {
			length_row_items--;
			refreshUI2();
		}
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
				ftpvita_ext_add_custom_command("PROM", ftpvita_PROM);
			}

			// Lock power timers
			powerLock();
		}

		// Dialog
		if (ftpvita_is_initialized()) {
			initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK_CANCEL, language_container[FTP_SERVER], vita_ip, vita_port);
			dialog_step = DIALOG_STEP_FTP;
		}
	}

		
	if (TOUCH_FRONT() == 0)  {
			//Reset state everytime TOUCH_FRONT called
			State_Touch = -1;
			if (lerp(touch.report[0].y, 1088, SCREEN_HEIGHT) > START_Y) {									
				have_touch = true;		  			
				int i = 0;
				int column = 0;
				int row = 0;
				float t_y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT) - height_item - slide_value;
				float t_x = lerp(touch.report[0].x, 1920, SCREEN_WIDTH) - width_item;			  	  
				while (true) {			 
					float x = SHELL_MARGIN_X_CUSTOM + (column * width_item);				  		  
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
							if (column >= length_row_items) {
								column = 0;						
								row ++;
								
							}
					}
					// Exit when not found
					if (i >= file_list.length) {
						touch_nothing = true;
						break;
					}		
				}	
			}		
		}


		//Slide up
		if (TOUCH_FRONT() == 6) {
			//Reset state everytime TOUCH_FRONT called		
			State_Touch = -1;
				
			if ((slide_value  > -slide_value_limit)) {
				if (slide_value_limit > SCREEN_HEIGHT - START_Y - height_item) {	
					slide_value -=  slide_value - (-(pre_touch_y - touch.report[0].y) + slide_value_hold);
					if (slide_value  < -slide_value_limit)
						slide_value = -slide_value_limit ;								
				}
			}
			else {
				
				//pre_touch_y = touch.report[0].y;
				animate_slide = true;
				Position = -slide_value_limit;			
			}
			
		}

		//Slide down
		if (TOUCH_FRONT() == 7)  {
			//Reset state everytime TOUCH_FRONT called
			State_Touch = -1;
			if ((pre_touch_y < HEIGHT_TITLE_BAR) ) {
				float n_y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT);
				slide_value_notification = n_y;
				notification_start = true;
			}
			else

			if((slide_value < 0)) {	
				slide_value -= slide_value - ((touch.report[0].y - pre_touch_y) + slide_value_hold);
				if (slide_value  < -slide_value_limit)
						slide_value = 0;						
			}
			else {
				
				//pre_touch_y = touch.report[0].y;			
				animate_slide = true;
				Position = 0;			
			}
		}

		// Move
		if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {				
			if ((rel_pos - length_row_items) > 0) {			
				rel_pos -= length_row_items;
			}
			else {
				rel_pos = 0;				
			}
			return_current_pos();	
		} else if (hold_buttons & SCE_CTRL_DOWN || hold2_buttons & SCE_CTRL_LEFT_ANALOG_DOWN ) {									
			if ((rel_pos + length_row_items) < file_list.length) {
				rel_pos += length_row_items;				
			}
			else {
						rel_pos = file_list.length - 1;
					}
			return_current_pos();		
		} else if (hold_buttons & SCE_CTRL_RIGHT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_RIGHT) {
			if ((rel_pos + 1) < file_list.length) {
				if ((rel_pos + 1) < file_list.length) {
					rel_pos++;
				} 
			}
			return_current_pos();
		} else if (hold_buttons & SCE_CTRL_LEFT || hold2_buttons & SCE_CTRL_LEFT_ANALOG_LEFT) {
			if (rel_pos > 0) {
				rel_pos--;
			} 
			return_current_pos();
		}

		// Not at 'home'
	if (dir_level > 0) {
		// Context menu trigger
		if (pressed_buttons & SCE_CTRL_TRIANGLE) {
			if (getContextMenuMode() == CONTEXT_MENU_CLOSED) {
				setContextMenuVisibilities();
				setContextMenuMode(CONTEXT_MENU_OPENING);
			}
		}

			// Mark entry
			if ((pressed_buttons & SCE_CTRL_SQUARE) || ((TOUCH_FRONT() == 3) & !have_touch_hold & !touch_nothing )) {
				State_Touch = -1;

				have_touch_hold = true;
				FileListEntry *file_entry = fileListGetNthEntry(&file_list, rel_pos);
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
			if (pressed_buttons & SCE_CTRL_CANCEL || (TOUCH_FRONT() == 10)) {
				State_Touch = -1;
				//reset position to top
				slide_value = 0;
				Position = 0;

				fileListEmpty(&mark_list);
				dirUp();
				refreshFileList();
			}
		}

		if (pressed_buttons & SCE_CTRL_ENTER || ((TOUCH_FRONT() == 1) & (lerp(touch.report[0].y, 1088, SCREEN_HEIGHT) > START_Y))) {
		State_Touch = -1;

		if (touch_nothing || have_touch_hold) {			
				touch_nothing = false;
				have_touch_hold = false;
				return;
		}

		fileListEmpty(&mark_list);

		// Handle file or folder
		FileListEntry *file_entry = fileListGetNthEntry(&file_list,  rel_pos);
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

			// Save last dir
			WriteFile(VITASHELL_LASTDIR, file_list.path, strlen(file_list.path) + 1);

			// Open folder
			int res = refreshFileList();
			if (res < 0)
				errorDialog(res);
		} else {
			snprintf(cur_file, MAX_PATH_LENGTH, "%s%s", file_list.path, file_entry->name);
			int type = handleFile(cur_file, file_entry);
			
			// Archive mode
			if (type == FILE_TYPE_ZIP) {
				slide_value = 0;	
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
	else {
		if (TOUCH_FRONT() == 0)  {
			//Reset state everytime TOUCH_FRONT called
			State_Touch = -1;
			have_touch = true;
			float x = lerp(touch.report[0].x, 1920, SCREEN_WIDTH);		  					
			float y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT);
			

			if ((x > SHELL_MARGIN_X_CUSTOM) & (x < SHELL_MARGIN_X_CUSTOM + width_item_notification))
				if ((y > START_Y  + 10) & (y < START_Y  + 10 + height_item_notification)) {
					
						toggle_item_notification[0] = !toggle_item_notification[0];
					 
				}
					
		}
		
		//Slide up close notification	
		if (TOUCH_FRONT() == 6) {
			//Reset state everytime TOUCH_FRONT called		
			State_Touch = -1;		
			if (pre_touch_y > SCREEN_HEIGHT - HEIGHT_TITLE_BAR) {
				float n_y = lerp(touch.report[0].y, 1088, SCREEN_HEIGHT);
				slide_value_notification = n_y;	
			}
		}

		if (hold_buttons & SCE_CTRL_UP || hold2_buttons & SCE_CTRL_LEFT_ANALOG_UP) {
			notification_close = true;
		}
		
	}
	
}

//Need Rework
void drawScrollBar2(int file_list_length) {
	if (round(file_list_length / length_row_items) * (height_item + length_border) > SCREEN_HEIGHT - START_Y) {
		
		float view_size = SCREEN_HEIGHT - START_Y;
		float max_scroll = ((file_list_length / length_row_items) * (height_item + length_border)) + view_size - height_item;
		vita2d_draw_rectangle(SCROLL_BAR_X, START_Y + 10.0f , SCROLL_BAR_WIDTH, view_size - FONT_Y_SPACE2 - 15.0f, SCROLL_BAR_BG_COLOR);// COLOR_ALPHA(GRAY, 0xC8));	
		
		float height = ( (view_size - FONT_Y_SPACE2 - 15.0f) * view_size)/ max_scroll ;
		float y = ( (-slide_value ) * (view_size - FONT_Y_SPACE2 - 15.0f )  )  / max_scroll  + START_Y + 10.0f;
		if (y < START_Y + 10.0f) y = START_Y + 10.0f;
		//if (y > view_size - FONT_Y_SPACE2 - 15.0f) y = view_size - FONT_Y_SPACE2 - 15.0f;
				
		vita2d_draw_rectangle(SCROLL_BAR_X, y, SCROLL_BAR_WIDTH, height, SCROLL_BAR_COLOR);// COLOR_ALPHA(WHITE, 0x64));
	}
}

void drawShellInfo2(char *path) {
	// Title
	char version[8];
	sprintf(version, "%X.%02X", VITASHELL_VERSION_MAJOR, VITASHELL_VERSION_MINOR);
	if (version[3] == '0')
		version[3] = '\0';

	vita2d_draw_texture(title_bar_bg_image, 0, slide_value_notification - HEIGHT_TITLE_BAR);

	pgf_draw_textf(SHELL_MARGIN_X_CUSTOM, SHELL_MARGIN_Y_CUSTOM, TITLE_COLOR, FONT_SIZE, "VitaShell %s", version);

	// Battery
	float battery_x = ALIGN_RIGHT(SCREEN_WIDTH - SHELL_MARGIN_X_CUSTOM, vita2d_texture_get_width(battery_image));
	vita2d_draw_texture(battery_image, battery_x, SHELL_MARGIN_Y_CUSTOM + 3.0f);

	vita2d_texture *battery_bar_image = battery_bar_green_image;

	if (scePowerIsLowBattery() && !scePowerIsBatteryCharging()) {
		battery_bar_image = battery_bar_red_image;
	} 

	float percent = scePowerGetBatteryLifePercent() / 100.0f;

	float width = vita2d_texture_get_width(battery_bar_image);
	vita2d_draw_texture_part(battery_bar_image, battery_x + 3.0f + (1.0f - percent) * width, SHELL_MARGIN_Y_CUSTOM + 5.0f, (1.0f - percent) * width, 0.0f, percent * width, vita2d_texture_get_height(battery_bar_image));

	if (scePowerIsBatteryCharging()) {
		vita2d_draw_texture(battery_bar_charge_image, battery_x + 3.0f, SHELL_MARGIN_Y_CUSTOM + 5.0f);
	}

	// Date & time
	SceDateTime time;
	sceRtcGetCurrentClock(&time, 0);

	char date_string[16];
	getDateString(date_string, date_format, &time);

	char time_string[24];
	getTimeString(time_string, time_format, &time);

	char string[64];
	sprintf(string, "%s  %s", date_string, time_string);
	float date_time_x = ALIGN_RIGHT(battery_x - 12.0f, vita2d_pgf_text_width(font, FONT_SIZE, string));
	pgf_draw_text(date_time_x, SHELL_MARGIN_Y_CUSTOM, DATE_TIME_COLOR, FONT_SIZE, string);

	// WIFI
	int state = 0;
	sceNetCtlInetGetState(&state);
	if (state == 3)
		vita2d_draw_texture(wifi_image, date_time_x - 60.0f, SHELL_MARGIN_Y_CUSTOM + 3.0f);

	// FTP
	if (ftpvita_is_initialized())
		vita2d_draw_texture(ftp_image, date_time_x - 30.0f, SHELL_MARGIN_Y_CUSTOM + 3.0f);

	// TODO: make this more elegant
	// Path
	if (!notification_start) {
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

		pgf_draw_text(SHELL_MARGIN_X_CUSTOM, PATH_Y, PATH_COLOR, FONT_SIZE, path_first_line);
		pgf_draw_text(SHELL_MARGIN_X_CUSTOM, PATH_Y + FONT_Y_SPACE, PATH_COLOR, FONT_SIZE, path_second_line);

		char str[128];

		// Show numbers of files and folders
		// sprintf(str, "%d files and %d folders", file_list.files, file_list.folders);

		// Draw on bottom left
		// pgf_draw_textf(ALIGN_RIGHT(SCREEN_WIDTH - SHELL_MARGIN_X_CUSTOM, vita2d_pgf_text_width(font, FONT_SIZE, str)), SCREEN_HEIGHT - SHELL_MARGIN_Y_CUSTOM - FONT_Y_SPACE - 2.0f, LITEGRAY, FONT_SIZE, str);
	}
}

/////////////////////////////////////////////////////////////////////////////////////////////


void DrawItem(FileListEntry *file_entry, float x, float y, float extend_item_value) {
			
																																	
				
				//Special Folder
				if (strcmp(file_entry->name, "gro0:") == 0) {				
					vita2d_draw_texture_scale(game_card_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));											
				} else
				if (strcmp(file_entry->name, "grw0:") == 0) {				
					vita2d_draw_texture_scale(game_card_storage_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));									
				} else
				if (strcmp(file_entry->name, "ux0:") == 0) {				
					vita2d_draw_texture_scale(memory_card_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));								
				} else
				if (strcmp(file_entry->name, "os0:") == 0) {				
					vita2d_draw_texture_scale(os0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));				
				} else
				if (strcmp(file_entry->name, "sa0:") == 0) {				
					vita2d_draw_texture_scale(sa0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));					
				} else
				if (strcmp(file_entry->name, "ur0:") == 0) {				
					vita2d_draw_texture_scale(ur0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));				
				} else
				if (strcmp(file_entry->name, "vd0:") == 0) {				
					vita2d_draw_texture_scale(vd0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));			
				} else
				if (strcmp(file_entry->name, "vs0:") == 0) {				
					vita2d_draw_texture_scale(vs0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));				
				} else
				if (strcmp(file_entry->name, "savedata0:") == 0) {				
					vita2d_draw_texture_scale(savedata0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));				
				} else
				if (strcmp(file_entry->name, "pd0:") == 0) {				
					vita2d_draw_texture_scale(pd0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));			
				} else
				if (strcmp(file_entry->name, "app0:") == 0) {				
					vita2d_draw_texture_scale(app0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));			
				} else
				if (strcmp(file_entry->name, "ud0:") == 0) {				
					vita2d_draw_texture_scale(ud0_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));				
				} 
				
				else {						
					// Folder default
					if (file_entry->is_folder) {
							/*char path_icon[100];							
							strcpy(path_icon,file_list.path);
							addEndSlash(path_icon);
							strcat(path_icon,file_entry->name);
							addEndSlash(path_icon);
							strcat(path_icon,"sce_sys");
							addEndSlash(path_icon);
							strcat(path_icon,"icon0.png");
							if( access(path_icon , F_OK ) != -1 ) {									
								vita2d_draw_rectangle( x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value , width_item, height_item , COLOR_ALPHA(GREEN, 0xC8));								
								//sceKernelStartThread(thid2, sizeof(path_icon), path_icon);
							}
							else*/
							if (strcmp(file_entry->name, DIR_UP) == 0)
								vita2d_draw_texture_scale(updir_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));
							else						
								vita2d_draw_texture_scale(folder_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));
						}
						else
						switch (file_entry->type) {
							case FILE_TYPE_BMP:
							case FILE_TYPE_PNG:
							case FILE_TYPE_JPEG:
								vita2d_draw_texture_scale(img_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));	
								break;
								
							case FILE_TYPE_VPK:
								vita2d_draw_texture_scale(run_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));
								break;

							case FILE_TYPE_ZIP:
								vita2d_draw_texture_scale(zip_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));	
								break;
								
							case FILE_TYPE_MP3:
								vita2d_draw_texture_scale(music_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));
								break;
								
							case FILE_TYPE_SFO:
								vita2d_draw_texture_scale(txt_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));	
								break;
							
							case FILE_TYPE_INI:
							case FILE_TYPE_TXT:
							case FILE_TYPE_XML:
								vita2d_draw_texture_scale(txt_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));	
								break;

							default:
								vita2d_draw_texture_scale(unknown_file_image,  x + 10.0f - extend_item_value, y + slide_value + 10.0f - extend_item_value, width_item*(0.008 + extend_item_value / 10000), width_item*(0.008 + extend_item_value / 10000));	
								break;
						}									  								
				}								

				if (dir_level == 0) {					
					draw_length_free_size(file_entry->size2 - file_entry->size, file_entry->size2, x - extend_item_value ,y - extend_item_value);
				}

				// File name
				int length = strlen(file_entry->name);
				int line_width = 0;

				int j;
				for (j = 0; j < length; j++) {
					char ch_width = font_size_cache[(int)file_entry->name[j]];

					// Too long
					if ((line_width + ch_width) >= MAX_NAME_WIDTH_TILE + extend_item_value)
						break;

					// Increase line width
					line_width += ch_width;
				}

								
			
				char ch = 0;

				if (j != length) {
					ch = file_entry->name[j];
					file_entry->name[j] = '\0';
				}

				// Draw shortened file name
				if (strcmp(file_entry->name, "..") != 0) {
					extend_item_value = (int) extend_item_value;								
					vita2d_draw_rectangle(x - extend_item_value - 1, y + height_item - FONT_Y_SPACE2 + slide_value + extend_item_value - 1,  2*extend_item_value + width_item, FONT_Y_SPACE2 - 1, COLOR_ALPHA(WHITE, 0xB4));																	
					pgf_draw_text(x + 2.0f - extend_item_value, y + height_item - FONT_Y_SPACE2 + slide_value + extend_item_value, COLOR_ALPHA(BLACK, 0xF0), FONT_SIZE, file_entry->name);
					
				}								
				
				if (j != length)
					file_entry->name[j] = ch;

				// Marked
				if (fileListFindEntry(&mark_list, file_entry->name)) {					
					vita2d_draw_rectangle(x - extend_item_value , y + slide_value - extend_item_value, width_item + 2*extend_item_value, height_item + 2*extend_item_value , COLOR_ALPHA(BLUE, 0x40));
					vita2d_draw_texture(mark_image, x + width_item - vita2d_texture_get_width(mark_image) - length_border + extend_item_value, y + slide_value + length_border - extend_item_value );									
				}

				if (extend_item_value > 0) 
					DrawBorderRectSimple(x - extend_item_value , y + slide_value - extend_item_value , width_item + 2*extend_item_value, height_item + 2*extend_item_value, GREEN, 4);										
				else 
					DrawBorderRectSimple(x, y + slide_value, width_item, height_item, 0xdddddd, 4);									
					
}

static void mainUI2() {				
		// Start drawing
		startDrawing(bg_browser_image);
		vita2d_draw_texture_part(default_wallpaper, 0, START_Y - 1 - max_extend_item_value , 0 , START_Y - 1 - max_extend_item_value, SCREEN_WIDTH, vita2d_texture_get_height(default_wallpaper));			

		// Draw scroll bar
		drawScrollBar2(file_list.length);

		// Draw
		FileListEntry *file_entry = fileListGetNthEntry(&file_list, 0);

		int i;
		int w = -1;
		int h = 0;
		int cur_pos = 0; // Current position
		float x_pos_item_old = 0; 
		float y_pos_item_old = 0;		
		for (i = 0; i < file_list.length && i < file_list.length; i++) {
			
			if ((i % length_row_items == 0) & (i != 0)) {
				w = 0;
				h++;							
			}
			else {
				w++;				
			}

			float x = SHELL_MARGIN_X_CUSTOM + (width_item * w);
			float y = START_Y + (h * height_item);
						

			x += length_border * w;
			y += length_border * (h + 1);															
			
			//if item out user view is NEXT_FILE
			if ((y + slide_value + height_item > 0) & (y + slide_value < SCREEN_HEIGHT))
			{
				// Current position
				if (i == rel_pos) {																																						
					cur_pos = i;				
				}
			
				if (have_touch) {
					DrawItem(file_entry, x, y, 0); 					
				}
				else {									
					if (i == rel_pos){
						x_pos_item_old = x;
						y_pos_item_old = y;														
					}
					else {
						DrawItem(file_entry, x, y, 0);							
					}
				}

				
			}
			else 
				goto NEXT_FILE;

			 
			

						
			
NEXT_FILE:			
			// Next
			file_entry = file_entry->next;
		}		
					
		FileListEntry *file_entry2 = fileListGetNthEntry(&file_list, cur_pos);

		//Draw Border
		if (!have_touch) {			
			if (animate_extend_item_pos != cur_pos)	{
				animate_extend_item = 0;
				animate_extend_item_pos = cur_pos;
			}
			else {
				if (animate_extend_item + 1 < max_extend_item_value)
					animate_extend_item++;
			}
			int a = x_pos_item_old - animate_extend_item;
			int b = y_pos_item_old - animate_extend_item + slide_value;

			vita2d_draw_texture_part(default_wallpaper, a- 1, b, a - 1, b, width_item + 2*animate_extend_item, height_item + 2*animate_extend_item);
			DrawItem(file_entry2, x_pos_item_old, y_pos_item_old, animate_extend_item);
			vita2d_draw_texture_part(default_wallpaper, 0, 0 ,0 , 0, SCREEN_WIDTH, START_Y - 1 - max_extend_item_value);						
		}	
		else
			vita2d_draw_texture_part(default_wallpaper, 0, 0 ,0 , 0, SCREEN_WIDTH, START_Y - 1);					

		// Draw shell info
		drawShellInfo2(file_list.path);	

		vita2d_draw_rectangle(0, SCREEN_HEIGHT - FONT_Y_SPACE2, SCREEN_WIDTH, FONT_Y_SPACE2, COLOR_ALPHA(BLACK, 0xC8));

		//Folder
		uint32_t color = FILE_COLOR;
		vita2d_texture *icon = NULL;

		if (file_entry2->is_folder) {
			color = FOLDER_COLOR;
			icon = folder_icon;
		} else {
			switch (file_entry2->type) {
					case FILE_TYPE_BMP:
					case FILE_TYPE_PNG:
					case FILE_TYPE_JPEG:
						color = IMAGE_COLOR;
						icon = image_icon;
						break;
						
					case FILE_TYPE_VPK:
					case FILE_TYPE_ZIP:
						color = ARCHIVE_COLOR;
						icon = archive_icon;
						break;
						
					case FILE_TYPE_MP3:
						color = IMAGE_COLOR;
						icon = audio_icon;
						break;
						
					case FILE_TYPE_SFO:
						// color = SFO_COLOR;
						icon = sfo_icon;
						break;
					
					case FILE_TYPE_INI:
					case FILE_TYPE_TXT:
					case FILE_TYPE_XML:
						// color = TXT_COLOR;
						icon = text_icon;
						break;
						
					default:
						icon = file_icon;
						break;
				}
				
		}

		// Draw icon
 		if (icon)
 			vita2d_draw_texture(icon, SHELL_MARGIN_X_CUSTOM, SCREEN_HEIGHT - FONT_Y_SPACE2 + 3.0f);

		// Draw shortened file name
		pgf_draw_text(SHELL_MARGIN_X_CUSTOM + 26.0f, SCREEN_HEIGHT - FONT_Y_SPACE2, WHITE, FONT_SIZE, file_entry2->name);

		// File information
		if (strcmp(file_entry2->name, DIR_UP) != 0) {
			char *str = NULL;

			if (dir_level == 0) {
				if (file_entry2->size != 0 && file_entry2->size2 != 0) {
					char free_size_string[16], max_size_string[16];
					getSizeString(free_size_string, file_entry2->size2 - file_entry2->size);
					getSizeString(max_size_string, file_entry2->size2);

					char string[32];
					snprintf(string, sizeof(string), "%s / %s", free_size_string, max_size_string);

					str = string;
				} else {
					str = "-";
				}
			} else {
				if (!file_entry2->is_folder) {
					// Folder/size
					char string[16];
					getSizeString(string, file_entry2->size);

					str = string;
				} else {
					str = language_container[FOLDER];
				}
			}

			pgf_draw_text(ALIGN_RIGHT(INFORMATION_X, vita2d_pgf_text_width(font, FONT_SIZE, str)), SCREEN_HEIGHT - FONT_Y_SPACE2, color, FONT_SIZE, str);

			// Date
			char date_string[16];
			getDateString(date_string, date_format, &file_entry2->time);

			char time_string[24];
			getTimeString(time_string, time_format, &file_entry2->time);

			char string[64];
			sprintf(string, "%s %s", date_string, time_string);

			pgf_draw_text(ALIGN_RIGHT(SCREEN_WIDTH - SHELL_MARGIN_X_CUSTOM, vita2d_pgf_text_width(font, FONT_SIZE, string)), SCREEN_HEIGHT - FONT_Y_SPACE2, color, FONT_SIZE, string);
		}
				
		if ( h > 0)			
			slide_value_limit = h * (height_item + length_border ) + length_border;			
		else 
			slide_value_limit = h * (height_item + length_border );
		
		//Animation for multi purpose 
		slide_to_location(0.1f);

		
		if (notification_start) {
			//dialog_step = DIALOG_STEP_NONE;			
			initnotification();
			drawShellInfo2(file_list.path);
		}
		

		
}

void shellUI2() {
// Position
	memset(base_pos_list, 0, sizeof(base_pos_list));
	memset(rel_pos_list, 0, sizeof(rel_pos_list));

	// Paths
	memset(cur_file, 0, sizeof(cur_file));
	memset(archive_path, 0, sizeof(archive_path));

	// File lists
	memset(&file_list, 0, sizeof(FileList));
	memset(&mark_list, 0, sizeof(FileList));
	memset(&copy_list, 0, sizeof(FileList));
	memset(&install_list, 0, sizeof(FileList));

	// Current path is 'home'
	strcpy(file_list.path, HOME_PATH);

	// Last dir
	char lastdir[MAX_PATH_LENGTH];
	ReadFile(VITASHELL_LASTDIR, lastdir, sizeof(lastdir));

	// Calculate dir positions if the dir is valid
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (sceIoGetstat(lastdir, &stat) >= 0) {
		int i;
		for (i = 0; i < strlen(lastdir) + 1; i++) {
			if (lastdir[i] == ':' || lastdir[i] == '/') {
				char ch = lastdir[i + 1];
				lastdir[i + 1] = '\0';

				char ch2 = lastdir[i];
				lastdir[i] = '\0';

				char *p = strrchr(lastdir, '/');
				if (!p)
					p = strrchr(lastdir, ':');
				if (!p)
					p = lastdir - 1;

				lastdir[i] = ch2;

				refreshFileList();
				focusOnFilename(p + 1);

				strcpy(file_list.path, lastdir);

				lastdir[i + 1] = ch;

				dirLevelUp();
			}
		}
	}

	// Refresh file list
	refreshFileList();

	// Init context menu param
	ContextMenu context_menu;
	context_menu.menu_entries = menu_entries;
	context_menu.n_menu_entries = N_MENU_ENTRIES;
	context_menu.menu_more_entries = menu_more_entries;
	context_menu.n_menu_more_entries = N_MENU_MORE_ENTRIES;
	context_menu.menu_max_width = ctx_menu_max_width;
	context_menu.menu_more_max_width = ctx_menu_more_max_width;
	context_menu.more_pos = MENU_ENTRY_MORE;
	context_menu.menuEnterCallback = contextMenuEnterCallback;
	context_menu.menuMoreEnterCallback = contextMenuMoreEnterCallback;

	refreshUI2();	

	while (1) {
		if (!Change_UI) break;

		readPad();

		int refresh = 0;

		// Control
		if (dialog_step == DIALOG_STEP_NONE) {
			if (getContextMenuMode() != CONTEXT_MENU_CLOSED) {
				contextMenuCtrl(&context_menu);
			} else if (getListDialogMode() != LIST_DIALOG_CLOSE) {
				listDialogCtrl();
			} else {
				fileBrowserMenuCtrl2();
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

		mainUI2();			

		// Draw context menu
		drawContextMenu(&context_menu);

		// Draw list dialog
		drawListDialog();

		// End drawing
		endDrawing();		
	
	}

	// Empty lists
	fileListEmpty(&copy_list);
	fileListEmpty(&mark_list);
	fileListEmpty(&file_list);
	
}

static void initContextMenuWidth() {
	int i;
	for (i = 0; i < N_MENU_ENTRIES; i++) {
		if (menu_entries[i].visibility != CTX_VISIBILITY_UNUSED)
			ctx_menu_max_width = MAX(ctx_menu_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_entries[i].name]));

		if (menu_entries[i].name == MARK_ALL) {
			menu_entries[i].name = UNMARK_ALL;
			i--;
		}
	}

	ctx_menu_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	ctx_menu_max_width = MAX(ctx_menu_max_width, CONTEXT_MENU_MIN_WIDTH);

	for (i = 0; i < N_MENU_MORE_ENTRIES; i++) {
		if (menu_more_entries[i].visibility != CTX_VISIBILITY_UNUSED)
			ctx_menu_more_max_width = MAX(ctx_menu_more_max_width, vita2d_pgf_text_width(font, FONT_SIZE, language_container[menu_more_entries[i].name]));
	}

	ctx_menu_more_max_width += 2.0f * CONTEXT_MENU_MARGIN;
	ctx_menu_more_max_width = MAX(ctx_menu_more_max_width, CONTEXT_MENU_MORE_MIN_WIDTH);
}

int UI2() {
	// Init context menu width
	initContextMenuWidth();
	initTextContextMenuWidth();	

	// Main
	shellUI2();


	
	return 0;
}
