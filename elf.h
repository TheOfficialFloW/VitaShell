/*
 * Copyright (c) 2015 Sergi Granell (xerpi)
 */

/* NOTE: This file has been modified to satisfy the SONY PSVita
   (PlayStation Vita) operation system's design.
 */

/* This file defines standard ELF types, structures, and macros.
   Copyright (C) 1995-2003,2004,2005,2006 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, write to the Free
   Software Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307 USA.  */

#ifndef _ELF_H
#define	_ELF_H 1

/* Standard ELF types.  */

#include <stdint.h>

/* Type for a 16-bit quantity.  */
typedef uint16_t Elf32_Half;
typedef uint16_t Elf64_Half;

/* Types for signed and unsigned 32-bit quantities.  */
typedef uint32_t Elf32_Word;
typedef	int32_t  Elf32_Sword;
typedef uint32_t Elf64_Word;
typedef	int32_t  Elf64_Sword;

/* Types for signed and unsigned 64-bit quantities.  */
typedef uint64_t Elf32_Xword;
typedef	int64_t  Elf32_Sxword;
typedef uint64_t Elf64_Xword;
typedef	int64_t  Elf64_Sxword;

/* Type of addresses.  */
typedef uint32_t Elf32_Addr;
typedef uint64_t Elf64_Addr;

/* Type of file offsets.  */
typedef uint32_t Elf32_Off;
typedef uint64_t Elf64_Off;

/* Type for section indices, which are 16-bit quantities.  */
typedef uint16_t Elf32_Section;
typedef uint16_t Elf64_Section;

/* Type for version symbol information.  */
typedef Elf32_Half Elf32_Versym;
typedef Elf64_Half Elf64_Versym;


/* The ELF file header.  This appears at the start of every ELF file.  */

#define EI_NIDENT (16)

typedef struct {
	unsigned char   e_ident[EI_NIDENT];     /* Magic number and other info */
	Elf32_Half      e_type;                 /* Object file type */
	Elf32_Half      e_machine;              /* Architecture */
	Elf32_Word      e_version;              /* Object file version */
	Elf32_Addr      e_entry;                /* Entry point virtual address */
	Elf32_Off       e_phoff;                /* Program header table file offset */
	Elf32_Off       e_shoff;                /* Section header table file offset */
	Elf32_Word      e_flags;                /* Processor-specific flags */
	Elf32_Half      e_ehsize;               /* ELF header size in bytes */
	Elf32_Half      e_phentsize;            /* Program header table entry size */
	Elf32_Half      e_phnum;                /* Program header table entry count */
	Elf32_Half      e_shentsize;            /* Section header table entry size */
	Elf32_Half      e_shnum;                /* Section header table entry count */
	Elf32_Half      e_shstrndx;             /* Section header string table index */
} Elf32_Ehdr;

/* Fields in the e_ident array.  The EI_* macros are indices into the
   array.  The macros under each EI_* macro are the values the byte
   may have.  */

#define EI_MAG0   0     /* File identification byte 0 index */
#define ELFMAG0   0x7f	/* Magic number byte 0 */

#define EI_MAG1   1     /* File identification byte 1 index */
#define ELFMAG1   'E'   /* Magic number byte 1 */

#define EI_MAG2   2     /* File identification byte 2 index */
#define ELFMAG2   'L'   /* Magic number byte 2 */

#define EI_MAG3   3     /* File identification byte 3 index */
#define ELFMAG3   'F'   /* Magic number byte 3 */

/* Conglomeration of the identification bytes, for easy testing as a word.  */
#define	ELFMAG   "\177ELF"
#define	SELFMAG  4

/** EI_CLASS values */
#define EI_CLASS        4   /* File class byte index */
#define ELFCLASSNONE	0   /* Invalid class */
#define ELFCLASS32      1   /* 32-bit objects */
#define ELFCLASS64      2   /* 64-bit objects */
#define ELFCLASSNUM     3

/** EI_DATA values */
#define EI_DATA         5   /* Data encoding byte index */
#define ELFDATANONE     0   /* Invalid data encoding */
#define ELFDATA2LSB     1   /* 2's complement, little endian */
#define ELFDATA2MSB     2   /* 2's complement, big endian */
#define ELFDATANUM      3
#define EI_VERSION	6   /* File version byte index */
                            /* Value must be EV_CURRENT */
#define	EI_OSABI	7   /* Operating system/ABI identification */
#define	EI_ABIVERSION	8   /* ABI version */
#define EI_PAD		9   /* Byte index of padding bytes */

/* Legal values for e_type (object file type).  */

#define ET_NONE	                0           /* No file type */
#define ET_REL                  1           /* Relocatable file */
#define ET_EXEC                 2           /* Executable file */
#define ET_DYN                  3           /* Shared object file */
#define ET_CORE	                4           /* Core file */
#define	ET_NUM                  5           /* Number of defined types */
#define ET_LOOS                 0xFE00      /* OS-specific range start */
#define ET_HIOS                 0xFEFF      /* OS-specific range end */
#define ET_SCE_EXEC             0xFE00      /* SCE Executable file */
#define ET_SCE_RELEXEC          0xFE04      /* SCE Relocatable file */
#define ET_SCE_STUBLIB          0xFE0C      /* SCE SDK Stubs */
#define ET_SCE_DYNAMIC          0xFE18      /* Unused */
#define ET_SCE_PSPRELEXEC       0xFFA0      /* Unused (PSP ELF only) */
#define ET_SCE_PPURELEXEC       0xFFA4      /* Unused (SPU ELF only) */
#define ET_SCE_UNK              0xFFA5      /* Unknown */
#define ET_LOPROC               0xFF00      /* Processor-specific range start */
#define ET_HIPROC               0xFFFF
/* Processor-specific range end */

/* Legal values for e_machine (architecture).  */

#define EM_NONE         0       /* No machine */
#define	EM_ARM          40      /* Advanced RISC Marchines ARM */
#define EM_NUM          95

/* Legal values for e_version (version).  */

#define EV_NONE		0		/* Invalid ELF version */
#define EV_CURRENT	1		/* Current version */
#define EV_NUM		2

/* Section header.  */

typedef struct {
	Elf32_Word      sh_name;        /* Section name (string tbl index) */
	Elf32_Word      sh_type;        /* Section type */
	Elf32_Word      sh_flags;       /* Section flags */
	Elf32_Addr	sh_addr;        /* Section virtual addr at execution */
	Elf32_Off       sh_offset;      /* Section file offset */
	Elf32_Word      sh_size;        /* Section size in bytes */
	Elf32_Word      sh_link;        /* Link to another section */
	Elf32_Word      sh_info;        /* Additional section information */
	Elf32_Word      sh_addralign;   /* Section alignment */
	Elf32_Word      sh_entsize;     /* Entry size if section holds table */
} Elf32_Shdr;

/* Special section indices.  */

#define SHN_UNDEF        0              /* Undefined section */
#define SHN_LORESERVE	 0xff00         /* Start of reserved indices */
#define SHN_LOPROC       0xff00         /* Start of processor-specific */
#define SHN_BEFORE       0xff00         /* Order section before all others
                                           (Solaris).  */
#define SHN_AFTER        0xff01         /* Order section after all others
                                           (Solaris).  */
#define SHN_HIPROC       0xff1f         /* End of processor-specific */
#define SHN_LOOS         0xff20         /* Start of OS-specific */
#define SHN_HIOS         0xff3f	        /* End of OS-specific */
#define SHN_ABS          0xfff1         /* Associated symbol is absolute */
#define SHN_COMMON       0xfff2         /* Associated symbol is common */
#define SHN_XINDEX       0xffff         /* Index is in extra table.  */
#define SHN_HIRESERVE    0xffff         /* End of reserved indices */

/* Legal values for sh_type (section type).  */

#define SHT_NULL        0               /* Section header table entry unused */
#define SHT_PROGBITS    1               /* Program data */
#define SHT_SYMTAB      2               /* Symbol table */
#define SHT_STRTAB      3               /* String table */
#define SHT_HASH        5               /* Symbol hash table */
#define SHT_DYNAMIC     6               /* Dynamic linking information */
#define SHT_NOTE        7               /* Notes */
#define SHT_NOBITS      8               /* Program space with no data (bss) */
#define SHT_SCE_RELA    0x60000000      /* SCE Relocations */
#define SHT_SCE_NID     0x61000001      /* Unused (PSP ELF only) */
#define SHT_SCE_PSPRELA 0x700000A0      /* Unused (PSP ELF only) */
#define SHT_SCE_ARMRELA 0x700000A4      /* Unused (SPU ELF only) */

/* Legal values for sh_flags (section flags).  */

#define SHF_WRITE       (1 << 0)        /* Writable */
#define SHF_ALLOC       (1 << 1)        /* Occupies memory during execution */
#define SHF_EXECINSTR   (1 << 2)        /* Executable */

/*  SCE relocation entry */

// assuming LSB of bitfield is listed first
union {
	Elf32_Word r_short               : 4;
	struct {
		Elf32_Word r_short       : 4;
		Elf32_Word r_symseg      : 4;
		Elf32_Word r_code        : 8;
		Elf32_Word r_datseg      : 4;
		Elf32_Word r_offset_lo   : 12;
		Elf32_Word r_offset_hi   : 20;
		Elf32_Word r_addend      : 12;
	} r_short_entry;
	struct {
		Elf32_Word r_short       : 4;
		Elf32_Word r_symseg      : 4;
		Elf32_Word r_code        : 8;
		Elf32_Word r_datseg      : 4;
		Elf32_Word r_code2       : 8;
		Elf32_Word r_dist2       : 4;
		Elf32_Word r_addend;
		Elf32_Word r_offset;
	} r_long_entry;
} SCE_Rel;

/* Symbol table entry.  */

typedef struct {
	Elf32_Word      st_name;        /* Symbol name (string tbl index) */
	Elf32_Addr      st_value;       /* Symbol value */
	Elf32_Word      st_size;        /* Symbol size */
	unsigned char   st_info;        /* Symbol type and binding */
	unsigned char   st_other;       /* Symbol visibility */
	Elf32_Section   st_shndx;       /* Section index */
} Elf32_Sym;

/* The syminfo section if available contains additional information about
   every dynamic symbol.  */

typedef struct {
	Elf32_Half si_boundto;          /* Direct bindings, symbol bound to */
	Elf32_Half si_flags;            /* Per symbol flags */
} Elf32_Syminfo;

/* Possible values for si_boundto.  */
#define SYMINFO_BT_SELF         0xffff  /* Symbol bound to self */
#define SYMINFO_BT_PARENT       0xfffe  /* Symbol bound to parent */
#define SYMINFO_BT_LOWRESERVE   0xff00  /* Beginning of reserved entries */

/* Possible bitmasks for si_flags.  */
#define SYMINFO_FLG_DIRECT      0x0001  /* Direct bound symbol */
#define SYMINFO_FLG_PASSTHRU    0x0002  /* Pass-thru symbol for translator */
#define SYMINFO_FLG_COPY        0x0004  /* Symbol is a copy-reloc */
#define SYMINFO_FLG_LAZYLOAD    0x0008  /* Symbol bound to object to be lazy
					   loaded */
/* Syminfo version values.  */
#define SYMINFO_NONE            0
#define SYMINFO_CURRENT         1
#define SYMINFO_NUM             2


/* How to extract and insert information held in the st_info field.  */

#define ELF32_ST_BIND(val)              (((unsigned char) (val)) >> 4)
#define ELF32_ST_TYPE(val)              ((val) & 0xf)
#define ELF32_ST_INFO(bind, type)       (((bind) << 4) + ((type) & 0xf))

/* Both Elf32_Sym and Elf64_Sym use the same one-byte st_info field.  */
#define ELF64_ST_BIND(val)              ELF32_ST_BIND (val)
#define ELF64_ST_TYPE(val)              ELF32_ST_TYPE (val)
#define ELF64_ST_INFO(bind, type)       ELF32_ST_INFO ((bind), (type))

/* Legal values for ST_BIND subfield of st_info (symbol binding).  */

#define STB_LOCAL       0               /* Local symbol */
#define STB_GLOBAL      1               /* Global symbol */
#define STB_WEAK        2               /* Weak symbol */
#define	STB_NUM         3               /* Number of defined types.  */
#define STB_LOOS        10              /* Start of OS-specific */
#define STB_HIOS        12              /* End of OS-specific */
#define STB_LOPROC      13              /* Start of processor-specific */
#define STB_HIPROC      15              /* End of processor-specific */

/* Legal values for ST_TYPE subfield of st_info (symbol type).  */

#define STT_NOTYPE      0               /* Symbol type is unspecified */
#define STT_OBJECT      1               /* Symbol is a data object */
#define STT_FUNC        2               /* Symbol is a code object */
#define STT_SECTION     3               /* Symbol associated with a section */
#define STT_FILE        4               /* Symbol's name is file name */
#define STT_COMMON      5               /* Symbol is a common data object */
#define STT_TLS         6               /* Symbol is thread-local data object*/
#define	STT_NUM         7               /* Number of defined types.  */
#define STT_LOOS        10              /* Start of OS-specific */
#define STT_HIOS        12              /* End of OS-specific */
#define STT_LOPROC      13              /* Start of processor-specific */
#define STT_HIPROC      15              /* End of processor-specific */


/* Symbol table indices are found in the hash buckets and chain table
   of a symbol hash table section.  This special index value indicates
   the end of a chain, meaning no further symbols are found in that bucket.  */

#define STN_UNDEF       0               /* End of a chain.  */


/* How to extract and insert information held in the st_other field.  */

#define ELF32_ST_VISIBILITY(o)  ((o) & 0x03)

/* For ELF64 the definitions are the same.  */
#define ELF64_ST_VISIBILITY(o)  ELF32_ST_VISIBILITY (o)

/* Symbol visibility specification encoded in the st_other field.  */
#define STV_DEFAULT	0		/* Default symbol visibility rules */
#define STV_INTERNAL	1		/* Processor specific hidden class */
#define STV_HIDDEN	2		/* Sym unavailable in other modules */
#define STV_PROTECTED	3		/* Not preemptible, not exported */


/* Relocation table entry without addend (in section of type SHT_REL).  */

typedef struct {
	Elf32_Addr      r_offset;               /* Address */
	Elf32_Word      r_info;                 /* Relocation type and symbol index */
} Elf32_Rel;

/* Relocation table entry with addend (in section of type SHT_RELA).  */

typedef struct {
	Elf32_Addr      r_offset;               /* Address */
	Elf32_Word      r_info;                 /* Relocation type and symbol index */
	Elf32_Sword     r_addend;               /* Addend */
} Elf32_Rela;

/* How to extract and insert information held in the r_info field.  */

#define ELF32_R_SYM(val)                ((val) >> 8)
#define ELF32_R_TYPE(val)               ((val) & 0xff)
#define ELF32_R_INFO(sym, type)         (((sym) << 8) + ((type) & 0xff))

//#define ELF32_R_ADDR_BASE(val)    (((val) >> 16) & 0xFF)
#define ELF32_R_ADDR_BASE(val)    ((val) >> 16)
#define ELF32_R_OFS_BASE(val)     (((val) >> 8) & 0xFF)

/* Program segment header.  */

typedef struct {
	Elf32_Word      p_type;                 /* Segment type */
	Elf32_Off       p_offset;               /* Segment file offset */
	Elf32_Addr      p_vaddr;                /* Segment virtual address */
	Elf32_Addr      p_paddr;                /* Segment physical address */
	Elf32_Word      p_filesz;               /* Segment size in file */
	Elf32_Word      p_memsz;                /* Segment size in memory */
	Elf32_Word      p_flags;                /* Segment flags */
	Elf32_Word      p_align;                /* Segment alignment */
} Elf32_Phdr;

/* Legal values for p_type (segment type).  */

#define	PT_NULL         0               /* Program header table entry unused */
#define PT_LOAD         1               /* Loadable program segment */
#define PT_DYNAMIC      2               /* Dynamic linking information */
#define PT_SCE_RELA     0x60000000      /* SCE Relocations */
#define PT_SCE_COMMENT  0x6FFFFF00      /* Unused */
#define PT_SCE_VERSION	0x6FFFFF01      /* Unused */
#define PT_SCE_UNK      0x70000001      /* Unknown */
#define PT_SCE_PSPRELA  0x700000A0      /* Unused (PSP ELF only) */
#define PT_SCE_PPURELA  0x700000A4      /* Unused (SPU ELF only) */

/* Legal values for p_flags (segment flags).  */

#define PF_X            (1 << 0)        /* Segment is executable */
#define PF_W            (1 << 1)        /* Segment is writable */
#define PF_R            (1 << 2)        /* Segment is readable */

/* Legal values for the note segment descriptor types for object files.  */

#define NT_VERSION      1               /* Contains a version string.  */

/* ARM relocs.  */

#define R_ARM_NONE              0
#define R_ARM_PC24              1
#define R_ARM_ABS32             2
#define R_ARM_REL32             3
#define R_ARM_THM_CALL          10
#define R_ARM_CALL              28
#define R_ARM_JUMP24            29
#define R_ARM_THM_JUMP24        30
#define R_ARM_TARGET1           38      /* Same as R_ARM_ABS32 */
#define R_ARM_V4BX              40      /* Same as R_ARM_NONE */
#define R_ARM TARGET2           41      /* Same as R_ARM_REL32 */
#define R_ARM_PREL31            42
#define R_ARM_MOVW_ABS_NC       43
#define R_ARM_MOVT_ABS          44
#define R_ARM_THM_MOVW_ABS_NC   47
#define R_ARM_THM_MOVT_ABS      48

typedef struct {
	uint64_t offset;
	uint64_t length;
	uint64_t compression; // 1 = uncompressed, 2 = compressed
	uint64_t encryption; // 1 = encrypted, 2 = plain
} segment_info;

/* Functions */

#include <stdio.h>

int elf_read_ehdr(FILE *fp, Elf32_Ehdr *ehdr);

// Precondition: ehdr is valid!
int elf_load_phdr(FILE *fp, const Elf32_Ehdr *ehdr, Elf32_Phdr **phdr);
int elf_load_shdr(FILE *fp, const Elf32_Ehdr *ehdr, Elf32_Shdr **shdr);
void elf_free_phdr(Elf32_Phdr **phdr);
void elf_free_shdr(Elf32_Shdr **shdr);

char *elf_load_shstrtab(FILE *fp, const Elf32_Ehdr *ehdr, const Elf32_Shdr *shdr);

void elf_print_ehdr(const Elf32_Ehdr *ehdr);
void elf_print_phdr(const Elf32_Phdr *phdr);


#endif	/* elf.h */
