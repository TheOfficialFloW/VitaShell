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
#include "psp2link/psp2link.h"

int verifyFd(SceUID fd) {
	SceIoStat stat;
	return sceIoGetstatByFd(fd, &stat);
}

void tempFixForPsp2Link(char *path, const char *file) {
	strcpy(path, file);

	char *p = strstr(path, "c:/");
	if (!p || p[3] != '\0') {
		removeEndSlash(path);
	}
}

int fileIoOpen(const char *file, int flags, SceMode mode) {
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoOpen(path, flags, mode);
	}

	return sceIoOpen(file, flags, mode);
}

int fileIoClose(SceUID fd) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoClose(fd);
	}

	return sceIoClose(fd);
}

int fileIoRead(SceUID fd, void *data, SceSize size) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoRead(fd, data, size);
	}

	return sceIoRead(fd, data, size);
}

int fileIoWrite(SceUID fd, const void *data, SceSize size) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoWrite(fd, data, size);
	}

	return sceIoWrite(fd, data, size);
}

int fileIoLseek(SceUID fd, int offset, int whence) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoLseek(fd, offset, whence);
	}

	return sceIoLseek(fd, offset, whence);
}

int fileIoRemove(const char *file) {
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoRemove(path);
	}

	return sceIoRemove(file);
}

int fileIoMkdir(const char *dirname, SceMode mode) {
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoMkdir(path, mode);
	}

	return sceIoMkdir(dirname, mode);
}

int fileIoRmdir(const char *dirname) {
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoRmdir(path);
	}

	return sceIoRmdir(dirname);
}

int fileIoRename(const char *oldname, const char *newname) {
	if (strncmp(oldname, HOST0, sizeof(HOST0) - 1) == 0) {
		// return psp2LinkRename(oldname, newname);
		return 0;
	}

	return sceIoRename(oldname, newname);
}

int fileIoDopen(const char *dirname) {
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoDopen(path);
	}

	return sceIoDopen(dirname);
}

int fileIoDread(SceUID fd, SceIoDirent *dir) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDread(fd, dir);
	}

	return sceIoDread(fd, dir);
}

int fileIoDclose(SceUID fd) {
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDclose(fd);
	}

	return sceIoDclose(fd);
}

int fileIoGetstat(const char *name, SceIoStat *stat) {
	if (strncmp(name, HOST0, sizeof(HOST0) - 1) == 0) {
		//return psp2LinkIoGetstat(name, stat);
		return 0;
	}

	return sceIoGetstat(name, stat);
}

int fileIoChstat(const char *name, SceIoStat *stat, unsigned int bits) {
	if (strncmp(name, HOST0, sizeof(HOST0) - 1) == 0) {
		//return psp2LinkIoGhstat(name, stat, bits);
		return 0;
	}

	return sceIoChstat(name, stat, bits);
}