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

#ifndef __PBOOT_H__
#define __PBOOT_H__

#define PBP_MAGIC 0x50425000
#define PBP_VERSION 0x00010001

#define PSF_MAGIC 0x46535000
#define PSF_VERSION 0x00000101

#define PSF_TYPE_BIN 0
#define PSF_TYPE_STR 2
#define PSF_TYPE_VAL 4

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t param_offset;
	uint32_t icon0_offset;
	uint32_t icon1_offset;
	uint32_t pic0_offset;
	uint32_t pic1_offset;
	uint32_t snd0_offset;
	uint32_t elf_offset;
	uint32_t psar_offset;
} PBPHeader;

typedef struct {
	uint32_t magic;
	uint32_t version;
	uint32_t keyofs;
	uint32_t valofs;
	uint32_t count;
} SFOHeader;

typedef struct {
	uint16_t nameofs;
	uint8_t alignment;
	uint8_t type;
	uint32_t valsize;
	uint32_t totalsize;
	uint32_t dataofs;
} SFOEntry;

void setDiscId(char *id);
int writePboot(char *filein, char *fileout);

#endif