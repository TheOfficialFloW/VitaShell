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
#include "io_wrapper.h"
#include "init.h"
#include "file.h"
#include "utils.h"
#include "module.h"
/*
typedef struct {
	char modname[27];
	uint32_t text_addr;
	uint32_t text_size;
	uint32_t data_addr;
	uint32_t data_size;
} ModuleInfo;
*/
static NidTable *nid_table = NULL;
static int nids_count = 0;

uint32_t decode_arm_inst(uint32_t inst, uint8_t *type) {
	if (type)
		*type = INSTRUCTION_UNKNOWN;

	if (!(BIT_SET(inst, 31) && BIT_SET(inst, 30) && BIT_SET(inst, 29) && !BIT_SET(inst, 28))) {
		return -1;
	}

	if (BIT_SET(inst, 27)) {
		if (BIT_SET(inst, 26) && BIT_SET(inst, 25) && BIT_SET(inst, 24)) {
			if (type)
				*type = INSTRUCTION_SYSCALL;

			return 1;
		}
	} else if (!BIT_SET(inst, 26) && BIT_SET(inst, 25)) {
		if (BIT_SET(inst, 24) && !BIT_SET(inst, 23)) {
			if (!(!BIT_SET(inst, 21) && !BIT_SET(inst, 20))) {
				return -1;
			}

			if (!(BIT_SET(inst, 15) && BIT_SET(inst, 14) && !BIT_SET(inst, 13) && !BIT_SET(inst, 12))) {
				return -1;
			}

			if (type) {
				if (BIT_SET(inst, 22)) {
					*type = INSTRUCTION_MOVT;
				} else {
					*type = INSTRUCTION_MOVW;
				}
			}

			return (((inst << 12) >> 28) << 12) | ((inst << 20) >> 20);
		} else if (!BIT_SET(inst, 24) && !BIT_SET(inst, 23)) {
			if (!(BIT_SET(inst, 22) && !BIT_SET(inst, 21) && !BIT_SET(inst, 20) && BIT_SET(inst, 19) && BIT_SET(inst, 18) && BIT_SET(inst, 17) && BIT_SET(inst, 16))) {
				return -1;
			}

			if (!(BIT_SET(inst, 15) && BIT_SET(inst, 14) && !BIT_SET(inst, 13) && !BIT_SET(inst, 12))) {
				return -1;
			}

			if (type)
				*type = INSTRUCTION_ADR;

			return inst & 0xFFF;
		} else if (BIT_SET(inst, 24) && BIT_SET(inst, 23)) {
			if (!(BIT_SET(inst, 22) && BIT_SET(inst, 21))) {
				return -1;
			}

			if (type)
				*type = INSTRUCTION_MVN;

			return 0;
		}
	} else if (!BIT_SET(inst, 26) && !BIT_SET(inst, 25)) {
		if ((inst << 7) >> 11 == 0x12FFF1) {
			if (type)
				*type = INSTRUCTION_BRANCH;

			return 1;
		} else if ((inst << 7) >> 11 == 0x12FFF3) {
			if (type)
				*type = INSTRUCTION_BRANCH;

			return 1;
		}
	}

	return -1;
}

uint32_t encode_arm_inst(uint8_t type, uint16_t immed, uint16_t reg) {
	switch (type) {
		case INSTRUCTION_MOVW:
			return ((uint32_t)0xE30 << 20) | ((uint32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);

		case INSTRUCTION_MOVT:
			return ((uint32_t)0xE34 << 20) | ((uint32_t)(immed & 0xF000) << 4) | (immed & 0xFFF) | (reg << 12);

		case INSTRUCTION_SYSCALL:
			return (uint32_t)0xEF000000;

		case INSTRUCTION_BRANCH:
			return ((uint32_t)0xE12FFF1 << 4) | reg;
	}

	return 0;
}

void makeSyscallStub(uint32_t address, uint16_t syscall) {
	if (!address)
		return;

	uvl_unlock_mem();

	uint32_t *f = (uint32_t *)address;
	f[0] = encode_arm_inst(INSTRUCTION_MOVW, syscall, 12);
	f[1] = encode_arm_inst(INSTRUCTION_SYSCALL, 0, 0);
	f[2] = encode_arm_inst(INSTRUCTION_BRANCH, 0, 14);
	f[3] = encode_arm_inst(INSTRUCTION_UNKNOWN, 0, 0);

	uvl_lock_mem();

	uvl_flush_icache(f, 0x10);
}

void makeFunctionStub(uint32_t address, void *function) {
	if (!address)
		return;

	uvl_unlock_mem();

	uint32_t *f = (uint32_t *)address;
	f[0] = encode_arm_inst(INSTRUCTION_MOVW, (uint16_t)(int)function, 12);
	f[1] = encode_arm_inst(INSTRUCTION_MOVT, (uint16_t)((int)function >> 16), 12);
	f[2] = encode_arm_inst(INSTRUCTION_BRANCH, 0, 12);
	f[3] = encode_arm_inst(INSTRUCTION_UNKNOWN, 0, 0);

	uvl_lock_mem();

	uvl_flush_icache(f, 0x10);
}

void makeStub(uint32_t address, void *function) {
	if ((uint32_t)function < MAX_SYSCALL_VALUE) {
		makeSyscallStub(address, (uint16_t)function);
	} else {
		makeFunctionStub(address, function);
	}
}

void copyStub(uint32_t address, void *function) {
	if (!address)
		return;

	uvl_unlock_mem();

	memcpy((void *)address, function, 0x10);

	uvl_lock_mem();

	uvl_flush_icache((void *)address, 0x10);
}

void makeResultStub(uint32_t address, int result) {
	if (!address)
		return;

	uvl_unlock_mem();

	uint32_t *f = (uint32_t *)address;
	f[0] = encode_arm_inst(INSTRUCTION_MOVW, (uint16_t)(int)result, 0);
	f[1] = encode_arm_inst(INSTRUCTION_MOVT, (uint16_t)((int)result >> 16), 0);
	f[2] = encode_arm_inst(INSTRUCTION_BRANCH, 0, 14);
	f[3] = encode_arm_inst(INSTRUCTION_UNKNOWN, 0, 0);

	uvl_lock_mem();

	uvl_flush_icache(f, 0x10);
}

void makeArmDummyFunction0(uint32_t address) {
	if (!address)
		return;

	uvl_unlock_mem();

	*(uint32_t *)(address + 0x0) = 0xE3E00000; // mvn a1, #0
	*(uint32_t *)(address + 0x4) = 0xE12FFF1E; // bx lr
	*(uint32_t *)(address + 0x8) = 0xE1A00000; // mov a1, r0
	*(uint32_t *)(address + 0xC) = 0x00000000; // nop

	uvl_lock_mem();

	uvl_flush_icache((void *)address, 0x10);
}

void makeThumbDummyFunction0(uint32_t address) {
	if (!address)
		return;

	uvl_unlock_mem();

	*(uint16_t *)(address + 0x0) = 0x2000; // movs a1, #0
	*(uint16_t *)(address + 0x2) = 0x4770; // bx lr
	
	uvl_lock_mem();

	uvl_flush_icache((void *)address, 0x4);
}

uint32_t extractFunctionStub(uint32_t address) {
	uint8_t type;

	uint32_t *f = (uint32_t *)address;

	if (!f)
		return 0;

	uint32_t movw = decode_arm_inst(f[0], &type);
	if (type != INSTRUCTION_MOVW)
		return 0;

	uint32_t movt = decode_arm_inst(f[1], &type);
	if (type != INSTRUCTION_MOVT)
		return 0;

	uint32_t branch = decode_arm_inst(f[2], &type);
	if (type != INSTRUCTION_BRANCH)
		return 0;

	uint32_t nop = decode_arm_inst(f[3], &type);
	if (type != INSTRUCTION_UNKNOWN)
		return 0;

	return (movt << 16) | movw;
}

uint32_t extractSyscallStub(uint32_t address) {
	uint8_t type;

	uint32_t *f = (uint32_t *)address;

	if (!f)
		return 0;

	uint32_t movw = decode_arm_inst(f[0], &type);
	if (type != INSTRUCTION_MOVW)
		return 0;

	uint32_t syscall = decode_arm_inst(f[1], &type);
	if (type != INSTRUCTION_SYSCALL)
		return 0;

	uint32_t branch = decode_arm_inst(f[2], &type);
	if (type != INSTRUCTION_BRANCH)
		return 0;

	uint32_t nop = decode_arm_inst(f[3], &type);
	if (type != INSTRUCTION_UNKNOWN)
		return 0;

	return movw;
}

uint32_t extractStub(uint32_t address) {
	uint32_t value = extractSyscallStub(address);

	if (!value)
		value = extractFunctionStub(address);

	return value;
}

int getModuleInfo(SceUID mod, char modname[27], uint32_t *text_addr, uint32_t *text_size) {
	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);
	if (sceKernelGetModuleInfo(mod, &info) < 0)
		return 0;

	if (modname)
		strcpy(modname, info.module_name);

	if (text_addr)
		*text_addr = (uint32_t)info.segments[0].vaddr;

	if (text_size)
		*text_size = (uint32_t)info.segments[0].memsz;

	return 1;
}

int findModuleByName(char *name, uint32_t *text_addr, uint32_t *text_size) {
	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	if (sceKernelGetModuleList(0xFF, mod_list, &mod_count) >= 0) {
		int i;
		for (i = mod_count - 1; i >= 0; i--) {
			SceKernelModuleInfo info;
			info.size = sizeof(SceKernelModuleInfo);
			if (sceKernelGetModuleInfo(mod_list[i], &info) >= 0) {
				if (strcmp(info.module_name, name) == 0) {
					return getModuleInfo(mod_list[i], NULL, text_addr, text_size);
				}
			}
		}
	}

	return 0;
}

int findModuleByAddress(uint32_t address, char modname[27], int *section, uint32_t *text_addr, uint32_t *text_size) {
	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	if (sceKernelGetModuleList(0xFF, mod_list, &mod_count) >= 0) {
		int i;
		for (i = mod_count - 1; i >= 0; i--) {
			SceKernelModuleInfo info;
			info.size = sizeof(SceKernelModuleInfo);
			if (sceKernelGetModuleInfo(mod_list[i], &info) >= 0) {
				int j;
				for (j = 0; j < 4; j++) {
					if (address >= (uint32_t)info.segments[j].vaddr && address < ((uint32_t)info.segments[j].vaddr + (uint32_t)info.segments[j].memsz)) {
						*section = j;
						return getModuleInfo(mod_list[i], modname, text_addr, text_size);
					}
				}
			}
		}
	}

	return 0;
}

SceModuleInfo *findModuleInfo(char *modname, uint32_t text_addr, uint32_t text_size) {
	SceModuleInfo *mod_info = NULL;

	uint32_t i;
	for (i = 0; i < text_size; i += 4) {
		if (strcmp((char *)(text_addr + i), modname) == 0) {
			mod_info = (SceModuleInfo *)(text_addr + i - 0x4);
			break;
		}
	}

	return mod_info;
}

uint32_t findModuleExportByInfo(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid) {
	if (!mod_info)
		return 0;

	uint32_t i = mod_info->expTop;
	while (i < mod_info->expBtm) {
		SceExportsTable *export = (SceExportsTable *)(text_addr + i);

		if (export->lib_name && strcmp(export->lib_name, libname) == 0) {
			int j;
			for (j = 0; j < export->num_functions; j++) {
				if (export->nid_table[j] == nid) {
					return (uint32_t)export->entry_table[j];
				}
			}
		}

		i += export->size;
	}

	return 0;
}

uint32_t findModuleExportByName(char *modname, char *libname, uint32_t nid) {
	uint32_t text_addr = 0, text_size = 0;
	if (findModuleByName(modname, &text_addr, &text_size) == 0)
		return 0;

	return findModuleExportByInfo(findModuleInfo(modname, text_addr, text_size), text_addr, libname, nid);
}

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

uint32_t findModuleImportByInfo(SceModuleInfo *mod_info, uint32_t text_addr, char *libname, uint32_t nid) {
	if (!mod_info)
		return 0;

	uint32_t i = mod_info->impTop;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx import;
		convertToImportsTable3xx((void *)text_addr + i, &import);

		if (import.lib_name && strcmp(import.lib_name, libname) == 0) {
			int j;
			for (j = 0; j < import.num_functions; j++) {
				if (import.func_nid_table[j] == nid) {
					return (uint32_t)import.func_entry_table[j];
				}
			}
		}

		i += import.size;
	}

	return 0;
}

uint32_t findModuleImportByUID(SceUID mod, char *libname, uint32_t nid) {
	char modname[27];
	uint32_t text_addr = 0, text_size = 0;
	if (getModuleInfo(mod, modname, &text_addr, &text_size) == 0)
		return 0;

	return findModuleImportByInfo(findModuleInfo(modname, text_addr, text_size), text_addr, libname, nid);
}

uint32_t findModuleImportByValue(char *modname, char *libname, uint32_t value) {
	uint32_t text_addr = 0, text_size = 0;
	if (findModuleByName(modname, &text_addr, &text_size) == 0)
		return 0;

	SceModuleInfo *mod_info = findModuleInfo(modname, text_addr, text_size);
	if (!mod_info)
		return 0;

	uint32_t i = mod_info->impTop;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx import;
		convertToImportsTable3xx((void *)text_addr + i, &import);

		if (import.lib_name && strcmp(import.lib_name, libname) == 0) {
			int j;
			for (j = 0; j < import.num_functions; j++) {
				if (extractStub((uint32_t)import.func_entry_table[j]) == value) {
					return (uint32_t)import.func_entry_table[j];
				}
			}
		}

		i += import.size;
	}

	return 0;
}

void duplicateModule(char *name, uint32_t *text_addr, uint32_t *text_size) {
	uint32_t ori_text_addr = 0, ori_text_size = 0;
	findModuleByName(name, &ori_text_addr, &ori_text_size);

	unsigned int length = ALIGN(ori_text_size, 0x100000);
	uint32_t new_text_addr = (uint32_t)uvl_alloc_code_mem(&length);

	uvl_unlock_mem();
	memcpy((void *)new_text_addr, (void *)ori_text_addr, ori_text_size);
	uvl_lock_mem();
	uvl_flush_icache((void *)new_text_addr, ori_text_size);

	if (text_addr)
		*text_addr = new_text_addr;

	if (text_size)
		*text_size = ori_text_size;
}

int dumpModule(SceUID uid) {
	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);
	int res = sceKernelGetModuleInfo(uid, &info);
	if (res < 0)
		return res;

	int i;
	for (i = 0; i < 2; i++) {
		if (info.segments[i].vaddr) {
			char *data = malloc(ALIGN(info.segments[i].memsz, 0x1000));
			memcpy(data, info.segments[i].vaddr, info.segments[i].memsz);

			if (i == 0 && strncmp(info.path, "os0:us/", 7) == 0) {
				SceModuleInfo *mod_info = findModuleInfo(info.module_name, (uint32_t)info.segments[0].vaddr, info.segments[0].memsz);
				if (mod_info) {
					uint32_t i = mod_info->impTop;
					while (i < mod_info->impBtm) {
						SceImportsTable3xx import;
						convertToImportsTable3xx(info.segments[0].vaddr + i, &import);

						int count_unknown_syscalls = 0;

						int j;
						for (j = 0; j < import.num_functions; j++) {
							uint32_t value = extractStub((uint32_t)import.func_entry_table[j]);
							uint32_t nid = getNidByValue(value);
							if (nid == 0)
								nid = count_unknown_syscalls++;

							// Resolve known NIDs
							*(uint32_t *)((uint32_t)data + (uint32_t)&import.func_nid_table[j] - (uint32_t)info.segments[0].vaddr) = nid;
						}

						i += import.size;
					}
				}
			}

			char path[MAX_PATH_LENGTH];
			sprintf(path, "cache0:/modules/%s_0x%08X_%d.bin", info.module_name, (unsigned int)info.segments[i].vaddr, i);
			WriteFile(path, data, info.segments[i].memsz);

			free(data);
		}
	}

	return 0;
}

int dumpModules() {
	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	int res = sceKernelGetModuleList(0xFF, mod_list, &mod_count);
	if (res < 0)
		return res;

	removePath("cache0:/modules", NULL, 0, NULL);
	fileIoMkdir("cache0:/modules", 0777);

	int i;
	for (i = mod_count - 1; i >= 0; i--) {
		dumpModule(mod_list[i]);
	}

	return 0;
}

void loadDumpModules() {
	removePath("cache0:/modules", NULL, 0, NULL);
	fileIoMkdir("cache0:/modules", 0777);

	SceUID dfd = fileIoDopen(sys_external_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = fileIoDread(dfd, &dir);
			if (res > 0) {
				if (!SCE_S_ISDIR(dir.d_stat.st_mode)) {
					char path[128];
					sprintf(path, "%s%s", sys_external_path, dir.d_name);

					SceUID mod = sceKernelLoadModule(path, 0, NULL);
					if (mod >= 0)
						dumpModule(mod);

					sceKernelUnloadModule(mod, 0, NULL);
				}
			}
		} while (res > 0);

		fileIoDclose(dfd);
	}

	char webcore_path[128];
	sprintf(webcore_path, "%s%s", data_external_path, "webcore");

	dfd = fileIoDopen(webcore_path);
	if (dfd >= 0) {
		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = fileIoDread(dfd, &dir);
			if (res > 0) {
				if (!SCE_S_ISDIR(dir.d_stat.st_mode)) {
					char path[128];
					sprintf(path, "%s/%s", webcore_path, dir.d_name);

					SceUID mod = sceKernelLoadModule(path, 0, NULL);
					if (mod >= 0)
						dumpModule(mod);

					sceKernelUnloadModule(mod, 0, NULL);
				}
			}
		} while (res > 0);

		fileIoDclose(dfd);
	}
}

uint32_t getValueByNid(uint32_t nid) {
	int i;
	for (i = 0; i < nids_count; i++) {
		if (nid_table[i].nid == nid) {
			return nid_table[i].value;
		}
	}

	return 0;
}

uint32_t getNidByValue(uint32_t value) {
	int i;
	for (i = 0; i < nids_count; i++) {
		if (nid_table[i].value == value) {
			return nid_table[i].nid;
		}
	}

	return 0;
}

int addNidValue(uint32_t nid, uint32_t value) {
	switch (nid) {
		case 0x79F8E492:
		case 0x913482A9:
		case 0x935CD196:
		case 0x6C2224BA:
			return 0;
	}

	int i;
	for (i = 0; i < nids_count; i++) {
		if (nid_table[i].nid == nid) {
			break;
		}
	}

	if (i == nids_count) {
		nid_table[nids_count].nid = nid;
		nid_table[nids_count].value = value;
		nids_count++;
	}

	return 1;
}

void addExportNids(SceModuleInfo *mod_info, uint32_t text_addr) {
	uint32_t i = mod_info->expTop;
	while (i < mod_info->expBtm) {
		SceExportsTable *export = (SceExportsTable *)(text_addr + i);

		int j;
		for (j = 0; j < export->num_functions; j++) {
			uint32_t value = (uint32_t)export->entry_table[j];
			uint32_t nid = export->nid_table[j];
			addNidValue(nid, value);
		}

		i += export->size;
	}
}

void addImportNids(SceModuleInfo *mod_info, uint32_t text_addr, uint32_t reload_text_addr, int only_syscalls) {
	uint32_t i = mod_info->impTop;
	while (i < mod_info->impBtm) {
		SceImportsTable3xx import;
		convertToImportsTable3xx((void *)text_addr + i, &import);

		int j;
		for (j = 0; j < import.num_functions; j++) {
			uint32_t value = extractStub((uint32_t)import.func_entry_table[j]);
			uint32_t nid = *(uint32_t *)(reload_text_addr + (uint32_t)&import.func_nid_table[j] - text_addr);

			if (only_syscalls && value >= MAX_SYSCALL_VALUE)
				continue;

			addNidValue(nid, value);
		}

		i += import.size;
	}
}

void addNids(SceUID uid, int only_syscalls) {
	SceKernelModuleInfo info;
	info.size = sizeof(SceKernelModuleInfo);
	if (sceKernelGetModuleInfo(uid, &info) < 0)
		return;

	// Can be reloaded, so the imports are not resolved yet, skip
	if (sceKernelUnloadModule(uid, 0, NULL) >= 0)
		return;

	int reload = 1; // If module cannot be reloaded, so its imports cannot be resolved
	SceUID reload_mod = -1;
	char reload_path[MAX_PATH_LENGTH];

	char reload_modname[27];
	uint32_t reload_text_addr = 0, reload_text_size = 0;
	SceModuleInfo *reload_mod_info;

	strcpy(reload_path, info.path);

	if (strncmp(info.path, "os0:us/", 7) == 0) {
		reload = 0;
	} else if (strncmp(info.path, "ux0:/patch/", 11) == 0) {
//		sprintf(reload_path, "app0:/%s", info.path + 21);
		reload = 0;
	} else if (strncmp(info.path, "vs0:sys/external/", 17) == 0) {
		sprintf(reload_path, "%s%s", sys_external_path, info.path + 17);
	}

	if (reload) {
		reload_mod = sceKernelLoadModule(reload_path, 0, NULL);
		if (reload_mod < 0)
			return;

		// Get reload module info (NID unpoisoned, syscall unresolved)
		if(!getModuleInfo(reload_mod, reload_modname, &reload_text_addr, &reload_text_size))
			return;

		reload_mod_info = findModuleInfo(reload_modname, reload_text_addr, reload_text_size);
		if (!reload_mod_info)
			return;
	}

	// Get module info (NID poisoned, syscall resolved)
	char modname[27];
	uint32_t text_addr = 0, text_size = 0;
	if(!getModuleInfo(uid, modname, &text_addr, &text_size))
		return;

	SceModuleInfo *mod_info = findModuleInfo(modname, text_addr, text_size);
	if (!mod_info)
		return;

	// Add import nids
	if (reload) {
		addImportNids(mod_info, text_addr, reload_text_addr, only_syscalls);
	}

	// Add export nids
	if (!only_syscalls)
		addExportNids(mod_info, text_addr);

	// Unload
	if (reload)
		sceKernelUnloadModule(reload_mod, 0, NULL);	
}

int setupNidTable() {
	SceUID mod_list[MAX_MODULES];
	int mod_count = MAX_MODULES;

	// Allocate and clear nid table
	if (!nid_table)
		nid_table = malloc(MAX_NIDS * sizeof(NidTable));

	memset(nid_table, 0, MAX_NIDS * sizeof(NidTable));
	nids_count = 0;

	// Add preload exports and imports
	int res = sceKernelGetModuleList(0xFF, mod_list, &mod_count);
	if (res < 0)
		return res;

	int i;
	for (i = mod_count - 1; i >= 0; i--) {
		addNids(mod_list[i], 0);
	}

	return 0;
}

void freeNidTable() {
	if (nid_table) {
		free(nid_table);
		nid_table = NULL;
	}
	
	nids_count = 0;
}