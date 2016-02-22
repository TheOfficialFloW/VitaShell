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

#include "../main.h"
#include "pboot.h"

#include "../elf_types.h"
#include "libkirk/kirk_engine.h"

#include "headers.h"
#include "binary.h"
#include "icon0.h"

#define HIJACKED_ADDRESS 0x174

#define BIG_BUFFER_SIZE 16 * 1024 * 1024

static uint8_t *big_buffer = NULL;

static char disc_id[128], psp_system_ver[128], title[128];
static int parental_level = 0, region = 0;
static int val_0 = 0, val_1 = 1;

static int argp_fix = 0;

typedef struct {
	char *name;
	int type;
	void *value;
	int exist;
} ParamEntry;

ParamEntry param_entries[] = {
	{ "APP_VER",		PSF_TYPE_STR, "99.99",			1 },
	{ "ATTRIBUTE",		PSF_TYPE_VAL, &val_1,			1 },
	{ "BOOTABLE",		PSF_TYPE_VAL, &val_1,			1 },
	{ "CATEGORY",		PSF_TYPE_STR, "PG",				1 },
	{ "DISC_ID",		PSF_TYPE_STR, disc_id,			1 },
	{ "DISC_VER",		PSF_TYPE_STR, "1.00",			1 },
	{ "PARENTAL_LEVEL", PSF_TYPE_VAL, &parental_level,	0 },
	{ "PBOOT_TITLE",	PSF_TYPE_STR, title,			0 },
	{ "PSP_SYSTEM_VER", PSF_TYPE_STR, psp_system_ver,	0 },
	{ "REGION",			PSF_TYPE_VAL, &region,			0 },
	{ "TITLE",			PSF_TYPE_STR, title,			0 },
	{ "USE_USB",		PSF_TYPE_VAL, &val_0,			1 },
};

#define N_PARAM_ENTRIES (sizeof(param_entries) / sizeof(ParamEntry))

typedef struct {
	uint8_t aes[16];
	uint8_t cmac[16];
} HeaderKeys;

int getKirkSize(uint8_t *key_hdr) {
	int size = *(uint32_t *)(key_hdr + 0x70);

	if (size % 0x10) {
		size += 0x10 - (size % 0x10); // 16 bytes aligned
	}

	size += KIRK_HEADER_SIZE;

	return size;
}

HeaderList *getHeaderList(int size) {
	int i;
	for (i = 0; i < sizeof(header_list) / sizeof(HeaderList); i++) {
		if ((getKirkSize(header_list[i].kirkHeader) - PSP_HEADER_SIZE) >= size) {
			return &header_list[i];
		}
	}

	return NULL;
}

int checkPRX(void *buffer) {
	Elf32_Ehdr *header = (Elf32_Ehdr *)buffer;

	// Is PRX
	if (header->e_magic == ELF_MAGIC && header->e_type == ELF_PRX_TYPE)
		return 1;

	return 0;
}

int fixRelocations(void *buffer) {
	int new_reloc = 0;

	Elf32_Ehdr *header = (Elf32_Ehdr *)buffer;
	Elf32_Shdr *section = (Elf32_Shdr *)((uint32_t)header + header->e_shoff);

	int i;
	for (i = 0; i < header->e_shnum; i++) {
		if (section[i].sh_type == SHT_PRXRELOC) {
			Elf32_Rel *reloc = (Elf32_Rel *)((uint32_t)header + section[i].sh_offset);

			int j;
			for (j = 0; j < section[i].sh_size / sizeof(Elf32_Rel); j++) {
				// Remove all relocations in the hijacked function and add new relocations
				if (argp_fix) {
					if ((uint32_t)reloc[j].r_offset >= HIJACKED_ADDRESS && (uint32_t)reloc[j].r_offset < (HIJACKED_ADDRESS + sizeof(binary))) {
						reloc[j].r_info = 0;

						if (new_reloc < 2) {
							if (new_reloc == 0) {
								reloc[j].r_offset = HIJACKED_ADDRESS + 0x44;
								reloc[j].r_info = R_MIPS_26;
							} else if (new_reloc == 1) {
								reloc[j].r_offset = HIJACKED_ADDRESS + 0x54;
								reloc[j].r_info = R_MIPS_26;
							}

							new_reloc++;
						}
					}
				}

				// Relocation type 7 -> type 0
				if (ELF32_R_TYPE(reloc[j].r_info) == R_MIPS_GPREL16) {
					reloc[j].r_info &= ~R_MIPS_GPREL16;
				}
			}
		}
	}

	return 0;
}

void fixMainArgp(void *buffer) {
	uint32_t text_addr = (uint32_t)buffer + 0x60;

	// Remove atexit
	*(uint32_t *)(text_addr + 0x90) = 0;

	// Redirect function
	*(uint32_t *)(text_addr + 0xD0) = 0x0C000000 | (((uint32_t)(HIJACKED_ADDRESS) >> 2) & 0x03FFFFFF);
	*(uint32_t *)(text_addr + 0x144) = 0x3C050000 | (HIJACKED_ADDRESS >> 16);
	*(uint32_t *)(text_addr + 0x148) = 0x24A50000 | (HIJACKED_ADDRESS & 0xFFFF);

	// Hijack function
	memcpy((void *)text_addr + HIJACKED_ADDRESS, binary, sizeof(binary));
}

void fixPrx(void *buffer) {
	uint32_t text_addr = (uint32_t)buffer + 0x60;

	if (*(uint32_t *)(text_addr + 0xD0) == 0x0C000000) {
		argp_fix = 1;
	} else {
		argp_fix = 0;
	}

	// Fix main argp
	if (argp_fix)
		fixMainArgp(buffer);

	// Fix relocations
	fixRelocations(buffer);
}

int writeDataPsp(SceUID fdin, SceUID fdout, uint32_t offset, int size) {
	// Init kirk
	kirk_init();

	// Find header
	HeaderList *header = getHeaderList(size);
	if (!header)
		return -1;

	uint8_t *kirk_header = header->kirkHeader;
	uint8_t *psp_header	= header->pspHeader;
	int kirk_size = getKirkSize(kirk_header);

	memcpy(big_buffer, kirk_header, KIRK_HEADER_SIZE);

	HeaderKeys keys;
	kirk_decrypt_keys((uint8_t *)&keys, big_buffer);
	memcpy(big_buffer, &keys, sizeof(HeaderKeys));

	// Read DATA.PSP
	sceIoLseek(fdin, offset, SCE_SEEK_SET);
	sceIoRead(fdin, big_buffer + KIRK_HEADER_SIZE, size);

	// Check PRX
	if (!checkPRX(big_buffer + KIRK_HEADER_SIZE))
		return -2;

	// Fix PRX
	fixPrx(big_buffer + KIRK_HEADER_SIZE);

	// Encrypt
	if (kirk_CMD0(big_buffer, big_buffer, sizeof(big_buffer)) != 0)
		return -3;

	// Forge cmac block
	memcpy(big_buffer, kirk_header, 0x90);
	if (kirk_forge(big_buffer, sizeof(big_buffer)) != 0)
		return -4;

	// Write DATA.PSP
	sceIoWrite(fdout, psp_header, PSP_HEADER_SIZE);
	sceIoWrite(fdout, big_buffer + KIRK_HEADER_SIZE, kirk_size - KIRK_HEADER_SIZE);

	return kirk_size + (PSP_HEADER_SIZE - KIRK_HEADER_SIZE);
}

void setDiscId(char *id) {
	memset(disc_id, 0, sizeof(disc_id));
	strncpy(disc_id, id, sizeof(disc_id));
}

void adjustParamSfo(void *buf) {
	SFOHeader *h = (SFOHeader *)buf;
	SFOEntry *e = (SFOEntry *)((uint32_t)h + sizeof(SFOHeader));

	int i;
	for (i = 0; i < h->count; i++) {
		int j;
		for (j = 0; j < N_PARAM_ENTRIES; j++) {
			if (!param_entries[j].exist) {
				if (strstr((char *)((uint32_t)h + h->keyofs + e[i].nameofs), param_entries[j].name)) {
					memcpy(param_entries[j].value, (void *)((uint32_t)h + h->valofs + e[i].dataofs), e[i].valsize);
				}
			}
		}
	}
}

int writeParamSfo(SceUID fdin, SceUID fdout, uint32_t offset, int size) {
	char head[0x2000], keys[0x2000], data[0x2000];

	memset(head, 0, sizeof(head));
	memset(keys, 0, sizeof(keys));
	memset(data, 0, sizeof(data));

	char *k = keys;
	char *d = data;

	// Read PARAM.SFO
	sceIoLseek(fdin, offset, SCE_SEEK_SET);
	sceIoRead(fdin, big_buffer, size);

	// Adjust PARAM.SFO
	adjustParamSfo(big_buffer);

	// Header info
	SFOHeader *h = (SFOHeader *)head;
	SFOEntry *e = (SFOEntry *)((uint32_t)h + sizeof(SFOHeader));

	h->magic = PSF_MAGIC;
	h->version = PSF_VERSION;

	uint32_t count = 0;

	int i;
	for (i = 0; i < N_PARAM_ENTRIES; i++) {
		// Entry info
		h->count = ++count;
		e->nameofs = k - keys;
		e->dataofs = d - data;
		e->alignment = 4;
		e->type = param_entries[i].type;

		// Add name
		strcpy(k, param_entries[i].name);
		k += strlen(k) + 1;

		// Add value
		if (e->type == PSF_TYPE_VAL) {
			e->valsize = sizeof(int);
			e->totalsize = sizeof(int);
			*(uint32_t *)d = *(uint32_t *)param_entries[i].value;
			d += sizeof(int);
		} else {
			e->valsize = strlen(param_entries[i].value) + 1;
			e->totalsize = (e->valsize + 3) & ~3;

			memset(d, 0, e->totalsize);
			memcpy(d, param_entries[i].value, e->valsize);
			d += e->totalsize;
		}

		e++;
	}

	// Adjust keysofs and valofs
	uint32_t keyofs = (char *)e - head;
	h->keyofs = keyofs;

	uint32_t align = 3 - ((uint32_t)(k - keys) & 3);
	while (align < 3) {
		k++;
		align--;
	}

	h->valofs = keyofs + (k - keys);

	// Write PARAM.SFO
	sceIoWrite(fdout, head, (char *)e - head);
	sceIoWrite(fdout, keys, k - keys);
	sceIoWrite(fdout, data, d - data);

	return ((char *)e - head) + (k - keys) + (d - data);
}

int writeIcon0(SceUID fdin, SceUID fdout, uint32_t offset, int size) {
	sceIoLseek(fdin, offset, SCE_SEEK_SET);
	sceIoRead(fdin, big_buffer, size);

	// Check
	if (size == 0 || ((uint32_t *)big_buffer)[0] != 0x474E5089 || ((uint32_t *)big_buffer)[1] != 0x0A1A0A0D || ((uint32_t *)big_buffer)[3] != 0x52444849 ||
		((uint32_t *)big_buffer)[4] > 0x90000000 || ((uint32_t *)big_buffer)[5] > 0x90000000) {
		memcpy(big_buffer, icon0, sizeof(icon0));
		size = sizeof(icon0);
	}

	sceIoWrite(fdout, big_buffer, size);

	return size;
}

int copyByOffset(SceUID fdin, SceUID fdout, uint32_t offset, int size) {
	if (size) {
		sceIoLseek(fdin, offset, SCE_SEEK_SET);
		sceIoRead(fdin, big_buffer, size);
		sceIoWrite(fdout, big_buffer, size);
	}

	return size;
}

int writePboot(char *filein, char *fileout) {
	int res = 0;

	big_buffer = malloc(BIG_BUFFER_SIZE);
	if (!big_buffer)
		return -1;

	memset(big_buffer, 0, BIG_BUFFER_SIZE);

	// Reset buffers
	memset(psp_system_ver, 0, sizeof(psp_system_ver));
	memset(title, 0, sizeof(title));

	// Open input file
	SceUID fdin = sceIoOpen(filein, SCE_O_RDONLY, 0);
	if (fdin < 0)
		goto ERROR_FREE_BUFFER_EXIT;

	// Open output file
	SceUID fdout = sceIoOpen(fileout, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fdout < 0)
		goto ERROR_FREE_BUFFER_EXIT;

	// Get size of input file
	uint32_t file_size = sceIoLseek(fdin, 0, SCE_SEEK_END);
	sceIoLseek(fdin, 0, SCE_SEEK_SET);

	uint32_t offset = 0;

	// Read header
	PBPHeader h_in;
	sceIoRead(fdin, &h_in, sizeof(PBPHeader));

	// Write header
	PBPHeader h_out;
	h_out.magic = PBP_MAGIC;
	h_out.version = PBP_VERSION;

	sceIoWrite(fdout, &h_out, sizeof(PBPHeader));
	offset += sizeof(PBPHeader);

	// Write PARAM.SFO
	h_out.param_offset = offset;
	res = writeParamSfo(fdin, fdout, h_in.param_offset, h_in.icon0_offset - h_in.param_offset);
	if (res < 0)
		goto ERROR_CLOSE_DESCRIPTORS_EXIT;

	offset += res;

	// Write ICON0.PNG, ICON1.PMF, PIC0.PNG, PIC1.PNG, SND0.AT3
	h_out.icon0_offset = offset;
	offset += writeIcon0(fdin, fdout, h_in.icon0_offset, h_in.icon1_offset - h_in.icon0_offset);

	h_out.icon1_offset = offset;
	offset += copyByOffset(fdin, fdout, h_in.icon1_offset, h_in.pic0_offset - h_in.icon1_offset);

	h_out.pic0_offset = offset;
	offset += copyByOffset(fdin, fdout, h_in.pic0_offset, h_in.pic1_offset - h_in.pic0_offset);

	h_out.pic1_offset = offset;
	offset += copyByOffset(fdin, fdout, h_in.pic1_offset, h_in.snd0_offset - h_in.pic1_offset);

	h_out.snd0_offset = offset;
	offset += copyByOffset(fdin, fdout, h_in.snd0_offset, h_in.elf_offset - h_in.snd0_offset);

	// Write DATA.PSP
	h_out.elf_offset = offset;
	res = writeDataPsp(fdin, fdout, h_in.elf_offset, h_in.psar_offset - h_in.elf_offset);
	if (res < 0)
		goto ERROR_CLOSE_DESCRIPTORS_EXIT;

	offset += res;

	// Write DATA.PSAR
	h_out.psar_offset = offset;
	offset += copyByOffset(fdin, fdout, h_in.psar_offset, file_size - h_in.psar_offset);

	// Update header
	sceIoLseek(fdout, 0, SCE_SEEK_SET);
	sceIoWrite(fdout, &h_out, sizeof(PBPHeader));

ERROR_CLOSE_DESCRIPTORS_EXIT:
	sceIoClose(fdout);
	sceIoClose(fdin);

ERROR_FREE_BUFFER_EXIT:
	free(big_buffer);

	if (res < 0)
		sceIoRemove(fileout);

	return res;
}