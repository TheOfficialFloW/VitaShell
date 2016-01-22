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

int findMemBlockByAddr(uint32_t address, SceKernelMemBlockInfo *pInfo) {
	if (address < 0x60000000 || address > 0xF0000000)
		return 0;

	uint32_t base = ALIGN(address, 0x10000);
	uint32_t i = base - 0x10000000;
	while (i < base + 0x10000000) {
		SceKernelMemBlockInfo info;
		memset(&info, 0, sizeof(SceKernelMemBlockInfo));
		info.size = sizeof(SceKernelMemBlockInfo);
		if (sceKernelGetMemBlockInfoByRange((void *)i, 0x1000, &info) >= 0) {
			if (address >= (uint32_t)info.mappedBase && address < (uint32_t)info.mappedBase + info.mappedSize) {
				memcpy(pInfo, &info, sizeof(SceKernelMemBlockInfo));
				return 1;
			}

			i = (uint32_t)info.mappedBase + info.mappedSize;
		} else {
			i += 0x1000;
		}
	}

	return 0;
}