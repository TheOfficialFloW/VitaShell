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
#include "homebrew.h"
#include "file.h"
#include "utils.h"
#include "module.h"
#include "misc.h"
#include "elf_types.h"

// sceKernelGetThreadmgrUIDClass is not available
// suspend/resume for vm returns 0x80020008

// sceKernelDeleteMutex not resolved by UVL

/*
	TODO:
	- Unload prxs
	- Delete semaphores, events, callbacks, msg pipes, rw locks
*/

static void *uvl_backup = NULL;
static uint32_t uvl_addr = 0;

static int force_exit = 0;

static SceUID exit_thid = INVALID_UID;

static SceUID UVLTemp_id = INVALID_UID, UVLHomebrew_id = INVALID_UID;
static uint32_t hb_text_addr = 0;
static SceModuleInfo hb_mod_info;

static SceUID hb_fds[MAX_UIDS];
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
		if (hb_fds[i] >= 0) {
			if (sceIoClose(hb_fds[i]) < 0)
				sceIoDclose(hb_fds[i]);
		}
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
	//{ "SceAudio", 0x02DB3F5F, exitThread }, // sceAudioOutOutput // crashes mGBA

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
	scePowerSetBusClockFrequency(166); // that's actually gpu frequency
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
	// Init libc
	_init_vita_newlib();

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

int sceAudioOutOutputPatchedHB(int port, const void *buf) {
	int res = sceAudioOutOutput(port, buf);

	if (force_exit)
		exitThread();

	return res;
}

int sceAudioOutOpenPortPatchedHB(int type, int len, int freq, int mode) {
	int port = sceAudioOutOpenPort(type, len, freq, mode);

	if (type < MAX_AUDIO_PORTS)
		hb_audio_ports[type] = port;

	return port;
}

int sceGxmCreateContextPatchedHB(const SceGxmContextParams *params, SceGxmContext **context) {
	int res = sceGxmCreateContext(params, context);

	if (res >= 0) {
		hb_gxm_context = *context;
	}

	return res;
}

int sceGxmCreateRenderTargetPatchedHB(const SceGxmRenderTargetParams *params, SceGxmRenderTarget **renderTarget) {
	int res = sceGxmCreateRenderTarget(params, renderTarget);

	if (res >= 0) {
		hb_gxm_render = *renderTarget;
	}

	return res;
}

int sceGxmShaderPatcherCreateVertexProgramPatchedHB(SceGxmShaderPatcher *shaderPatcher, SceGxmShaderPatcherId programId, const SceGxmVertexAttribute *attributes, unsigned int attributeCount, const SceGxmVertexStream *streams, unsigned int streamCount, SceGxmVertexProgram **vertexProgram) {
	int res = sceGxmShaderPatcherCreateVertexProgram(shaderPatcher, programId, attributes, attributeCount, streams, streamCount, vertexProgram);

	// debugPrintf("%s 0x%08X 0x%08X\n", __FUNCTION__, programId, *vertexProgram);

	if (hb_shader_patcher == NULL)
		hb_shader_patcher = shaderPatcher;

	if (res >= 0) {
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
	}

	return res;
}

int sceGxmShaderPatcherCreateFragmentProgramPatchedHB(SceGxmShaderPatcher *shaderPatcher, SceGxmShaderPatcherId programId, SceGxmOutputRegisterFormat outputFormat, SceGxmMultisampleMode multisampleMode, const SceGxmBlendInfo *blendInfo, const SceGxmProgram *vertexProgram, SceGxmFragmentProgram **fragmentProgram) {
	int res = sceGxmShaderPatcherCreateFragmentProgram(shaderPatcher, programId, outputFormat, multisampleMode, blendInfo, vertexProgram, fragmentProgram);

	// debugPrintf("%s 0x%08X 0x%08X\n", __FUNCTION__, programId, *fragmentProgram);

	if (hb_shader_patcher == NULL)
		hb_shader_patcher = shaderPatcher;

	if (res >= 0) {
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
	}

	return res;
}

int sceGxmSyncObjectCreatePatchedHB(SceGxmSyncObject **syncObject) {
	int res = sceGxmSyncObjectCreate(syncObject);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_SYNC_OBJECTS; i++) {
			if (hb_sync_objects[i] == NULL) {
				// debugPrintf("%s 0x%08X %d\n", __FUNCTION__, *syncObject, i);
				hb_sync_objects[i] = *syncObject;
				break;
			}
		}
	}

	return res;
}

int sceKernelExitDeleteThreadPatchedHB(int status) {
	DELETE_UID(hb_thids, sceKernelGetThreadId());
	return sceKernelExitDeleteThread(status);
}

SceUID sceKernelCreateThreadPatchedHB(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMask, const SceKernelThreadOptParam *option) {
	SceUID thid = sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);

	// debugPrintf("%s 0x%08X\n", name, thid);

	if (thid >= 0) {
		INSERT_UID(hb_thids, thid);
	}

	return thid;
}

SceUID sceKernelAllocMemBlockPatchedHB(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	if (blockid >= 0) {
		INSERT_UID(hb_blockids, blockid);
	}

	return blockid;
}

int sceKernelFreeMemBlockPatchedHB(SceUID uid) {
	int res = sceKernelFreeMemBlock(uid);

	if (res >= 0) {
		DELETE_UID(hb_blockids, uid);
	}

	return res;
}

SceUID sceIoOpenPatchedHB(const char *file, int flags, SceMode mode) {
	SceUID fd = sceIoOpen(file, flags, mode);

	if (fd >= 0) {
		INSERT_UID(hb_fds, fd);
	}

	return fd;
}

int sceIoClosePatchedHB(SceUID fd) {
	int res = sceIoClose(fd);

	if (res >= 0) {
		DELETE_UID(hb_fds, fd);
	}

	return res;
}

SceUID sceIoDopenPatchedHB(const char *dirname) {
	SceUID fd = sceIoDopen(dirname);

	if (fd >= 0) {
		INSERT_UID(hb_fds, fd);
	}

	return fd;
}

int sceIoDclosePatchedHB(SceUID fd) {
	int res = sceIoDclose(fd);

	if (res >= 0) {
		DELETE_UID(hb_fds, fd);
	}

	return res;
}

SceUID sceKernelCreateSemaPatchedHB(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option) {
	SceUID semaid = sceKernelCreateSema(name, attr, initVal, maxVal, option);

	if (semaid >= 0) {
		INSERT_UID(hb_semaids, semaid);
	}

	return semaid;
}

int sceKernelDeleteSemaPatchedHB(SceUID semaid) {
	int res = sceKernelDeleteSema(semaid);

	if (res >= 0) {
		DELETE_UID(hb_semaids, semaid);
	}

	return res;
}

SceUID sceKernelCreateMutexPatchedHB(const char *name, SceUInt attr, int initCount, SceKernelMutexOptParam *option) {
	SceUID mutexid = sceKernelCreateMutex(name, attr, initCount, option);

	if (mutexid >= 0) {
		INSERT_UID(hb_mutexids, mutexid);
	}

	return mutexid;
}

int sceKernelDeleteMutexPatchedHB(SceUID mutexid) {
	int res = sceKernelDeleteMutex(mutexid);

	if (res >= 0) {
		DELETE_UID(hb_mutexids, mutexid);
	}

	return res;
}

PatchNID patches_init[] = {
	{ "SceAudio", 0x02DB3F5F, sceAudioOutOutputPatchedHB },
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
	// debugPrintf("Module name: %s\n", hb_mod_info.name);

	if (strcmp(hb_mod_info.name, "VitaShell.elf") == 0) {
		// Pass address of the current code memblock to sceKernelExitProcess stub
		SceKernelMemBlockInfo info;
		findMemBlockByAddr((uint32_t)&sceKernelCreateThreadPatchedUVL, &info);
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, hb_text_addr, "SceLibKernel", 0x7595D9AA), info.mappedBase);

		restoreUVL();
		_free_vita_newlib();
	} else {
		exit_thid = sceKernelCreateThread("exit_thread", (SceKernelThreadEntry)exit_thread, 0x10000100, 0x1000, 0, 0, NULL);
		if (exit_thid >= 0)
			sceKernelStartThread(exit_thid, 0, NULL);

		int i;
		for (i = 0; i < (sizeof(patches_init) / sizeof(PatchNID)); i++) {
			makeFunctionStub(findModuleImportByInfo(&hb_mod_info, hb_text_addr, patches_init[i].library, patches_init[i].nid), patches_init[i].function);
		}
	}

	return sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);
}

SceUID sceKernelAllocMemBlockPatchedUVL(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	debugPrintf("%s %s: 0x%08X\n", __FUNCTION__, name, blockid);

	if (strcmp(name, "UVLTemp") == 0) {
		UVLTemp_id = blockid;
	} else if (strcmp(name, "UVLHomebrew") == 0) {
		UVLHomebrew_id = blockid;

		void *buf = NULL;
		int res = sceKernelGetMemBlockBase(UVLTemp_id, &buf);
		debugPrintf("sceKernelGetMemBlockBase: 0x%08X\n", res);
		if (res < 0)
			return res;

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

SceUID sceIoOpenPatchedUVL(const char *file, int flags, SceMode mode) {
	debugPrintf("%s\n", __FUNCTION__);
	return sceIoOpen(file, flags, mode);
}

SceOff sceIoLseekPatchedUVL(SceUID fd, SceOff offset, int whence) {
	debugPrintf("%s\n", __FUNCTION__);
	return sceIoLseek(fd, offset, whence);
}

int sceIoReadPatchedUVL(SceUID fd, void *data, SceSize size) {
	debugPrintf("%s\n", __FUNCTION__);
	return sceIoRead(fd, data, size);
}

int sceIoWritePatchedUVL(SceUID fd, const void *data, SceSize size) {
	debugPrintf("%s\n", __FUNCTION__);
	return sceIoWrite(fd, data, size);
}

int sceIoClosePatchedUVL(SceUID fd) {
	debugPrintf("%s\n", __FUNCTION__);
	return sceIoClose(fd);
}

PatchValue patches_uvl[] = {
	{ -1, (uint32_t)&sceKernelAllocMemBlock, sceKernelAllocMemBlockPatchedUVL },
	{ -1, (uint32_t)&sceKernelGetMemBlockBase, sceKernelGetMemBlockBasePatchedUVL },
	{ -1, (uint32_t)&sceKernelCreateThread, sceKernelCreateThreadPatchedUVL },
	{ -1, (uint32_t)&sceIoOpen, sceIoOpenPatchedUVL },
	{ -1, (uint32_t)&sceIoLseek, sceIoLseekPatchedUVL },
	{ -1, (uint32_t)&sceIoRead, sceIoReadPatchedUVL },
	{ -1, (uint32_t)&sceIoWrite, sceIoWritePatchedUVL },
	{ -1, (uint32_t)&sceIoClose, sceIoClosePatchedUVL },
};

#define N_UVL_PATCHES (sizeof(patches_uvl) / sizeof(PatchValue))

void initPatchValues(PatchValue *patches, int n_patches) {
	int i;
	for (i = 0; i < n_patches; i++) {
		patches[i].value = extractStub(patches[i].stub);
	}
}

void PatchUVL() {
	if (!uvl_backup)
		return;

	initPatchValues(patches_uvl, N_UVL_PATCHES);

	int count = 0;

	uint32_t i = 0;
	while (i < UVL_SIZE && count < N_UVL_PATCHES) {
		uint32_t addr = uvl_addr + i;
		uint32_t value = extractStub(addr);

		int j;
		for (j = 0; j < N_UVL_PATCHES; j++) {
			if (patches_uvl[j].value == value) {
				makeFunctionStub(addr, patches_uvl[j].function);
				count++;
				break;
			}
		}

		i += ((j == N_UVL_PATCHES) ? 0x4 : 0x10);
	}
}

void getUVLTextAddr() {
	SceKernelMemBlockInfo info;
	findMemBlockByAddr(extractFunctionStub((uint32_t)&uvl_load), &info);
	uvl_addr = (uint32_t)info.mappedBase;
}

void backupUVL() {
	if (!uvl_backup) {
		uvl_backup = malloc(UVL_SIZE);
		memcpy(uvl_backup, (void *)uvl_addr, UVL_SIZE);
	}
}

void restoreUVL() {
	if (uvl_backup) {
		uvl_unlock_mem();

		memcpy((void *)uvl_addr, uvl_backup, UVL_SIZE);

		uvl_lock_mem();
		uvl_flush_icache((void *)uvl_addr, UVL_SIZE);

		free(uvl_backup);
		uvl_backup = NULL;
	}
}
