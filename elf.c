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
#include "elf.h"

//! Module Information
typedef struct {
	uint16_t attr;	//!< Attribute
	uint16_t ver;	//!< Version
	char name[27];	//!< Name
	uint8_t type;	//!< Type
	void *gp;	//!< Global Pointer
	uint32_t expTop;	//!< Offset of the top of export table
	uint32_t expBtm;	//!< Offset of the bottom of export table
	uint32_t impTop;	//!< Offset of the top of import table
	uint32_t impBtm;	//!< Offset of the bottom of import table
	uint32_t nid;	//!< NID
	uint32_t unk[3];	//!< Unknown
	uint32_t start;	//!< Offset of module_start function
	uint32_t stop;	//!< Offset of module_stop function
	uint32_t exidxTop;	//!< Offset of the top of exidx section
	uint32_t exidxBtm;	//!< Offset of the bottom of exidx section
	uint32_t extabTop;	//!< Offset of the top of extab section
	uint32_t extabBtm;	//!< Offset of the bottom of extab section
} SceModuleInfo;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t num_tls_vars;
	uint32_t reserved1;
	uint32_t module_nid;
	char *lib_name;
	uint32_t reserved2;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
	uint32_t tls_nid_table;
	uint32_t tls_entry_table;
} SceImportsTable2xx;

typedef struct {
	uint16_t size;
	uint16_t lib_version;
	uint16_t attribute;
	uint16_t num_functions;
	uint16_t num_vars;
	uint16_t unknown1;
	uint32_t module_nid;
	char *lib_name;
	uint32_t func_nid_table;
	uint32_t func_entry_table;
	uint32_t var_nid_table;
	uint32_t var_entry_table;
} SceImportsTable3xx;

void convertToImportsTable3xx(SceImportsTable2xx *import_2xx, SceImportsTable3xx *import_3xx) {
	memset(import_3xx, 0, sizeof(SceImportsTable3xx));

	if (import_2xx->size == sizeof(SceImportsTable2xx)) {
		import_3xx->size = import_2xx->size;
		import_3xx->lib_version = import_2xx->lib_version;
		import_3xx->attribute = import_2xx->attribute;
		import_3xx->num_functions = import_2xx->num_functions;
		import_3xx->num_vars = import_2xx->num_vars;
		import_3xx->module_nid = import_2xx->module_nid;
		import_3xx->lib_name = import_2xx->lib_name;
		import_3xx->func_nid_table = import_2xx->func_nid_table;
		import_3xx->func_entry_table = import_2xx->func_entry_table;
		import_3xx->var_nid_table = import_2xx->var_nid_table;
		import_3xx->var_entry_table = import_2xx->var_entry_table;
	} else if (import_2xx->size == sizeof(SceImportsTable3xx)) {
		memcpy(import_3xx, import_2xx, sizeof(SceImportsTable3xx));
	}
}

int checkForUnsafeImports(void *buffer) {
	Elf32_Ehdr *ehdr = (Elf32_Ehdr *)buffer;
	Elf32_Phdr *phdr = (Elf32_Phdr *)((uint32_t)buffer + ehdr->e_phoff);

	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		return -1;
	}

	uint32_t segment = ehdr->e_entry >> 30;
	uint32_t offset = ehdr->e_entry & 0x3FFFFFFF;

	uint32_t text_addr = (uint32_t)buffer + phdr[segment].p_offset;

	SceModuleInfo *mod_info = (SceModuleInfo *)(text_addr + offset);

	int has_dangerous_nids = 0;
	int has_unsafe_libraries = 0;

	uint32_t i = mod_info->impTop;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx import;
		convertToImportsTable3xx((void *)text_addr + i, &import);

		char *libname = (char *)(text_addr + import.lib_name - phdr[segment].p_vaddr);
		uint32_t *func_nid_table = (uint32_t *)(text_addr + import.func_nid_table - phdr[segment].p_vaddr);

		if (strcmp(libname, "SceVshBridge") == 0) {
			int j;
			for (j = 0; j < import.num_functions; j++) {
				// Check for dangerous _vshIoMount/vshIoUmount
				if (func_nid_table[j] == 0x3C522C35 || func_nid_table[j] == 0x35BC26AC) {
					has_dangerous_nids = 1;
					break;
				}
			}
		} else if (strcmp(libname, "ScePromoterUtil") == 0 || strcmp(libname, "SceShellSvc") == 0) {
			has_unsafe_libraries = 1;
		}

		i += import.size;
	}

	if (has_dangerous_nids)
		return 2; // Really not safe bro
	
	if (has_unsafe_libraries)
		return 1; // Unsafe, but won't kill you

	return 0; // Safe
}

char *uncompressBuffer(const Elf32_Ehdr *ehdr, const Elf32_Phdr *phdr, const segment_info *segment,
		       const char *buffer) {
	if (ehdr->e_ident[EI_MAG0] != ELFMAG0 ||
	    ehdr->e_ident[EI_MAG1] != ELFMAG1 ||
	    ehdr->e_ident[EI_MAG2] != ELFMAG2 ||
	    ehdr->e_ident[EI_MAG3] != ELFMAG3) {
		return NULL;
	}

	int i;
	// sum all segment size
	uint32_t total_sz = 0;
	for (i = 0; i < ehdr->e_phnum; i++) {
		total_sz += (phdr + i)->p_filesz;
	}

	char *out = malloc(total_sz);
	if (out == NULL) {
		return NULL;
	}

	// uncompress each segments
	char *buf = out;
	for (i = 0; i < ehdr->e_phnum; i++) {
		uint64_t offset = (segment + i)->offset - segment->offset;
		uint32_t size = (phdr + i)->p_filesz;

		if ((segment + i)->compression == 1) {
			memcpy(buf, buffer + offset, (segment + i)->length);
			buf += size;
			continue;
		}
		uLongf buf_len = size;
		int ret = uncompress((Bytef*)buf, &buf_len, (const Bytef*)buffer + offset, (segment + i)->length);
		if (ret != Z_OK) {
			free(out);
			return NULL;
		}
		buf += size;
	}
	return out;
}

