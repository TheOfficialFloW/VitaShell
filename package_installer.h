/*
	VitaShell
	Copyright (C) 2015-2017, TheFloW

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

#ifndef __PACKAGE_INSTALLER_H__
#define __PACKAGE_INSTALLER_H__

#define ntohl __builtin_bswap32

#define PACKAGE_DIR "ux0:data/pkg"
#define HEAD_BIN PACKAGE_DIR "/sce_sys/package/head.bin"

typedef struct {
	char *file;
} InstallArguments;

int promoteApp(char *path);
int deleteApp(char *titleid);

int makeHeadBin();

int installPackage(char *file);
int install_thread(SceSize args_size, InstallArguments *args);

#endif