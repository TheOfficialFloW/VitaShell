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
#include "init.h"
#include "file.h"
#include "utils.h"
#include "module.h"
#include "elf_types.h"

// sceKernelGetThreadmgrUIDClass is not available
// suspend/resume for vm returns 0x80020008

// sceKernelDeleteMutex not resolved by UVL

/*
	TODO:
	- Unload prxs
	- Delete semaphores, events, callbacks, msg pipes, rw locks
*/

typedef struct {
	char *library;
	uint32_t nid;
	void *function;
} PatchNID;

#define MAX_AUDIO_PORTS 3
#define MAX_SYNC_OBJECTS 3
#define MAX_GXM_PRGRAMS 16
#define MAX_UIDS 32

#define INVALID_UID -1

static int force_exit = 0;

static SceUID exit_thid = INVALID_UID;

static SceUID UVLTemp_id = INVALID_UID, UVLHomebrew_id = INVALID_UID;
static uint32_t hb_text_addr = 0;
static SceModuleInfo hb_mod_info;

static SceUID hb_fds[MAX_UIDS];
static SceUID hb_dfds[MAX_UIDS];
static SceUID hb_thids[MAX_UIDS];
static SceUID hb_semaids[MAX_UIDS];
static SceUID hb_mutexids[MAX_UIDS];
static SceUID hb_blockids[MAX_UIDS];

static int hb_audio_ports[MAX_AUDIO_PORTS];

static SceGxmContext *hb_gxm_context = NULL;
static SceGxmRenderTarget *hb_gxm_render = NULL;
static SceGxmShaderPatcher *hb_shader_patcher = NULL;

static SceGxmSyncObject *hb_sync_objects[MAX_SYNC_OBJECTS];

static SceGxmFragmentProgram *hb_fragment_programs[MAX_GXM_PRGRAMS];
static SceGxmShaderPatcherId hb_fragment_program_ids[MAX_GXM_PRGRAMS];

static SceGxmVertexProgram *hb_vertex_programs[MAX_GXM_PRGRAMS];
static SceGxmShaderPatcherId hb_vertex_program_ids[MAX_GXM_PRGRAMS];

int isValidElf(char *file) {
	Elf32_Ehdr header;
	ReadFile(file, &header, sizeof(Elf32_Ehdr));
	if (header.e_magic != ELF_MAGIC || header.e_type != 0xFE04 || header.e_machine != 40) {
		return 0;
	}

	return 1;
}

SceModuleInfo *getElfModuleInfo(void *buf) {
	Elf32_Ehdr *header = (Elf32_Ehdr *)buf;
	Elf32_Phdr *program = (Elf32_Phdr *)((uint32_t)header + header->e_phoff);

	uint32_t index = ((uint32_t)header->e_entry & 0xC0000000) >> 30;
	uint32_t offset = (uint32_t)header->e_entry & 0x3FFFFFFF;

	return (SceModuleInfo *)((uint32_t)buf + program[index].p_offset + offset);	
}

void initHomebrewPatch() {
	force_exit = 0;

	exit_thid = INVALID_UID;

	UVLTemp_id = INVALID_UID;
	UVLHomebrew_id = INVALID_UID;
	hb_text_addr = 0;
	memset((void *)&hb_mod_info, 0, sizeof(SceModuleInfo));

	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		hb_fds[i] = INVALID_UID;
		hb_dfds[i] = INVALID_UID;
		hb_thids[i] = INVALID_UID;
		hb_semaids[i] = INVALID_UID;
		hb_mutexids[i] = INVALID_UID;
		hb_blockids[i] = INVALID_UID;
	}

	memset(hb_audio_ports, 0, sizeof(hb_audio_ports));

	hb_gxm_context = NULL;
	hb_gxm_render = NULL;
	hb_shader_patcher = NULL;

	memset(hb_sync_objects, 0, sizeof(hb_sync_objects));
	memset(hb_fragment_programs, 0, sizeof(hb_fragment_programs));
	memset(hb_fragment_program_ids, 0, sizeof(hb_fragment_program_ids));
	memset(hb_vertex_programs, 0, sizeof(hb_vertex_programs));
	memset(hb_vertex_program_ids, 0, sizeof(hb_vertex_program_ids));	
}

void waitVblankStart() {
	int i;
	for (i = 0; i < 10; i++) {
		sceDisplayWaitVblankStart();
	}
}

void finishGxm() {
	int i;

	// Wait until rendering is done
	sceGxmFinish(hb_gxm_context);
/*
	// THIS CRASHES, WHY???
	// Clean up allocations
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_programs[i] != NULL)
			sceGxmShaderPatcherReleaseFragmentProgram(hb_shader_patcher, hb_fragment_programs[i]);

		if (hb_vertex_programs[i] != NULL)
			sceGxmShaderPatcherReleaseVertexProgram(hb_shader_patcher, hb_vertex_programs[i]);
	}
*/
	// Wait until display queue is finished before deallocating display buffers
	sceGxmDisplayQueueFinish();

	// Clean up display queue
	for (i = 0; i < MAX_SYNC_OBJECTS; i++) {
		// Destroy the sync object
		if (hb_sync_objects[i] != NULL)
			sceGxmSyncObjectDestroy(hb_sync_objects[i]);
	}
/*
	// Unregister programs and destroy shader patcher
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_program_ids[i] != NULL)
			sceGxmShaderPatcherUnregisterProgram(hb_shader_patcher, hb_fragment_program_ids[i]);

		if (hb_vertex_program_ids[i] != NULL)
			sceGxmShaderPatcherUnregisterProgram(hb_shader_patcher, hb_vertex_program_ids[i]);
	}

	sceGxmShaderPatcherDestroy(hb_shader_patcher);
*/
	// Destroy the render target
	sceGxmDestroyRenderTarget(hb_gxm_render);

	// Destroy the context
	sceGxmDestroyContext(hb_gxm_context);

	// Terminate libgxm
	sceGxmTerminate();
}

void releaseAudioPorts() {
	int i;
	for (i = 0; i < MAX_AUDIO_PORTS; i++) {
		sceAudioOutReleasePort(hb_audio_ports[i]);
	}
}

void closeFileDescriptors() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_fds[i] >= 0)
			sceIoClose(hb_fds[i]);
		
		if (hb_dfds[i] >= 0)
			sceIoDclose(hb_dfds[i]);
	}
}

// TODO: delete sema
void signalSema() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_semaids[i] >= 0) {
			int res = sceKernelSignalSema(hb_semaids[i], 1);
			debugPrintf("Signal sema 0x%08X: 0x%08X\n", hb_semaids[i], res);
			res = sceKernelDeleteSema(hb_semaids[i]);
			debugPrintf("Delete sema 0x%08X: 0x%08X\n", hb_semaids[i], res);
		}
	}
}

// TODO: delete mutex
void unlockMutex() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_mutexids[i] >= 0) {
			int res = sceKernelUnlockMutex(hb_mutexids[i], 1);
			debugPrintf("Unlock mutex 0x%08X: 0x%08X\n", hb_mutexids[i], res);
		}
	}
}

void waitThreadEnd() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_thids[i] >= 0) {
			//debugPrintf("Wait for 0x%08X\n", hb_thids[i]);
			sceKernelWaitThreadEnd(hb_thids[i], NULL, NULL);
		}
	}	
}

int exitThread() {
	debugPrintf("Exiting 0x%08X...\n", sceKernelGetThreadId());
	signalSema();
	unlockMutex();
	debugPrintf("Wait for threads...\n");
	waitThreadEnd();
	waitVblankStart();
	return sceKernelExitDeleteThread(0);
}

PatchNID patches_exit[] = {
	{ "SceAudio", 0x02DB3F5F, exitThread }, // sceAudioOutOutput // crashes mGBA

	{ "SceCtrl", 0x104ED1A7, exitThread }, // sceCtrlPeekBufferNegative
	{ "SceCtrl", 0x15F96FB0, exitThread }, // sceCtrlReadBufferNegative
	{ "SceCtrl", 0x67E7AB83, exitThread }, // sceCtrlReadBufferPositive
	{ "SceCtrl", 0xA9C3CED6, exitThread }, // sceCtrlPeekBufferPositive

	{ "SceGxm", 0x8734FF4E, exitThread }, // sceGxmBeginScene

	//{ "SceDisplay", 0x5795E898, exitThread }, // sceDisplayWaitVblankStart

	{ "SceLibKernel", 0x0C7B834B, exitThread }, // sceKernelWaitSema
	{ "SceLibKernel", 0x174692B4, exitThread }, // sceKernelWaitSemaCB
	{ "SceLibKernel", 0x1D8D7945, exitThread }, // sceKernelLockMutex
	{ "SceLibKernel", 0x72FC1F54, exitThread }, // sceKernelTryLockMutex
	{ "SceLibKernel", 0xE6B761D1, exitThread }, // sceKernelSignalSema

	{ "SceThreadmgr", 0x1A372EC8, exitThread }, // sceKernelUnlockMutex
	//{ "SceThreadmgr", 0x4B675D05, exitThread }, // sceKernelDelayThread
};

// TODO: maybe patch all imports?
void PatchHomebrew() {
	int i;
	for (i = 0; i < (sizeof(patches_exit) / sizeof(PatchNID)); i++) {
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, hb_text_addr, patches_exit[i].library, patches_exit[i].nid), patches_exit[i].function);
	}

	force_exit = 1;
}

int exit_thread(SceSize args, void *argp) {
	while (1) {
		SceCtrlData pad;
		sceCtrlPeekBufferPositive(0, &pad, 1);

		if (holdButtons(&pad, SCE_CTRL_START, 1 * 1000 * 1000)) {
			PatchHomebrew();
			break;
		}

		sceKernelDelayThread(10 * 1000);
	}

	return sceKernelExitDeleteThread(0);
}

void loadElf(char *file) {
	// Finish vita2dlib
	finishVita2dLib();
/*
	// Empty lists
	fileListEmpty(&copy_list);
	fileListEmpty(&mark_list);
	fileListEmpty(&file_list);
*/
/*
	// Free language container
	freeLanguageContainer();

	// Free heap
	_free_vita_heap();
*/
	// Init
	initHomebrewPatch();
/*
	scePowerSetBusClockFrequency(166); // that's actually gpu
	scePowerSetConfigurationMode(0x00010880);
*/
	// Load
	uvl_load(file);

	// Wait...otherwise it crashes
	waitVblankStart();

	// Clean up
	if (force_exit) {
		// Wait for exit thread to end
		sceKernelWaitThreadEnd(exit_thid, NULL, NULL);

		// Finish gxm
		finishGxm();

		// Release audio ports
		releaseAudioPorts();

		// Close file descriptors
		closeFileDescriptors();
	}

	// Unmap gxm memories and free blocks
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_blockids[i] >= 0) {
			int res;

			void *mem = NULL;
			if (sceKernelGetMemBlockBase(hb_blockids[i], &mem) < 0)
				continue;

			if (force_exit) {
				res = sceGxmUnmapMemory(mem);
				if (res < 0)
					sceGxmUnmapVertexUsseMemory(mem);
				if (res < 0)
					sceGxmUnmapFragmentUsseMemory(mem);
			}

			res = sceKernelFreeMemBlock(hb_blockids[i]);
			// debugPrintf("free 0x%08X (0x%08X): 0x%08X\n", mem, hb_blockids[i], res);
		}
	}

/*
	// Init heap
	_init_vita_heap();

	// Load language
	loadLanguage(language);
*/
	// Reset lists
	// resetFileLists();

	// Init vita2dlib
	initVita2dLib();
}

int sceKernelExitProcessPatchedHB(int res) {
	// debugPrintf("%s\n", __FUNCTION__);
	return 0;
}

int sceAudioOutOpenPortPatchedHB(int type, int len, int freq, int mode) {
	int port = sceAudioOutOpenPort(type, len, freq, mode);

	if (type < MAX_AUDIO_PORTS)
		hb_audio_ports[type] = port;

	return port;
}

int sceGxmCreateContextPatchedHB(const SceGxmContextParams *params, SceGxmContext **context) {
	int res = sceGxmCreateContext(params, context);
	hb_gxm_context = *context;
	// debugPrintf("%s 0x%08X\n", __FUNCTION__, hb_gxm_context);
	return res;
}

int sceGxmCreateRenderTargetPatchedHB(const SceGxmRenderTargetParams *params, SceGxmRenderTarget **renderTarget) {
	int res = sceGxmCreateRenderTarget(params, renderTarget);
	hb_gxm_render = *renderTarget;
	// debugPrintf("%s 0x%08X\n", __FUNCTION__, hb_gxm_render);
	return res;
}

int sceGxmShaderPatcherCreateVertexProgramPatchedHB(SceGxmShaderPatcher *shaderPatcher, SceGxmShaderPatcherId programId, const SceGxmVertexAttribute *attributes, unsigned int attributeCount, const SceGxmVertexStream *streams, unsigned int streamCount, SceGxmVertexProgram **vertexProgram) {
	int res = sceGxmShaderPatcherCreateVertexProgram(shaderPatcher, programId, attributes, attributeCount, streams, streamCount, vertexProgram);

	// debugPrintf("%s 0x%08X 0x%08X\n", __FUNCTION__, programId, *vertexProgram);

	if (hb_shader_patcher == NULL)
		hb_shader_patcher = shaderPatcher;

	int i;
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_vertex_program_ids[i] == NULL) {
			hb_vertex_program_ids[i] = programId;
			break;
		}
	}

	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_vertex_programs[i] == NULL) {
			hb_vertex_programs[i] = *vertexProgram;
			break;
		}
	}

	return res;
}

int sceGxmShaderPatcherCreateFragmentProgramPatchedHB(SceGxmShaderPatcher *shaderPatcher, SceGxmShaderPatcherId programId, SceGxmOutputRegisterFormat outputFormat, SceGxmMultisampleMode multisampleMode, const SceGxmBlendInfo *blendInfo, const SceGxmProgram *vertexProgram, SceGxmFragmentProgram **fragmentProgram) {
	int res = sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, programId, outputFormat, multisampleMode, blendInfo, vertexProgram, fragmentProgram);

	// debugPrintf("%s 0x%08X 0x%08X\n", __FUNCTION__, programId, *fragmentProgram);

	if (hb_shader_patcher == NULL)
		hb_shader_patcher = shaderPatcher;

	int i;
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_program_ids[i] == NULL) {
			hb_fragment_program_ids[i] = programId;
			break;
		}
	}

	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_programs[i] == NULL) {
			hb_fragment_programs[i] = *fragmentProgram;
			break;
		}
	}

	return res;
}

int sceGxmSyncObjectCreatePatchedHB(SceGxmSyncObject **syncObject) {
	int res = sceGxmSyncObjectCreate(syncObject);

	int i;
	for (i = 0; i < MAX_SYNC_OBJECTS; i++) {
		if (hb_sync_objects[i] == NULL) {
			// debugPrintf("%s 0x%08X %d\n", __FUNCTION__, *syncObject, i);
			hb_sync_objects[i] = *syncObject;
			break;
		}
	}

	return res;
}

int sceKernelExitDeleteThreadPatchedHB(int status) {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_thids[i] == sceKernelGetThreadId()) {
			hb_thids[i] = INVALID_UID;
			break;
		}
	}

	return sceKernelExitDeleteThread(status);
}

SceUID sceKernelCreateThreadPatchedHB(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMask, const SceKernelThreadOptParam *option) {
	SceUID thid = sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);

	// debugPrintf("%s 0x%08X\n", name, thid);

	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_thids[i] < 0) {
			hb_thids[i] = thid;
			break;
		}
	}

	return thid;
}

SceUID sceKernelAllocMemBlockPatchedHB(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_blockids[i] < 0) {
			hb_blockids[i] = blockid;
			break;
		}
	}

	return blockid;
}

int sceKernelFreeMemBlockPatchedHB(SceUID uid) {
	int res = sceKernelFreeMemBlock(uid);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_blockids[i] == uid) {
				hb_blockids[i] = INVALID_UID;
				break;
			}
		}
	}

	return res;
}

SceUID sceIoOpenPatchedHB(const char *file, int flags, SceMode mode) {
	SceUID fd = sceIoOpen(file, flags, mode);

	if (fd >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_fds[i] < 0) {
				hb_fds[i] = fd;
				break;
			}
		}
	}

	return fd;
}

int sceIoClosePatchedHB(SceUID fd) {
	int res = sceIoClose(fd);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_fds[i] == fd) {
				hb_fds[i] = INVALID_UID;
				break;
			}
		}
	}

	return res;
}

SceUID sceIoDopenPatchedHB(const char *dirname) {
	SceUID dfd = sceIoDopen(dirname);

	if (dfd >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_fds[i] < 0) {
				hb_fds[i] = dfd;
				break;
			}
		}
	}

	return dfd;
}

int sceIoDclosePatchedHB(SceUID fd) {
	int res = sceIoDclose(fd);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_dfds[i] == fd) {
				hb_dfds[i] = INVALID_UID;
				break;
			}
		}
	}

	return res;
}

SceUID sceKernelCreateSemaPatchedHB(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option) {
	SceUID semaid = sceKernelCreateSema(name, attr, initVal, maxVal, option);

	if (semaid >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_semaids[i] < 0) {
				hb_semaids[i] = semaid;
				break;
			}
		}
	}

	return semaid;
}

int sceKernelDeleteSemaPatchedHB(SceUID semaid) {
	int res = sceKernelDeleteSema(semaid);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_semaids[i] == semaid) {
				hb_semaids[i] = INVALID_UID;
				break;
			}
		}
	}

	return res;
}

SceUID sceKernelCreateMutexPatchedHB(const char *name, SceUInt attr, int initCount, SceKernelMutexOptParam *option) {
	SceUID mutexid = sceKernelCreateMutex(name, attr, initCount, option);

	if (mutexid >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_mutexids[i] < 0) {
				hb_mutexids[i] = mutexid;
				break;
			}
		}
	}

	return mutexid;
}

int sceKernelDeleteMutexPatchedHB(SceUID mutexid) {
	int res = sceKernelDeleteMutex(mutexid);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_UIDS; i++) {
			if (hb_mutexids[i] == mutexid) {
				hb_mutexids[i] = INVALID_UID;
				break;
			}
		}
	}

	return res;
}

PatchNID patches_init[] = {
	{ "SceAudio", 0x5BC341E4, sceAudioOutOpenPortPatchedHB },

	{ "SceGxm", 0x207AF96B, sceGxmCreateRenderTargetPatchedHB },
	{ "SceGxm", 0x4ED2E49D, sceGxmShaderPatcherCreateFragmentProgramPatchedHB },
	{ "SceGxm", 0x6A6013E1, sceGxmSyncObjectCreatePatchedHB },
	{ "SceGxm", 0xB7BBA6D5, sceGxmShaderPatcherCreateVertexProgramPatchedHB },
	{ "SceGxm", 0xE84CE5B4, sceGxmCreateContextPatchedHB },

	{ "SceIofilemgr", 0x422A221A, sceIoDclosePatchedHB },
	{ "SceIofilemgr", 0xC70B8886, sceIoClosePatchedHB },

	{ "SceLibKernel", 0x1BD67366, sceKernelCreateSemaPatchedHB },
	{ "SceLibKernel", 0x1D17DECF, sceKernelExitDeleteThreadPatchedHB },
	{ "SceLibKernel", 0x6C60AC61, sceIoOpenPatchedHB },
	{ "SceLibKernel", 0x7595D9AA, sceKernelExitProcessPatchedHB },
	{ "SceLibKernel", 0xA9283DD0, sceIoDopenPatchedHB },
	{ "SceLibKernel", 0xC5C11EE7, sceKernelCreateThreadPatchedHB },
	{ "SceLibKernel", 0xCB78710D, sceKernelDeleteMutexPatchedHB },
	{ "SceLibKernel", 0xDB32948A, sceKernelDeleteSemaPatchedHB },
	{ "SceLibKernel", 0xED53334A, sceKernelCreateMutexPatchedHB },

	{ "SceSysmem", 0xA91E15EE, sceKernelFreeMemBlockPatchedHB },
	{ "SceSysmem", 0xB9D5EBDE, sceKernelAllocMemBlockPatchedHB },
};

SceUID sceKernelCreateThreadPatchedUVL(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMask, const SceKernelThreadOptParam *option) {
	exit_thid = sceKernelCreateThread("exit_thread", (SceKernelThreadEntry)exit_thread, 0x10000100, 0x1000, 0, 0, NULL);
	if (exit_thid >= 0)
		sceKernelStartThread(exit_thid, 0, NULL);

	int i;
	for (i = 0; i < (sizeof(patches_init) / sizeof(PatchNID)); i++) {
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, hb_text_addr, patches_init[i].library, patches_init[i].nid), patches_init[i].function);
	}

	// debugPrintf("Module name: %s\n", hb_mod_info.name);

	if (strcmp(hb_mod_info.name, "VitaShell.elf") == 0) {
		_free_vita_heap();
	}

	return sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);
}

SceUID sceKernelAllocMemBlockPatchedUVL(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	// debugPrintf("%s %s: 0x%08X\n", __FUNCTION__, name, blockid);

	if (strcmp(name, "UVLTemp") == 0) {
		UVLTemp_id = blockid;
	} else if (strcmp(name, "UVLHomebrew") == 0) {
		UVLHomebrew_id = blockid;

		void *buf = NULL;
		int res = sceKernelGetMemBlockBase(UVLTemp_id, &buf);
		// debugPrintf("sceKernelGetMemBlockBase: 0x%08X\n", res);
		memcpy((void *)&hb_mod_info, (void *)getElfModuleInfo(buf), sizeof(SceModuleInfo));
	}

	return blockid;
}

int sceKernelGetMemBlockBasePatchedUVL(SceUID uid, void **basep) {
	int res = sceKernelGetMemBlockBase(uid, basep);

	if (uid != UVLTemp_id && uid != UVLHomebrew_id) {
		hb_text_addr = (uint32_t)*basep;
	}

	return res;
}

void PatchUVL() {
	uint32_t sceKernelAllocMemBlockSyscall = extractSyscallStub((uint32_t)&sceKernelAllocMemBlock);
	uint32_t sceKernelGetMemBlockBaseSyscall = extractSyscallStub((uint32_t)&sceKernelGetMemBlockBase);
	uint32_t sceKernelCreateThreadFunction = extractFunctionStub((uint32_t)&sceKernelCreateThread);

	uint32_t text_addr = ALIGN(extractFunctionStub((uint32_t)&uvl_load), 0x100000) - 0x100000;

	int count = 0;

	uint32_t i = 0;
	while (i < 0x10000 && count < 3) {
		uint32_t addr = text_addr + i;
		uint32_t syscall = extractSyscallStub(addr);
		uint32_t function = extractFunctionStub(addr);

		if (syscall == sceKernelAllocMemBlockSyscall) {
			makeFunctionStub(addr, sceKernelAllocMemBlockPatchedUVL);
			count++;
		} else if (syscall == sceKernelGetMemBlockBaseSyscall) {
			makeFunctionStub(addr, sceKernelGetMemBlockBasePatchedUVL);
			count++;
		} else if (function == sceKernelCreateThreadFunction) {
			makeFunctionStub(addr, sceKernelCreateThreadPatchedUVL);
			count++;
		}

		if (syscall != -1 || function != -1) {
			i += 0x10;
		} else {
			i += 4;
		}
	}
}