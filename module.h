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

#ifndef __MODULE_H__
#define __MODULE_H__

#define HIJACK_STUB(a, f, ptr) \
{ \
	uint8_t type = 0; \
	uint32_t low = decode_arm_inst(*(uint32_t *)(a), &type); \
	uint32_t high = decode_arm_inst(*(uint32_t *)(a + 4), &type) << 16; \
	\
	ptr = (void *)(high | low); \
	\
	makeFunctionStub(a, f); \
}

#define BIT_SET(i, b) (i & (0x1 << b))

#define INSTRUCTION_UNKNOWN		0
#define INSTRUCTION_MOVW		1
#define INSTRUCTION_MOVT		2
#define INSTRUCTION_SYSCALL		3
#define INSTRUCTION_BRANCH		4
#define INSTRUCTION_ADR			5
#define INSTRUCTION_MVN			6

#define MAX_MODULES 128
#define MAX_NIDS 0x10000

typedef struct {
	uint16_t size;
	uint8_t lib_version[2];
	uint16_t attribute;
	uint16_t num_functions;
	uint32_t num_vars;
	uint32_t num_tls_vars;
	uint32_t module_nid;
	char *lib_name;
	uint32_t *nid_table;
	void **entry_table;
} SceExportsTable;

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
	uint32_t *func_nid_table;
	void **func_entry_table;
	uint32_t *var_nid_table;
	void **var_entry_table;
	uint32_t *tls_nid_table;
	void **tls_entry_table;
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
	uint32_t *func_nid_table;
	void **func_entry_table;
	uint32_t *var_nid_table;
	void **var_entry_table;
} SceImportsTable3xx;

typedef struct {
	uint32_t nid;
	uint32_t value;
} NidTable;

uint32_t decode_arm_inst(uint32_t inst, uint8_t *type);
uint32_t encode_arm_inst(uint8_t type, uint16_t immed, uint16_t reg);

void makeSyscallStub(uint32_t address, uint16_t syscall);
void makeFunctionStub(uint32_t address, void *function);
void copyStub(uint32_t address, void *function);

uint32_t extractFunctionStub(uint32_t address);
uint32_t extractSyscallStub(uint32_t address);
uint32_t extractStub(uint32_t address);

int getModuleInfo(SceUID mod, char modname[27], uint32_t *text_addr, uint32_t *text_size);

int findModuleByName(char *name, uint32_t *text_addr, uint32_t *text_size);
int findModuleByAddress(uint32_t address, char modname[27], int *section, uint32_t *text_addr, uint32_t *text_size);

SceModuleInfo *findModuleInfo(char *modname, uint32_t text_addr, uint32_t text_size);

uint32_t findModuleExportByInfo(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid);
uint32_t findModuleExportByName(char *modname, char *libname, uint32_t nid);

uint32_t findModuleImportByInfo(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid);
uint32_t findModuleImportByUID(SceUID mod, char *libname, uint32_t nid);

int findSyscallInModuleImports(uint32_t syscall, char modulename[27], uint32_t *addr);

void duplicateModule(char *name, uint32_t *text_addr, uint32_t *text_size);

int dumpModule(SceUID uid);
int dumpModules();

uint32_t getNid(uint32_t val);
void addNid(uint32_t nid, uint32_t val);
int setupNidTable();
void freeNidTable();

#endif