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

int listMemBlocks(uint32_t start, uint32_t end) {
	if (start < 0x60000000 || start > 0xF0000000 || end < 0x60000000 || end > 0xF0000000)
		return -1;

	debugPrintf("List memory blocks...\n");

	int count = 0;

	uint32_t i = start;
	while (i < end) {
		SceKernelMemBlockInfo info;
		memset(&info, 0, sizeof(SceKernelMemBlockInfo));
		info.size = sizeof(SceKernelMemBlockInfo);
		if (sceKernelGetMemBlockInfoByRange((void *)i, 0x1000, &info) >= 0) {
			SceUID blockid = sceKernelFindMemBlockByAddr(info.mappedBase, 0); // fails on module executable blocks
			debugPrintf("0x%08X, 0x%08X, 0x%08X: 0x%08X\n", info.mappedBase, info.mappedSize, info.type, blockid);
			i = (uint32_t)info.mappedBase + info.mappedSize;
			count++;
		} else {
			i += 0x1000;
		}
	}

	debugPrintf("Found %d memory blocks\n", count);

	return 0;
}

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