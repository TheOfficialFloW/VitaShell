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
#include "file.h"
#include "message_dialog.h"
#include "language.h"
#include "utils.h"

SceCtrlData pad;
uint32_t old_buttons, current_buttons, pressed_buttons, hold_buttons, hold2_buttons, released_buttons;

static int netdbg_sock = -1;
static void *net_memory = NULL;
static int net_init = -1;

void errorDialog(int error) {
	if (error < 0) {
		initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, language_container[ERROR], error);
		dialog_step = DIALOG_STEP_ERROR;
	}
}

void infoDialog(char *msg, ...) {
	va_list list;
	char string[512];

	va_start(list, msg);
	vsprintf(string, msg, list);
	va_end(list);

	initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, string);
	dialog_step = DIALOG_STEP_INFO;
}

void disableAutoSuspend() {
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_AUTO_SUSPEND);
	sceKernelPowerTick(SCE_KERNEL_POWER_TICK_DISABLE_OLED_OFF);
}

void readPad() {
	static int hold_n = 0, hold2_n = 0;

	memset(&pad, 0, sizeof(SceCtrlData));
	sceCtrlPeekBufferPositive(0, &pad, 1);

	if (pad.ly < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_UP;
	} else if (pad.ly > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_DOWN;
	}

	if (pad.lx < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_LEFT;
	} else if (pad.lx > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_LEFT_ANALOG_RIGHT;
	}

	if (pad.ry < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_UP;
	} else if (pad.ry > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_DOWN;
	}

	if (pad.rx < ANALOG_CENTER - ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_LEFT;
	} else if (pad.rx > ANALOG_CENTER + ANALOG_THRESHOLD) {
		pad.buttons |= SCE_CTRL_RIGHT_ANALOG_RIGHT;
	}

	current_buttons = pad.buttons;
	pressed_buttons = current_buttons & ~old_buttons;
	hold_buttons = pressed_buttons;
	hold2_buttons = pressed_buttons;
	released_buttons = ~current_buttons & old_buttons;

	if (old_buttons == current_buttons) {
		hold_n++;
		if (hold_n >= 10) {
			hold_buttons = current_buttons;
			hold_n = 6;
		}

		hold2_n++;
		if (hold2_n >= 10) {
			hold2_buttons = current_buttons;
			hold2_n = 10;
		}
	} else {
		hold_n = 0;
		hold2_n = 0;
		old_buttons = current_buttons;
	}
}

int holdButtons(SceCtrlData *pad, uint32_t buttons, uint64_t time) {
	if ((pad->buttons & buttons) == buttons) {
		uint64_t time_start = sceKernelGetProcessTimeWide();

		while ((pad->buttons & buttons) == buttons) {
			sceKernelDelayThread(10 * 1000);
			sceCtrlPeekBufferPositive(0, pad, 1);

			if ((sceKernelGetProcessTimeWide() - time_start) >= time) {
				return 1;
			}
		}
	}

	return 0;
}

int removeEndSlash(char *path) {
	int len = strlen(path);

	if (path[len - 1] == '/') {
		path[len - 1] = '\0';
		return 1;
	}

	return 0;
}

int addEndSlash(char *path) {
	int len = strlen(path);
	if (len < MAX_PATH_LENGTH - 2) {
		if (path[len - 1] != '/') {
			strcat(path, "/");
			return 1;
		}
	}

	return 0;
}

void getSizeString(char *string, uint64_t size) {
	double double_size = (double)size;

	int i = 0;
	static char *units[] = { "B", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB" };
	while (double_size >= 1024.0f) {
		double_size /= 1024.0f;
		i++;
	}

	sprintf(string, "%.*f %s", (i == 0) ? 0 : 2, double_size, units[i]);
}

void getDateString(char *string, int date_format, SceRtcTime *time) {
	switch (date_format) {
		case SCE_SYSTEM_PARAM_DATE_FORMAT_YYYYMMDD:
			sprintf(string, "%04d/%02d/%02d", time->year, time->month, time->day);
			break;

		case SCE_SYSTEM_PARAM_DATE_FORMAT_DDMMYYYY:
			sprintf(string, "%02d/%02d/%04d", time->day, time->month, time->year);
			break;

		case SCE_SYSTEM_PARAM_DATE_FORMAT_MMDDYYYY:
			sprintf(string, "%02d/%02d/%04d", time->month, time->day, time->year);
			break;
	}
}

void getTimeString(char *string, int time_format, SceRtcTime *time) {
	switch(time_format) {
		case SCE_SYSTEM_PARAM_TIME_FORMAT_12HR:
			sprintf(string, "%02d:%02d %s", time->hour >= 12 ? time->hour - 12 : time->hour, time->minutes, time->hour >= 12 ? "PM" : "AM");
			break;

		case SCE_SYSTEM_PARAM_TIME_FORMAT_24HR:
			sprintf(string, "%02d:%02d", time->hour, time->minutes);
			break;
	}
}

int debugPrintf(char *text, ...) {
#ifndef RELEASE
	va_list list;
	char string[512];

	va_start(list, text);
	vsprintf(string, text, list);
	va_end(list);

	netdbg(string);

#ifdef ENABLE_FILE_LOGGING
	SceUID fd = sceIoOpen("ux0:vitashell_log.txt", SCE_O_WRONLY | SCE_O_CREAT | SCE_O_APPEND, 0777);
	if (fd >= 0) {
		sceIoWrite(fd, string, strlen(string));
		sceIoClose(fd);
	}
#endif

#endif
	return 0;
}

int netdbg_init() {
	int ret = 0;
#ifdef NETDBG_ENABLE
	SceNetSockaddrIn server;
	SceNetInitParam initparam;
	SceUShort16 port = NETDBG_DEFAULT_PORT;

#ifdef NETDBG_PORT
	port = NETDBG_PORT;
#endif

	/* Init Net */
	ret = sceNetShowNetstat();
	if (ret == SCE_NET_ERROR_ENOTINIT) {
		net_memory = malloc(NET_INIT_SIZE);

		initparam.memory = net_memory;
		initparam.size = NET_INIT_SIZE;
		initparam.flags = 0;

		ret = net_init = sceNetInit(&initparam);
		if (net_init < 0)
			goto error_netinit;
	} else if (ret != 0) {
		goto error_netstat;
	}

	server.sin_len = sizeof(server);
	server.sin_family = SCE_NET_AF_INET;
	sceNetInetPton(SCE_NET_AF_INET, NETDBG_IP, &server.sin_addr);
	server.sin_port = sceNetHtons(port);
	memset(server.sin_zero, 0, sizeof(server.sin_zero));

	ret = netdbg_sock = sceNetSocket("VitaShellNetDbg", SCE_NET_AF_INET, SCE_NET_SOCK_STREAM, 0);
	if (netdbg_sock < 0)
		goto error_netsock;

	ret = sceNetConnect(netdbg_sock, (SceNetSockaddr *)&server, sizeof(server));
	if (ret < 0)
		goto error_netconnect;

	return 0;

error_netconnect:
	sceNetSocketClose(netdbg_sock);
	netdbg_sock = -1;
error_netsock:
	if (net_init == 0) {
		sceNetTerm();
		net_init = -1;
	}
error_netinit:
	if (net_memory) {
		free(net_memory);
		net_memory = NULL;
	}
error_netstat:
#endif
	return ret;
}

void netdbg_fini() {
	if (netdbg_sock > 0) {
		sceNetSocketClose(netdbg_sock);
		if (net_init == 0)
			sceNetTerm();
		if (net_memory)
			free(net_memory);
		netdbg_sock = -1;
		net_init = -1;
		net_memory = NULL;
	}
}

int netdbg(const char *text, ...) {
	va_list list;
	char string[512];
	if (netdbg_sock > 0) {
		va_start(list, text);
		vsprintf(string, text, list);
		va_end(list);
		return sceNetSend(netdbg_sock, string, strlen(string), 0);
	}
	return -1;
}
