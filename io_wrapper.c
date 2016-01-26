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

#ifdef USE_HOST0

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

#endif

int fileIoOpen(const char *file, int flags, SceMode mode) {
#ifdef USE_HOST0
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoOpen(path, flags, mode);
	}
#endif
	return sceIoOpen(file, flags, mode);
}

int fileIoClose(SceUID fd) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoClose(fd);
	}
#endif
	return sceIoClose(fd);
}

int fileIoRead(SceUID fd, void *data, SceSize size) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoRead(fd, data, size);
	}
#endif
	return sceIoRead(fd, data, size);
}

int fileIoWrite(SceUID fd, const void *data, SceSize size) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoWrite(fd, data, size);
	}
#endif
	return sceIoWrite(fd, data, size);
}

SceOff fileIoLseek(SceUID fd, SceOff offset, int whence) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return (SceOff)psp2LinkIoLseek(fd, offset, whence);
	}
#endif
	return sceIoLseek(fd, offset, whence);
}

int fileIoRemove(const char *file) {
#ifdef USE_HOST0
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoRemove(path);
	}
#endif
	return sceIoRemove(file);
}

int fileIoMkdir(const char *dirname, SceMode mode) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoMkdir(path, mode);
	}
#endif
	return sceIoMkdir(dirname, mode);
}

int fileIoRmdir(const char *dirname) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoRmdir(path);
	}
#endif
	return sceIoRmdir(dirname);
}

int fileIoRename(const char *oldname, const char *newname) {
#ifdef USE_HOST0
	if (strncmp(oldname, HOST0, sizeof(HOST0) - 1) == 0) {
		// return psp2LinkRename(oldname, newname);
		return 0;
	}
#endif
	return sceIoRename(oldname, newname);
}

int fileIoDopen(const char *dirname) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoDopen(path);
	}
#endif
	return sceIoDopen(dirname);
}

int fileIoDread(SceUID fd, SceIoDirent *dir) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDread(fd, dir);
	}
#endif
	return sceIoDread(fd, dir);
}

int fileIoDclose(SceUID fd) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDclose(fd);
	}
#endif
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