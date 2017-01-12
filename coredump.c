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

#include "main.h"
#include "file.h"
#include "utils.h"
#include "theme.h"
#include "coredump.h"
#include "elf.h"

typedef struct {
	int unk_0; // 0x00
	int size; // 0x04
	int unk_8; // 0x08
	char name[16]; // 0x0C
	int unk_1C; // 0x1C
	int num; // 0x20
} InfoHeader; // 0x24

typedef struct {
	int unk; // 0x00
	int perm; // 0x04
	uint32_t addr; // 0x08
	uint32_t size; // 0x0C
	int unk_10; // 0x10
} ModuleSegment; // 0x14

typedef struct {
	int unk_0; // 0x00
	int uid; // 0x04
	int sdk_version; // 0x08
	int version; // 0xC
	int unk_10; // 0x10
	int addr_14; // 0x14
	int unk_18; // 0x18
	int addr_1C; // 0x1C
	int unk_20; // 0x20
	char name[28]; // 0x24
	int unk_44; // 0x40
	int unk_48; // 0x44 2
	uint32_t nid; // 0x48
	int num_segments; // 0x4C
	ModuleSegment segments[1]; // 0x50
	uint32_t addr[4]; // 0x78
} ModuleInfoOneSegment; // 0x88

typedef struct {
	int unk_0; // 0x00
	int uid; // 0x04
	int sdk_version; // 0x08
	int version; // 0xC
	int unk_10; // 0x10
	int addr_14; // 0x14
	int unk_18; // 0x18
	int addr_1C; // 0x1C
	int unk_20; // 0x20
	char name[28]; // 0x24
	int unk_44; // 0x40
	int unk_48; // 0x44 2
	uint32_t nid; // 0x48
	int num_segments; // 0x4C
	ModuleSegment segments[2]; // 0x50
	uint32_t addr[4]; // 0x78
} ModuleInfoTwoSegment; // 0x88

typedef struct {
	int unk; // 0x00
	int uid; // 0x04
	char name[32]; // 0x08
	int type; // 0x28
	uint32_t vaddr; // 0x2C
	uint32_t vsize; // 0x30
	int unk_34;
	int unk_38;
	int unk_3C;
	int unk_40;
	int unk_44;
} MemBlkInfo; // 0x48

typedef struct {
	int size; // 0x00
	int uid; // 0x04
	char name[32]; // 0x08
	int unk_28; // 0x28
	int unk_2C[18]; // 0x2C
	int cause; // 0x74
	int unk_78[9]; // 0x78
	uint32_t pc; // 0x9C
	int unk_A0[10]; // 0xA0
} ThreadInfo; // 0xC8

typedef struct {
	int size; // 0x00
	int uid; // 0x04
	uint32_t regs[16]; // 0x08
	int unk[75]; // 0x48
	uint32_t bad_v_addr; // 0x174
} ThreadRegInfo; // 0x178

// ARM registers
static char *regnames[] = { "a1", "a2", "a3", "a4",
							"v1", "v2", "v3", "v4",
							"v5", "v6", "v7", "v8",
							"ip", "sp", "lr", "pc" };

char *getCauseName(int cause) {
	switch (cause) {
		case 0x30002:
			return "Undefined instruction exception";
			
		case 0x30003:
			return "Prefetch abort exception";
		
		case 0x30004:
			return "Data abort exception";
			
		case 0x60080:
			return "Division by zero";
	}
	
	return "Unknown cause";
}

int decompressGzip(uint8_t *dst, int size_dst, uint8_t *src, int size_src) {
	z_stream z;
	memset(&z, 0, sizeof(z_stream));

	z.avail_in = size_src;
	z.next_in = src;
	z.avail_out = size_dst;
	z.next_out = dst;

	if (inflateInit2(&z, 15 + 32) != Z_OK)
		return -1;

	if (inflate(&z, Z_FINISH) != Z_STREAM_END) {
		inflateEnd(&z);
		return -2;
	}

	inflateEnd(&z);
	return z.total_out;
}

int coredumpViewer(char *file) {
	void *buffer = malloc(BIG_BUFFER_SIZE);
	if (!buffer)
		return -1;

	int size = ReadFile(file, buffer, BIG_BUFFER_SIZE);

	if (*(uint16_t *)buffer == 0x8B1F) {
		void *out_buf = malloc(BIG_BUFFER_SIZE);
		if (!out_buf) {
			free(buffer);
			return -2;
		}

		int out_size = decompressGzip(out_buf, BIG_BUFFER_SIZE, buffer, size);
		if (out_size < 0) {
			free(out_buf);
			free(buffer);
			return -3;
		}

		memcpy(buffer, out_buf, out_size);

		free(out_buf);
	}

	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buffer;
	Elf32_Phdr *phdr = (Elf32_Phdr *)((uint32_t)buffer + ehdr->e_phoff);

	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		free(buffer);
		return -4;
	}

	char thname[32], modname[32];
	SceUID thid = -1, modid = -1;
	uint32_t epc = -1, rel_epc = -1;
	int cause = -1;
	uint32_t bad_v_addr = -1;
	uint32_t regs[16];

	int i;
	for (i = 0; i < ehdr->e_phnum; i++) {
		if (phdr[i].p_type == 0x4) {
			void *program = malloc(phdr[i].p_filesz);
			if (!program) {
				free(buffer);
				return -5;
			}

			memcpy(program, buffer + phdr[i].p_offset, phdr[i].p_filesz);

			InfoHeader *info = (InfoHeader *)program;
			
			if (strcmp(info->name, "THREAD_INFO") == 0) {
				int j;
				for (j = 0; j < ((InfoHeader *)program)->num; j++) {
					ThreadInfo *thread_info = (ThreadInfo *)(program + sizeof(InfoHeader));
					if (thread_info[j].cause != 0) {
						strcpy(thname, thread_info[j].name);
						thid = thread_info[j].uid;
						epc = thread_info[j].pc;
						cause = thread_info[j].cause;
						break;
					}
				}
			} else if (strcmp(info->name, "THREAD_REG_INFO") == 0) {
				int j;
				for (j = 0; j < ((InfoHeader *)program)->num; j++) {
					ThreadRegInfo *thread_reg_info = (ThreadRegInfo *)(program + sizeof(InfoHeader));

					if (thread_reg_info[j].uid == thid) {
						memcpy(&regs, thread_reg_info[j].regs, sizeof(regs));
						bad_v_addr = thread_reg_info[j].bad_v_addr;
						break;
					}
				}
			} else if (strcmp(info->name, "MODULE_INFO") == 0) {
				uint32_t offset = 0;

				int j;
				for (j = 0; j < ((InfoHeader *)program)->num; j++) {
					ModuleInfoTwoSegment *mod_info = (ModuleInfoTwoSegment *)(program + sizeof(InfoHeader) + offset);
					
					if (epc >= mod_info->segments[0].addr && epc < (mod_info->segments[0].addr + mod_info->segments[0].size)) {
						strcpy(modname, mod_info->name);
						modid = mod_info->uid;
						rel_epc = epc - mod_info->segments[0].addr;
						break;
					}

					if (mod_info->num_segments == 2) {
						offset += sizeof(ModuleInfoTwoSegment);
					} else {
						offset += sizeof(ModuleInfoOneSegment);
					}
				}
			}

			free(program);
		}
	}

	while (1) {
		readPad();

		if (pressed_buttons & SCE_CTRL_CANCEL) {
			break;	
		}

		// Start drawing
		startDrawing(bg_text_image);

		// Draw shell info
		drawShellInfo(file);
		
		float y = START_Y;
		
		// Exception
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "Exception");
		pgf_draw_text(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, getCauseName(cause));
		y += FONT_Y_SPACE;
		
		// Thread ID
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "Thread ID");
		pgf_draw_textf(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, "0x%08X", thid);
		y += FONT_Y_SPACE;
		
		// Thread name
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "Thread name");
		pgf_draw_text(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, thname);
		y += FONT_Y_SPACE;
		
		// EPC
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "EPC");
		if (rel_epc != -1)
			pgf_draw_textf(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, "%s + 0x%08X", modname, rel_epc);
		else
			pgf_draw_textf(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, "0x%08X", epc);
		y += FONT_Y_SPACE;
		
		// Cause
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "Cause");
		pgf_draw_textf(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, "0x%08X", cause);
		y += FONT_Y_SPACE;

		// BadVAddr
		pgf_draw_text(SHELL_MARGIN_X, y, TEXT_COLOR, FONT_SIZE, "BadVAddr");
		pgf_draw_textf(COREDUMP_INFO_X, y, TEXT_COLOR, FONT_SIZE, "0x%08X", bad_v_addr);
		y += FONT_Y_SPACE;

		// Registers
		int j;
		for (j = 0; j < 4; j++) {
			float x = SHELL_MARGIN_X;

			int i;
			for (i = 0; i < 4; i++) {
				pgf_draw_textf(x, y, TEXT_COLOR, FONT_SIZE, "%s:", regnames[j * 4 + i]);
				x += COREDUMP_REGISTER_NAME_SPACE;

				pgf_draw_textf(x, y, TEXT_COLOR, FONT_SIZE, "0x%08X", regs[j * 4 + i]);
				x += COREDUMP_REGISTER_SPACE;
			}
	
			y += FONT_Y_SPACE;
		}

		// End drawing
		endDrawing();
	}

	free(buffer);
	return 0;
}