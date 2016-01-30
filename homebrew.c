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
#include "homebrew.h"
#include "file.h"
#include "utils.h"
#include "module.h"
#include "misc.h"
#include "elf_types.h"

// sceKernelGetThreadmgrUIDClass is not available
// suspend/resume for vm returns 0x80020008

/*
	TODO:
	- Unload prxs
	- Delete events, callbacks, msg pipes, rw locks
*/

static void *code_memory = NULL;
static SceUID code_blockid = INVALID_UID;

static int force_exit = 0;
static int exited = 0;

static SceUID exit_thid = INVALID_UID;

static SceUID UVLTemp_id = INVALID_UID;
static SceModuleInfo hb_mod_info;

static SceUID hb_fds[MAX_UIDS];
static SceUID hb_thids[MAX_UIDS];
static SceUID hb_semaids[MAX_UIDS];
static SceUID hb_mutexids[MAX_UIDS];
static SceUID hb_blockids[MAX_UIDS];
static SceUID hb_sockids[MAX_UIDS];

void *hb_lw_mutex_works[MAX_LW_MUTEXES];

static int hb_audio_ports[MAX_AUDIO_PORTS];

static int hb_gxm_initialized = 0;
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

void initCodeMemory(SceUID blockid) {
	if (blockid >= 0) {
		code_blockid = blockid;
		sceKernelGetMemBlockBase(code_blockid, &code_memory);
	} else {
		unsigned int length = MAX_CODE_SIZE;
		code_memory = uvl_alloc_code_mem(&length);
		code_blockid = sceKernelFindMemBlockByAddr(code_memory, 0);
	}
}

void initHomebrewPatch() {
	force_exit = 0;
	exited = 0;

	exit_thid = INVALID_UID;

	UVLTemp_id = INVALID_UID;
	memset((void *)&hb_mod_info, 0, sizeof(SceModuleInfo));

	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		hb_fds[i] = INVALID_UID;
		hb_thids[i] = INVALID_UID;
		hb_semaids[i] = INVALID_UID;
		hb_mutexids[i] = INVALID_UID;
		hb_blockids[i] = INVALID_UID;
		hb_sockids[i] = INVALID_UID;
	}

	memset(hb_lw_mutex_works, 0, sizeof(hb_lw_mutex_works));

	memset(hb_audio_ports, 0, sizeof(hb_audio_ports));

	hb_gxm_initialized = 0;
	hb_gxm_context = NULL;
	hb_gxm_render = NULL;
	hb_shader_patcher = NULL;

	memset(hb_sync_objects, 0, sizeof(hb_sync_objects));
	memset(hb_fragment_programs, 0, sizeof(hb_fragment_programs));
	memset(hb_fragment_program_ids, 0, sizeof(hb_fragment_program_ids));
	memset(hb_vertex_programs, 0, sizeof(hb_vertex_programs));
	memset(hb_vertex_program_ids, 0, sizeof(hb_vertex_program_ids));
}

void loadHomebrew(char *file) {
	// Finish vita2dlib
	finishVita2dLib();

	// Init
	initHomebrewPatch();

	// Load
	uvl_load(file);

	// Init vita2lib
	initVita2dLib();
}

int finishGxm() {
	if (!hb_gxm_initialized)
		return 0;

	int i;

	// Wait until rendering is done
	sceGxmFinish(hb_gxm_context);

	// Clean up allocations
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_programs[i]) {
			if (sceGxmShaderPatcherReleaseFragmentProgram(hb_shader_patcher, hb_fragment_programs[i]) >= 0)
				hb_fragment_programs[i] = NULL;
		}

		if (hb_vertex_programs[i]) {
			if (sceGxmShaderPatcherReleaseVertexProgram(hb_shader_patcher, hb_vertex_programs[i]) >= 0)
				hb_vertex_programs[i] = NULL;
		}
	}

	// Wait until display queue is finished before deallocating display buffers
	sceGxmDisplayQueueFinish();

	// Clean up display queue
	for (i = 0; i < MAX_SYNC_OBJECTS; i++) {
		// Destroy the sync object
		if (hb_sync_objects[i]) {
			if (sceGxmSyncObjectDestroy(hb_sync_objects[i]) >= 0)
				hb_sync_objects[i] = NULL;
		}
	}

	// Unregister programs
	for (i = 0; i < MAX_GXM_PRGRAMS; i++) {
		if (hb_fragment_program_ids[i]) {
			if (sceGxmShaderPatcherUnregisterProgram(hb_shader_patcher, hb_fragment_program_ids[i]) >= 0)
				hb_fragment_program_ids[i] = NULL;
		}

		if (hb_vertex_program_ids[i]) {
			if (sceGxmShaderPatcherUnregisterProgram(hb_shader_patcher, hb_vertex_program_ids[i]) >= 0)
				hb_vertex_program_ids[i] = NULL;
		}
	}

	// Destroy shader patchers
	if (sceGxmShaderPatcherDestroy(hb_shader_patcher) >= 0)
		hb_shader_patcher = NULL;

	// Destroy the render target
	if (sceGxmDestroyRenderTarget(hb_gxm_render) >= 0)
		hb_gxm_render = NULL;

	// Destroy the context
	if (sceGxmDestroyContext(hb_gxm_context) >= 0)
		hb_gxm_context = NULL;

	// Terminate libgxm
	sceGxmTerminate();

	return 1;
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
			int res = sceIoClose(hb_fds[i]);
			if (res < 0)
				res = sceIoDclose(hb_fds[i]);

			if (res >= 0)
				hb_fds[i] = INVALID_UID;
		}
	}
}

void deleteSemas() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_semaids[i] >= 0) {
			if (sceKernelDeleteSema(hb_semaids[i]) >= 0)
				hb_semaids[i] = INVALID_UID;
		}
	}
}

void deleteMutexes() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_mutexids[i] >= 0) {
			if (sceKernelDeleteMutex(hb_mutexids[i]) >= 0)
				hb_mutexids[i] = INVALID_UID;
		}
	}
}

void deleteLwMutexes() {
	int i;
	for (i = 0; i < MAX_LW_MUTEXES; i++) {
		if (hb_lw_mutex_works[i]) {
			if (sceKernelDeleteLwMutex(hb_lw_mutex_works[i]) >= 0)
				hb_lw_mutex_works[i] = NULL;
		}
	}
}

void closeSockets() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_sockids[i] >= 0) {
			if (sceNetSocketClose(hb_sockids[i]) >= 0)
				hb_sockids[i] = INVALID_UID;
		}
	}
}

void freeMemBlocks() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_blockids[i] < 0)
			continue;

		int res = 0;

		void *mem = NULL;
		if (sceKernelGetMemBlockBase(hb_blockids[i], &mem) < 0)
			continue;

		res = sceGxmUnmapMemory(mem);
		if (res < 0)
			sceGxmUnmapVertexUsseMemory(mem);
		if (res < 0)
			sceGxmUnmapFragmentUsseMemory(mem);

		res = sceKernelFreeMemBlock(hb_blockids[i]);
		if (res >= 0)
			hb_blockids[i] = INVALID_UID;
	}
}

void homebrewCleanUp() {
	// Wait for exit thread to end
	exited = 1;
	sceKernelWaitThreadEnd(exit_thid, NULL, NULL);

	// Clean up
	finishGxm();
	freeMemBlocks();
	deleteSemas();
	deleteMutexes();
	deleteLwMutexes();
	closeFileDescriptors();
	closeSockets();
	releaseAudioPorts();
}

void signalSemas() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_semaids[i] >= 0)
			sceKernelSignalSema(hb_semaids[i], 1);
	}
}

void unlockMutexes() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_mutexids[i] >= 0)
			sceKernelUnlockMutex(hb_mutexids[i], 1);
	}
}

void unlockLwMutexes() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_lw_mutex_works[i])
			sceKernelUnlockLwMutex(hb_lw_mutex_works[i], 1);
	}
}

void waitThreadEnd() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_thids[i] >= 0 && hb_thids[i] != sceKernelGetThreadId()) {
			if (sceKernelWaitThreadEnd(hb_thids[i], NULL, NULL) >= 0)
				hb_thids[i] = INVALID_UID;
		}
	}
}

int exitThread() {
	// Signal semaphores, unlock mutexes, unlock lw mutexes
	signalSemas();
	unlockMutexes();
	unlockLwMutexes();

	// Wait for all other threads to end
	waitThreadEnd();

	return sceKernelExitDeleteThread(0);
}

PatchNID patches_exit[] = {
	//{ "SceAudio", 0x02DB3F5F, exitThread }, // sceAudioOutOutput // crashes mGBA

	{ "SceCtrl", 0x104ED1A7, exitThread }, // sceCtrlPeekBufferNegative
	{ "SceCtrl", 0x15F96FB0, exitThread }, // sceCtrlReadBufferNegative
	{ "SceCtrl", 0x67E7AB83, exitThread }, // sceCtrlReadBufferPositive
	{ "SceCtrl", 0xA9C3CED6, exitThread }, // sceCtrlPeekBufferPositive

	//{ "SceDisplay", 0x5795E898, exitThread }, // sceDisplayWaitVblankStart

	{ "SceGxm", 0x8734FF4E, exitThread }, // sceGxmBeginScene

	{ "SceLibKernel", 0x0C7B834B, exitThread }, // sceKernelWaitSema
	{ "SceLibKernel", 0x174692B4, exitThread }, // sceKernelWaitSemaCB
	{ "SceLibKernel", 0x1D8D7945, exitThread }, // sceKernelLockMutex
	{ "SceLibKernel", 0x72FC1F54, exitThread }, // sceKernelTryLockMutex
	{ "SceLibKernel", 0xE6B761D1, exitThread }, // sceKernelSignalSema

	{ "SceNet", 0x023643B7, exitThread }, // sceNetRecv
	{ "SceNet", 0x1ADF9BB1, exitThread }, // sceNetAccept

	{ "SceThreadmgr", 0x1A372EC8, exitThread }, // sceKernelUnlockMutex
	//{ "SceThreadmgr", 0x4B675D05, exitThread }, // sceKernelDelayThread
};

void PatchHomebrew() {
	int i;
	for (i = 0; i < (sizeof(patches_exit) / sizeof(PatchNID)); i++) {
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, (uint32_t)code_memory, patches_exit[i].library, patches_exit[i].nid), patches_exit[i].function);
	}

	force_exit = 1;
}

int exit_thread(SceSize args, void *argp) {
	while (!exited) {
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

int sceKernelExitProcessPatchedHB(int res) {
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

int sceGxmInitializePatchedHB(const SceGxmInitializeParams *params) {
	int res = sceGxmInitialize(params);

	if (res >= 0)
		hb_gxm_initialized = 1;

	return res;
}

int sceGxmTerminatePatchedHB() {
	int res = sceGxmTerminate();

	if (res >= 0)
		hb_gxm_initialized = 0;

	return res;
}

int sceGxmCreateContextPatchedHB(const SceGxmContextParams *params, SceGxmContext **context) {
	int res = sceGxmCreateContext(params, context);

	if (res >= 0)
		hb_gxm_context = *context;

	return res;
}

int sceGxmCreateRenderTargetPatchedHB(const SceGxmRenderTargetParams *params, SceGxmRenderTarget **renderTarget) {
	int res = sceGxmCreateRenderTarget(params, renderTarget);

	if (res >= 0)
		hb_gxm_render = *renderTarget;

	return res;
}

int sceGxmShaderPatcherCreateVertexProgramPatchedHB(SceGxmShaderPatcher *shaderPatcher, SceGxmShaderPatcherId programId, const SceGxmVertexAttribute *attributes, unsigned int attributeCount, const SceGxmVertexStream *streams, unsigned int streamCount, SceGxmVertexProgram **vertexProgram) {
	int res = sceGxmShaderPatcherCreateVertexProgram(shaderPatcher, programId, attributes, attributeCount, streams, streamCount, vertexProgram);

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

	if (thid >= 0)
		INSERT_UID(hb_thids, thid);

	return thid;
}

SceUID sceKernelAllocMemBlockPatchedHB(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	if (blockid >= 0)
		INSERT_UID(hb_blockids, blockid);

	return blockid;
}

int sceKernelFreeMemBlockPatchedHB(SceUID uid) {
	int res = sceKernelFreeMemBlock(uid);

	if (res >= 0)
		DELETE_UID(hb_blockids, uid);

	return res;
}

SceUID sceIoOpenPatchedHB(const char *file, int flags, SceMode mode) {
	SceUID fd = sceIoOpen(file, flags, mode);

	if (fd >= 0)
		INSERT_UID(hb_fds, fd);

	return fd;
}

int sceIoClosePatchedHB(SceUID fd) {
	int res = sceIoClose(fd);

	if (res >= 0)
		DELETE_UID(hb_fds, fd);

	return res;
}

SceUID sceIoDopenPatchedHB(const char *dirname) {
	SceUID fd = sceIoDopen(dirname);

	if (fd >= 0)
		INSERT_UID(hb_fds, fd);

	return fd;
}

int sceIoDclosePatchedHB(SceUID fd) {
	int res = sceIoDclose(fd);

	if (res >= 0)
		DELETE_UID(hb_fds, fd);

	return res;
}

int sceNetSocketPatchedHB(const char *name, int domain, int type, int protocol) {
	SceUID sockid = sceNetSocket(name, domain, type, protocol);

	if (sockid >= 0)
		INSERT_UID(hb_sockids, sockid);

	return sockid;
}

int sceNetSocketClosePatchedHB(int s) {
	int res = sceNetSocketClose(s);

	if (res >= 0)
		DELETE_UID(hb_sockids, s);

	return res;
}

SceUID sceKernelCreateSemaPatchedHB(const char *name, SceUInt attr, int initVal, int maxVal, SceKernelSemaOptParam *option) {
	SceUID semaid = sceKernelCreateSema(name, attr, initVal, maxVal, option);

	if (semaid >= 0)
		INSERT_UID(hb_semaids, semaid);

	return semaid;
}

int sceKernelDeleteSemaPatchedHB(SceUID semaid) {
	int res = sceKernelDeleteSema(semaid);

	if (res >= 0)
		DELETE_UID(hb_semaids, semaid);

	return res;
}

SceUID sceKernelCreateMutexPatchedHB(const char *name, SceUInt attr, int initCount, SceKernelMutexOptParam *option) {
	SceUID mutexid = sceKernelCreateMutex(name, attr, initCount, option);

	if (mutexid >= 0)
		INSERT_UID(hb_mutexids, mutexid);

	return mutexid;
}

int sceKernelDeleteMutexPatchedHB(SceUID mutexid) {
	int res = sceKernelDeleteMutex(mutexid);

	if (res >= 0)
		DELETE_UID(hb_mutexids, mutexid);

	return res;
}

int sceKernelCreateLwMutexPatchedHB(void *work, const char *name, SceUInt attr, int initCount, void *option) {
	int res = sceKernelCreateLwMutex(work, name, attr, initCount, option);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_LW_MUTEXES; i++) {
			if (hb_lw_mutex_works[i] == NULL) {
				hb_lw_mutex_works[i] = work;
				break;
			}
		}
	}

	return res;
}

int sceKernelDeleteLwMutexPatchedHB(void *work) {
	int res = sceKernelDeleteLwMutex(work);

	if (res >= 0) {
		int i;
		for (i = 0; i < MAX_LW_MUTEXES; i++) {
			if (hb_lw_mutex_works[i] == work) {
				hb_lw_mutex_works[i] = NULL;
				break;
			}
		}
	}

	return res;
}

PatchNID patches_init[] = {
	{ "SceAudio", 0x02DB3F5F, sceAudioOutOutputPatchedHB },
	{ "SceAudio", 0x5BC341E4, sceAudioOutOpenPortPatchedHB },

	{ "SceGxm", 0x207AF96B, sceGxmCreateRenderTargetPatchedHB },
	{ "SceGxm", 0x4ED2E49D, sceGxmShaderPatcherCreateFragmentProgramPatchedHB },
	{ "SceGxm", 0x6A6013E1, sceGxmSyncObjectCreatePatchedHB },	
	{ "SceGxm", 0xB0F1E4EC, sceGxmInitializePatchedHB },
	{ "SceGxm", 0xB627DE66, sceGxmTerminatePatchedHB },
	{ "SceGxm", 0xB7BBA6D5, sceGxmShaderPatcherCreateVertexProgramPatchedHB },
	{ "SceGxm", 0xE84CE5B4, sceGxmCreateContextPatchedHB },

	{ "SceIofilemgr", 0x422A221A, sceIoDclosePatchedHB },
	{ "SceIofilemgr", 0xC70B8886, sceIoClosePatchedHB },

	{ "SceLibKernel", 0x1BD67366, sceKernelCreateSemaPatchedHB },
	{ "SceLibKernel", 0x1D17DECF, sceKernelExitDeleteThreadPatchedHB },
	{ "SceLibKernel", 0x244E76D2, sceKernelDeleteLwMutexPatchedHB },
	{ "SceLibKernel", 0x6C60AC61, sceIoOpenPatchedHB },
	{ "SceLibKernel", 0x7595D9AA, sceKernelExitProcessPatchedHB },
	{ "SceLibKernel", 0xA9283DD0, sceIoDopenPatchedHB },
	{ "SceLibKernel", 0xC5C11EE7, sceKernelCreateThreadPatchedHB },
	{ "SceLibKernel", 0xCB78710D, sceKernelDeleteMutexPatchedHB },
	{ "SceLibKernel", 0xDA6EC8EF, sceKernelCreateLwMutexPatchedHB },
	{ "SceLibKernel", 0xDB32948A, sceKernelDeleteSemaPatchedHB },
	{ "SceLibKernel", 0xED53334A, sceKernelCreateMutexPatchedHB },

	{ "SceNet", 0x29822B4D, sceNetSocketClosePatchedHB },
	{ "SceNet", 0xF084FCE3, sceNetSocketPatchedHB },

	{ "SceSysmem", 0xA91E15EE, sceKernelFreeMemBlockPatchedHB },
	{ "SceSysmem", 0xB9D5EBDE, sceKernelAllocMemBlockPatchedHB },
};

SceUID sceKernelCreateThreadPatchedUVL(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMask, const SceKernelThreadOptParam *option) {
	exit_thid = sceKernelCreateThread("exit_thread", (SceKernelThreadEntry)exit_thread, 191, 0x1000, 0, 0, NULL);
	if (exit_thid >= 0)
		sceKernelStartThread(exit_thid, 0, NULL);

	int i;
	for (i = 0; i < (sizeof(patches_init) / sizeof(PatchNID)); i++) {
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, (uint32_t)code_memory, patches_init[i].library, patches_init[i].nid), patches_init[i].function);
	}

	return sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);
}

int sceKernelWaitThreadEndPatchedUVL(SceUID thid, int *stat, SceUInt *timeout) {
	int res = sceKernelWaitThreadEnd(thid, stat, timeout);

	// The homebrew has been terminated, now clean up all resources
	homebrewCleanUp();

	return res;
}

SceUID sceKernelAllocMemBlockPatchedUVL(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	// UVLTemp buffer contains the elf data, get its blockid
	if (strcmp(name, "UVLTemp") == 0) {
		UVLTemp_id = blockid;
	}

	return blockid;
}

SceUID sceKernelFindMemBlockByAddrPatchedUVL(const void *addr, SceSize size) {
	// uvl_alloc_code_mem is patched to always return NULL
	if (addr == NULL) {
		// Now it is the moment we need to know the SceModuleInfo of this elf
		// to decide whether to return the blockid of VitaShell or the blockid of code memory
		void *buf = NULL;
		int res = sceKernelGetMemBlockBase(UVLTemp_id, &buf);
		if (res < 0)
			return res;

		/* Save module info */
		memcpy((void *)&hb_mod_info, (void *)getElfModuleInfo(buf), sizeof(SceModuleInfo));

		// It's not VitaShell, so return code memory blockid
		if (strcmp(hb_mod_info.name, "VitaShell.elf") != 0) {
			return code_blockid;
		}

		// Restore IO patches
		restoreIOPatches();

		// Restore UVL patches
		restoreUVLPatches();

		// Finish VitaShell
		finishVitaShell();

		// Free newlib
		_free_vita_newlib();

		// Adjust shared memory
		shared_memory->code_blockid = code_blockid;
		shared_memory->data_blockid = sceKernelFindMemBlockByAddr((void *)&code_memory, 0);

		// Shall I redirect data block too? Nah, I guess it's not neccessary
	//	makeResultStub(shared_memory->sceKernelAllocMemBlockAddr, shared_block->data_blockid);
/*
		// Replace sceKernelWaitThreadEnd by sceKernelExitDeleteThread
		if (shared_memory->reload_count > 0)
			makeStub(shared_memory->sceKernelWaitThreadEndAddr, (void *)extractStub((uint32_t)&sceKernelExitDeleteThread));

		// Count reload
		shared_memory->reload_count++;
*/
		// Change address to our's
		addr = (void *)&sceKernelFindMemBlockByAddrPatchedUVL;
	}

	return sceKernelFindMemBlockByAddr(addr, size);
}

int sceKernelFreeMemBlockPatchedUVL(SceUID uid) {
	// Never free the code memory
	if (uid == code_blockid)
		return 0;

	return sceKernelFreeMemBlock(uid);
}

SceUID sceIoOpenPatchedUVL(const char *file, int flags, SceMode mode) {
	return sceIoOpen(file, flags, mode);
}

SceOff sceIoLseekPatchedUVL(SceUID fd, SceOff offset, int whence) {
	return sceIoLseek(fd, offset, whence);
}

int sceIoReadPatchedUVL(SceUID fd, void *data, SceSize size) {
	return sceIoRead(fd, data, size);
}

int sceIoWritePatchedUVL(SceUID fd, const void *data, SceSize size) {
	// Redirect to our debug logging and close the uvloader.log descriptor
	debugPrintf((char *)data);
	return sceIoClose(fd);
	//return sceIoWrite(fd, data, size);
}

int sceIoClosePatchedUVL(SceUID fd) {
	return sceIoClose(fd);
}

PatchValue patches_uvl[] = {
	{ 0, 0, (uint32_t)&sceKernelAllocMemBlock, sceKernelAllocMemBlockPatchedUVL },
	{ 0, 0, (uint32_t)&sceKernelFindMemBlockByAddr, sceKernelFindMemBlockByAddrPatchedUVL },
	{ 0, 0, (uint32_t)&sceKernelFreeMemBlock, sceKernelFreeMemBlockPatchedUVL },
	{ 0, 0, (uint32_t)&sceKernelCreateThread, sceKernelCreateThreadPatchedUVL },
	{ 0, 0, (uint32_t)&sceKernelWaitThreadEnd, sceKernelWaitThreadEndPatchedUVL },
	{ 0, 0, (uint32_t)&sceIoOpen, sceIoOpenPatchedUVL },
	{ 0, 0, (uint32_t)&sceIoLseek, sceIoLseekPatchedUVL },
	{ 0, 0, (uint32_t)&sceIoRead, sceIoReadPatchedUVL },
	{ 0, 0, (uint32_t)&sceIoWrite, sceIoWritePatchedUVL },
	{ 0, 0, (uint32_t)&sceIoClose, sceIoClosePatchedUVL },
};

#define N_UVL_PATCHES (sizeof(patches_uvl) / sizeof(PatchValue))

void savePatchAddresses() {
	if (!shared_memory->uvl_addresses_saved) {
		shared_memory->sceKernelAllocMemBlockAddr = patches_uvl[0].addr;
		shared_memory->sceKernelFindMemBlockByAddrAddr = patches_uvl[1].addr;
		shared_memory->sceKernelFreeMemBlockAddr = patches_uvl[2].addr;
		shared_memory->sceKernelCreateThreadAddr = patches_uvl[3].addr;
		shared_memory->sceKernelWaitThreadEndAddr = patches_uvl[4].addr;
		shared_memory->sceIoOpenAddr = patches_uvl[5].addr;
		shared_memory->sceIoLseekAddr = patches_uvl[6].addr;
		shared_memory->sceIoReadAddr = patches_uvl[7].addr;
		shared_memory->sceIoWriteAddr = patches_uvl[8].addr;
		shared_memory->sceIoCloseAddr = patches_uvl[9].addr;
		shared_memory->uvl_addresses_saved = 1;
	}
}

void initPatchValues(PatchValue *patches, int n_patches) {
	int i;
	for (i = 0; i < n_patches; i++) {
		patches[i].value = extractStub(patches[i].stub);
		
		// These stubs call imports instead of syscalls/exports
		if (i >= 7 && i <= 9) {
			patches[i].value = findModuleImportByValue("SceLibKernel", "SceIofilemgr", patches[i].value);
		}
	}
}

int PatchUVL() {
	// Find out patch addresses
	if (!shared_memory->uvl_addresses_saved) {
		uint32_t text_addr = 0;
		SceUID blockid = sceKernelFindMemBlockByAddr((void *)extractFunctionStub((uint32_t)&uvl_load), 0);
		if (blockid < 0)
			return blockid;

		int res = sceKernelGetMemBlockBase(blockid, (void *)&text_addr);
		if (res < 0)
			return res;

		initPatchValues(patches_uvl, N_UVL_PATCHES);

		int count = 0;

		uint32_t i = 0;
		while (i < MAX_UVL_SIZE && count < N_UVL_PATCHES) {
			uint32_t addr = text_addr + i;
			uint32_t value = extractStub(addr);

			int j;
			for (j = 0; j < N_UVL_PATCHES; j++) {
				if (patches_uvl[j].value == value) {
					patches_uvl[j].addr = addr;
					count++;
					break;
				}
			}

			i += ((j == N_UVL_PATCHES) ? 0x4 : 0x10);
		}

		// Save the addresses
		savePatchAddresses();
	}

	// Patch stubs
	makeFunctionStub(shared_memory->sceKernelAllocMemBlockAddr, sceKernelAllocMemBlockPatchedUVL);
	makeFunctionStub(shared_memory->sceKernelFindMemBlockByAddrAddr, sceKernelFindMemBlockByAddrPatchedUVL);
	makeFunctionStub(shared_memory->sceKernelFreeMemBlockAddr, sceKernelFreeMemBlockPatchedUVL);
	makeFunctionStub(shared_memory->sceKernelCreateThreadAddr, sceKernelCreateThreadPatchedUVL);
	makeFunctionStub(shared_memory->sceKernelWaitThreadEndAddr, sceKernelWaitThreadEndPatchedUVL);
	makeFunctionStub(shared_memory->sceIoOpenAddr, sceIoOpenPatchedUVL);
	makeFunctionStub(shared_memory->sceIoLseekAddr, sceIoLseekPatchedUVL);
	makeFunctionStub(shared_memory->sceIoReadAddr, sceIoReadPatchedUVL);
	makeFunctionStub(shared_memory->sceIoWriteAddr, sceIoWritePatchedUVL);
	makeFunctionStub(shared_memory->sceIoCloseAddr, sceIoClosePatchedUVL);

	// Make uvl_alloc_code_mem return 0
	makeThumbDummyFunction0(extractFunctionStub((uint32_t)&uvl_alloc_code_mem) & ~0x1);

#ifdef DISABLE_UVL_LOGGING
	// Disable UVL logging because this causes crash for xerpi derpi
	makeThumbDummyFunction0(extractFunctionStub((uint32_t)&uvl_debug_log) & ~0x1);
	makeThumbDummyFunction0(extractFunctionStub((uint32_t)&uvl_log_write) & ~0x1);
#endif

	return 0;
}

void restoreUVLPatches() {
	makeStub(shared_memory->sceKernelAllocMemBlockAddr, (void *)extractStub((uint32_t)&sceKernelAllocMemBlock));
	makeStub(shared_memory->sceKernelFindMemBlockByAddrAddr, (void *)extractStub((uint32_t)&sceKernelFindMemBlockByAddr));
	makeStub(shared_memory->sceKernelFreeMemBlockAddr, (void *)extractStub((uint32_t)&sceKernelFreeMemBlock));
	makeStub(shared_memory->sceKernelCreateThreadAddr, (void *)extractStub((uint32_t)&sceKernelCreateThread));
	makeStub(shared_memory->sceKernelWaitThreadEndAddr, (void *)extractStub((uint32_t)&sceKernelWaitThreadEnd));
	makeStub(shared_memory->sceIoOpenAddr, (void *)extractStub((uint32_t)&sceIoOpen));
	makeStub(shared_memory->sceIoLseekAddr, (void *)extractStub((uint32_t)&sceIoLseek));
	makeStub(shared_memory->sceIoReadAddr, (void *)extractStub((uint32_t)&sceIoRead));
	makeStub(shared_memory->sceIoWriteAddr, (void *)extractStub((uint32_t)&sceIoWrite));
	makeStub(shared_memory->sceIoCloseAddr, (void *)extractStub((uint32_t)&sceIoClose));
}