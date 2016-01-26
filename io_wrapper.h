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

int fileIoOpen(const char *file, int flags, SceMode mode);
int fileIoClose(SceUID fd);
int fileIoRead(SceUID fd, void *data, SceSize size);
int fileIoWrite(SceUID fd, const void *data, SceSize size);
int fileIoLseek(SceUID fd, int offset, int whence);
int fileIoRemove(const char *file);
int fileIoMkdir(const char *dirname, SceMode mode);
int fileIoRmdir(const char *dirname);
int fileIoRename(const char *oldname, const char *newname);
int fileIoDopen(const char *dirname);
int fileIoDread(SceUID fd, SceIoDirent *dir);
int fileIoDclose(SceUID fd);
int fileIoGetstat(const char *name, SceIoStat *stat);
int fileIoChstat(const char *name, SceIoStat *stat, unsigned int bits);

#endif