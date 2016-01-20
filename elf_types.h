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

#ifndef __ELFTYPES_H__
#define __ELFTYPES_H__

#define ELF_MACHINE_MIPS 0x0008
#define ELF_SH_STRTAB ".shstrtab"

#define ELF_SECT_MAX_NAME 128

/* Define ELF types */
typedef uint32_t Elf32_Addr;
typedef uint32_t Elf32_Off;
typedef uint32_t Elf32_Sword;
typedef uint32_t Elf32_Word;
typedef uint16_t Elf32_Half;

#define ELF_MAGIC	0x464C457F

#define ELF_EXEC_TYPE 0x0002
#define ELF_PRX_TYPE  0xFFA0

#define SHT_NULL 0 
#define SHT_PROGBITS 1 
#define SHT_SYMTAB 2 
#define SHT_STRTAB 3 
#define SHT_RELA 4 
#define SHT_HASH 5 
#define SHT_DYNAMIC 6 
#define SHT_NOTE 7 
#define SHT_NOBITS 8 
#define SHT_REL 9 
#define SHT_SHLIB 10 
#define SHT_DYNSYM 11 
#define SHT_LOPROC 0x70000000 
#define SHT_HIPROC 0x7fffffff 
#define SHT_LOUSER 0x80000000 
#define SHT_HIUSER 0xffffffff

#define SHT_PRXRELOC (SHT_LOPROC | 0xA0)

// MIPS Reloc Entry Types
#define R_MIPS_NONE	 0
#define R_MIPS_16	   1
#define R_MIPS_32	   2
#define R_MIPS_REL32	3
#define R_MIPS_26	   4
#define R_MIPS_HI16	 5
#define R_MIPS_LO16	 6
#define R_MIPS_GPREL16  7
#define R_MIPS_LITERAL  8
#define R_MIPS_GOT16	9
#define R_MIPS_PC16	 10
#define R_MIPS_CALL16   11
#define R_MIPS_GPREL32  12

#define SHF_WRITE 		1
#define SHF_ALLOC 		2
#define SHF_EXECINSTR 	4

#define PT_NULL 		0
#define PT_LOAD 		1
#define PT_DYNAMIC 		2
#define PT_INTERP 		3
#define PT_NOTE 		4
#define PT_SHLIB 		5
#define PT_PHDR 		6
#define PT_LOPROC 		0x70000000
#define PT_HIPROC 		0x7fffffff

/* ELF file header */
typedef struct { 
	Elf32_Word e_magic;
	uint8_t e_class;
	uint8_t e_data;
	uint8_t e_idver;
	uint8_t e_pad[9];
	Elf32_Half e_type; 
	Elf32_Half e_machine; 
	Elf32_Word e_version; 
	Elf32_Addr e_entry; 
	Elf32_Off e_phoff; 
	Elf32_Off e_shoff; 
	Elf32_Word e_flags; 
	Elf32_Half e_ehsize; 
	Elf32_Half e_phentsize; 
	Elf32_Half e_phnum; 
	Elf32_Half e_shentsize; 
	Elf32_Half e_shnum; 
	Elf32_Half e_shstrndx; 
} __attribute__((packed)) Elf32_Ehdr;

/* ELF section header */
typedef struct { 
	Elf32_Word sh_name; 
	Elf32_Word sh_type; 
	Elf32_Word sh_flags; 
	Elf32_Addr sh_addr; 
	Elf32_Off sh_offset; 
	Elf32_Word sh_size; 
	Elf32_Word sh_link; 
	Elf32_Word sh_info; 
	Elf32_Word sh_addralign; 
	Elf32_Word sh_entsize; 
} __attribute__((packed)) Elf32_Shdr;

typedef struct { 
	Elf32_Word p_type; 
	Elf32_Off p_offset; 
	Elf32_Addr p_vaddr; 
	Elf32_Addr p_paddr; 
	Elf32_Word p_filesz; 
	Elf32_Word p_memsz; 
	Elf32_Word p_flags; 
	Elf32_Word p_align; 
} Elf32_Phdr;

#define ELF32_R_SYM(i) ((i)>>8) 
#define ELF32_R_TYPE(i) ((uint8_t)(i&0xFF))

typedef struct { 
	Elf32_Addr r_offset; 
	Elf32_Word r_info; 
} Elf32_Rel;

typedef struct { 
	Elf32_Word st_name; 
	Elf32_Addr st_value; 
	Elf32_Word st_size; 
	unsigned char st_info; 
	unsigned char st_other; 
	Elf32_Half st_shndx; 
} __attribute__((packed)) Elf32_Sym;

#define STB_LOCAL 0
#define STB_GLOBAL 1
#define STB_WEAK 2
#define STB_LOPROC 13
#define STB_HIPROC 15

#define ELF32_ST_BIND(i) ((i)>>4)
#define ELF32_ST_TYPE(i) ((i)&0xf)
#define ELF32_ST_INFO(b,t) (((b)<<4)+((t)&0xf))

#endif