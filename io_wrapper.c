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
#include "io_wrapper.h"
#include "file.h"
#include "utils.h"
#include "module.h"
#include <psp2link.h>

static int io_patched = 0;

/*
	sa0:
	ux0:app/XXXXXXXXX/
	ux0:cache/XXXXXXXXX/
	ux0:music/
	ux0:picture/
	ux0:pspemu/
	vs0:data/external/
	vs0:sys/external/
*/

#ifdef USE_HOST0

int verifyFd(SceUID fd) {
	SceIoStat stat;
	return sceIoGetstatByFd(fd, &stat);
}

void tempFixForPsp2Link(char *path, const char *file) {
	strncpy(path, file, MAX_PATH_LENGTH);
	path[MAX_PATH_LENGTH - 1] = '\0';

	if (strcmp(path, HOST0) != 0) {
		removeEndSlash(path);
	}
}

#endif

int sceIoOpenPatched(const char *file, int flags, SceMode mode) {
#ifdef USE_HOST0
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoOpen(path, flags, mode);
	}
#endif
	return _sceIoOpen(file, flags, mode);
}

int sceIoClosePatched(SceUID fd) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoClose(fd);
	}
#endif
	return _sceIoClose(fd);
}

int sceIoReadPatched(SceUID fd, void *data, SceSize size) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoRead(fd, data, size);
	}
#endif
	return _sceIoRead(fd, data, size);
}

int sceIoWritePatched(SceUID fd, const void *data, SceSize size) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoWrite(fd, data, size);
	}
#endif
	return _sceIoWrite(fd, data, size);
}

SceOff sceIoLseekPatched(SceUID fd, SceOff offset, int whence) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return (SceOff)psp2LinkIoLseek(fd, offset, whence);
	}
#endif
	return _sceIoLseek(fd, offset, whence);
}

int sceIoRemovePatched(const char *file) {
#ifdef USE_HOST0
	if (strncmp(file, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, file);
		return psp2LinkIoRemove(path);
	}
#endif
	return _sceIoRemove(file);
}

int sceIoMkdirPatched(const char *dirname, SceMode mode) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoMkdir(path, mode);
	}
#endif
	return _sceIoMkdir(dirname, mode);
}

int sceIoRmdirPatched(const char *dirname) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoRmdir(path);
	}
#endif
	return _sceIoRmdir(dirname);
}

int sceIoRenamePatched(const char *oldname, const char *newname) {
#ifdef USE_HOST0
	if (strncmp(oldname, HOST0, sizeof(HOST0) - 1) == 0) {
		// return psp2LinkRename(oldname, newname);
		return 0;
	}
#endif
	return _sceIoRename(oldname, newname);
}

int sceIoDopenPatched(const char *dirname) {
#ifdef USE_HOST0
	if (strncmp(dirname, HOST0, sizeof(HOST0) - 1) == 0) {
		char path[MAX_PATH_LENGTH];
		tempFixForPsp2Link(path, dirname);
		return psp2LinkIoDopen(path);
	}
#endif
	return _sceIoDopen(dirname);
}

int sceIoDreadPatched(SceUID fd, SceIoDirent *dir) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDread(fd, dir);
	}
#endif
	return _sceIoDread(fd, dir);
}

int sceIoDclosePatched(SceUID fd) {
#ifdef USE_HOST0
	int res = verifyFd(fd);
	if (res < 0) {
		return psp2LinkIoDclose(fd);
	}
#endif
	return _sceIoDclose(fd);
}

int sceIoGetstatPatched(const char *name, SceIoStat *stat) {
	if (strncmp(name, HOST0, sizeof(HOST0) - 1) == 0) {
		//return psp2LinkIoGetstat(name, stat);
		return 0;
	}

	return _sceIoGetstat(name, stat);
}

int sceIoChstatPatched(const char *name, SceIoStat *stat, unsigned int bits) {
	if (strncmp(name, HOST0, sizeof(HOST0) - 1) == 0) {
		//return psp2LinkIoGhstat(name, stat, bits);
		return 0;
	}

	return _sceIoChstat(name, stat, bits);
}

void PatchIO() {
	copyStub((uint32_t)&_sceIoOpen, (void *)&sceIoOpen);
	copyStub((uint32_t)&_sceIoClose, (void *)&sceIoClose);
	copyStub((uint32_t)&_sceIoRead, (void *)&sceIoRead);
	copyStub((uint32_t)&_sceIoWrite, (void *)&sceIoWrite);
	copyStub((uint32_t)&_sceIoLseek, (void *)&sceIoLseek);
	copyStub((uint32_t)&_sceIoRemove, (void *)&sceIoRemove);
	copyStub((uint32_t)&_sceIoMkdir, (void *)&sceIoMkdir);
	copyStub((uint32_t)&_sceIoRmdir, (void *)&sceIoRmdir);
	copyStub((uint32_t)&_sceIoRename, (void *)&sceIoRename);
	copyStub((uint32_t)&_sceIoDopen, (void *)&sceIoDopen);
	copyStub((uint32_t)&_sceIoDread, (void *)&sceIoDread);
	copyStub((uint32_t)&_sceIoDclose, (void *)&sceIoDclose);
	copyStub((uint32_t)&_sceIoGetstat, (void *)&sceIoGetstat);
	copyStub((uint32_t)&_sceIoChstat, (void *)&sceIoChstat);

	makeFunctionStub((uint32_t)&sceIoOpen, sceIoOpenPatched);
	makeFunctionStub((uint32_t)&sceIoClose, sceIoClosePatched);
	makeFunctionStub((uint32_t)&sceIoRead, sceIoReadPatched);
	makeFunctionStub((uint32_t)&sceIoWrite, sceIoWritePatched);
	makeFunctionStub((uint32_t)&sceIoLseek, sceIoLseekPatched);
	makeFunctionStub((uint32_t)&sceIoRemove, sceIoRemovePatched);
	makeFunctionStub((uint32_t)&sceIoMkdir, sceIoMkdirPatched);
	makeFunctionStub((uint32_t)&sceIoRmdir, sceIoRmdirPatched);
	makeFunctionStub((uint32_t)&sceIoRename, sceIoRenamePatched);
	makeFunctionStub((uint32_t)&sceIoDopen, sceIoDopenPatched);
	makeFunctionStub((uint32_t)&sceIoDread, sceIoDreadPatched);
	makeFunctionStub((uint32_t)&sceIoDclose, sceIoDclosePatched);
	makeFunctionStub((uint32_t)&sceIoGetstat, sceIoGetstatPatched);
	makeFunctionStub((uint32_t)&sceIoChstat, sceIoChstatPatched);

	io_patched = 1;
}

void restoreIOPatches() {
	if (io_patched) {
		copyStub((uint32_t)&sceIoOpen, (void *)&_sceIoOpen);
		copyStub((uint32_t)&sceIoClose, (void *)&_sceIoClose);
		copyStub((uint32_t)&sceIoRead, (void *)&_sceIoRead);
		copyStub((uint32_t)&sceIoWrite, (void *)&_sceIoWrite);
		copyStub((uint32_t)&sceIoLseek, (void *)&_sceIoLseek);
		copyStub((uint32_t)&sceIoRemove, (void *)&_sceIoRemove);
		copyStub((uint32_t)&sceIoMkdir, (void *)&_sceIoMkdir);
		copyStub((uint32_t)&sceIoRmdir, (void *)&_sceIoRmdir);
		copyStub((uint32_t)&sceIoRename, (void *)&_sceIoRename);
		copyStub((uint32_t)&sceIoDopen, (void *)&_sceIoDopen);
		copyStub((uint32_t)&sceIoDread, (void *)&_sceIoDread);
		copyStub((uint32_t)&sceIoDclose, (void *)&_sceIoDclose);
		copyStub((uint32_t)&sceIoGetstat, (void *)&_sceIoGetstat);
		copyStub((uint32_t)&sceIoChstat, (void *)&_sceIoChstat);
	}
}