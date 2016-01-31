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

#ifndef __IO_WRAPPER_H__
#define __IO_WRAPPER_H__

int _sceIoOpen(const char *file, int flags, SceMode mode);
int _sceIoClose(SceUID fd);
int _sceIoRead(SceUID fd, void *data, SceSize size);
int _sceIoWrite(SceUID fd, const void *data, SceSize size);
SceOff _sceIoLseek(SceUID fd, SceOff offset, int whence);
int _sceIoRemove(const char *file);
int _sceIoMkdir(const char *dirname, SceMode mode);
int _sceIoRmdir(const char *dirname);
int _sceIoRename(const char *oldname, const char *newname);
int _sceIoDopen(const char *dirname);
int _sceIoDread(SceUID fd, SceIoDirent *dir);
int _sceIoDclose(SceUID fd);
int _sceIoGetstat(const char *name, SceIoStat *stat);
int _sceIoGetstatByFd(SceUID fd, SceIoStat *stat);
int _sceIoChstat(const char *name, SceIoStat *stat, unsigned int bits);

void PatchIO();
void restoreIOPatches();

#endif