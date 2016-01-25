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

/*
	TODO:
	- Unload prxs
	- Delete events, callbacks, msg pipes, rw locks
*/

int sceKernelCreateLwMutex(void *work, const char *name, SceUInt attr, int initCount, void *option);
int sceKernelDeleteLwMutex(void *work);

static void *code_memory = NULL;
static SceUID code_blockid = INVALID_UID;

static void *uvl_backup = NULL;
static uint32_t uvl_addr = 0;

static int force_exit = 0;
static int exited = 0;

static SceUID exit_thid = INVALID_UID;

static SceUID UVLTemp_id = INVALID_UID, UVLHomebrew_id = INVALID_UID;
static SceModuleInfo hb_mod_info;

static SceUID hb_fds[MAX_UIDS];
static SceUID hb_thids[MAX_UIDS];
static SceUID hb_semaids[MAX_UIDS];
static SceUID hb_mutexids[MAX_UIDS];
static SceUID hb_blockids[MAX_UIDS];

void *hb_lw_mutex_works[MAX_LW_MUTEXES];

static int hb_audio_ports[MAX_AUDIO_PORTS];

static SceGxmContext *hb_gxm_context = NULL;
static SceGxmRenderTarget *hb_gxm_render = NULL;
static SceGxmShaderPatcher *hb_shader_patcher = NULL;

static SceGxmSyncObject *hb_sync_objects[MAX_SYNC_OBJECTS];

static SceGxmFragmentProgram *hb_fragment_programs[MAX_GXM_PRGRAMS];
static SceGxmShaderPatcherId hb_fragment_program_ids[MAX_GXM_PRGRAMS];

static SceGxmVertexProgram *hb_vertex_programs[MAX_GXM_PRGRAMS];
static SceGxmShaderPatcherId hb_vertex_program_ids[MAX_GXM_PRGRAMS];

extern unsigned char _binary_payload_payload_bin_start;
extern unsigned char _binary_payload_payload_bin_end;
extern unsigned char _binary_payload_payload_bin_size; // crashes

/*
	* Self-reloading VitaShell *
	- Restore uvl
	- Load payload to code_memory
	- Remove sceKernelWaitThreadEnd and replace by sceKernelExitDeleteThread in uvl. This will then not wait for thread to end but will exit the previous vitashell thread.
	- Patch allocation VM and use old .text segment, so no new allocation is needed. By dummying uvl_alloc_code_mem and patching sceKernelFindMemBlockByAddr to return code_blockid.
	- When it has been exited, free the previous .data segment and load new VitaShell to old .text segment with uvl_load.
	- What is needed to be passed to the payload: path, uvl_load, uvl_unlock_mem, uvl_lock_mem, uvl_flush_icache to do patching.
	  sceKernelExitDeleteThread function to replace. The old .text blockid of code memory must be passed (not code_blockid).
*/

typedef struct {
	char *path;
	int (* uvl_load)(const char *path);
	int (* sceKernelExitDeleteThread)(int status);
} PayloadArgs;

void payload() {
	// Restore uvl
	restoreUVL();

	int size = &_binary_payload_payload_bin_end - &_binary_payload_payload_bin_start;

	// Copy payload to code memory
	uvl_unlock_mem();
	memcpy(code_memory, &_binary_payload_payload_bin_start, size);
	uvl_lock_mem();
	uvl_flush_icache(code_memory, size);

	// Call payload
	PayloadArgs args = { (void *)uvl_load, (void *)sceKernelExitDeleteThread };
	int (* executePayload)(PayloadArgs *args) = (void *)((uint32_t)code_memory | 0x1);
	executePayload(&args);

	while (1); // Should never reach here
}

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

void initCodeMemory() {
	unsigned int length = MAX_CODE_SIZE;
	code_memory = uvl_alloc_code_mem(&length);
	code_blockid = sceKernelFindMemBlockByAddr(code_memory, 0);	
}

void initHomebrewPatch() {
	force_exit = 0;
	exited = 0;

	exit_thid = INVALID_UID;

	UVLTemp_id = INVALID_UID;
	UVLHomebrew_id = INVALID_UID;
	memset((void *)&hb_mod_info, 0, sizeof(SceModuleInfo));

	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		hb_fds[i] = INVALID_UID;
		hb_thids[i] = INVALID_UID;
		hb_semaids[i] = INVALID_UID;
		hb_mutexids[i] = INVALID_UID;
		hb_blockids[i] = INVALID_UID;
	}

	memset(hb_lw_mutex_works, 0, sizeof(hb_lw_mutex_works));

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

void loadHomebrew(char *file) {
	// Finish netdbg
	netdbg_fini();

	// Finish
	finishVita2dLib();
	finishSceAppUtil();

	// Init
	initHomebrewPatch();

	// Load
	uvl_load(file);

	// Init
	initVita2dLib();
	initSceAppUtil();

	// Init netdbg
	netdbg_init();
}

void finishGxm() {
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

// TODO: clean up no matter if force exit or not
void homebrewCleanUp() {
	// Wait for exit thread to end
	exited = 1;
	sceKernelWaitThreadEnd(exit_thid, NULL, NULL);

	if (force_exit) {
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
		if (hb_blockids[i] < 0)
			continue;

		int res = 0;

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
		if (res >= 0)
			hb_blockids[i] = INVALID_UID;
	}
}

void signalDeleteSema() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_semaids[i] >= 0) {
			int res = sceKernelSignalSema(hb_semaids[i], 1);
			debugPrintf("Signal sema 0x%08X: 0x%08X\n", hb_semaids[i], res);

			res = sceKernelDeleteSema(hb_semaids[i]);
			debugPrintf("Delete sema 0x%08X: 0x%08X\n", hb_semaids[i], res);
			if (res >= 0)
				hb_semaids[i] = INVALID_UID;
		}
	}
}

void unlockDeleteMutex() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_mutexids[i] >= 0) {
			int res = sceKernelUnlockMutex(hb_mutexids[i], 1);
			debugPrintf("Unlock mutex 0x%08X: 0x%08X\n", hb_mutexids[i], res);

			res = sceKernelDeleteMutex(hb_mutexids[i]);
			debugPrintf("Delete mutex 0x%08X: 0x%08X\n", hb_mutexids[i], res);
			if (res >= 0)
				hb_mutexids[i] = INVALID_UID;
		}
	}
}

void deleteLwMutex() {
	int i;
	for (i = 0; i < MAX_LW_MUTEXES; i++) {
		if (hb_lw_mutex_works[i]) {
			int res = sceKernelDeleteLwMutex(hb_lw_mutex_works[i]);
			debugPrintf("Delete lw mutex 0x%08X: 0x%08X\n", hb_lw_mutex_works[i], res);
			if (res >= 0)
				hb_lw_mutex_works[i] = NULL;
		}
	}
}

void waitThreadEnd() {
	int i;
	for (i = 0; i < MAX_UIDS; i++) {
		if (hb_thids[i] >= 0) {
			//debugPrintf("Wait for 0x%08X\n", hb_thids[i]);
			int res = sceKernelWaitThreadEnd(hb_thids[i], NULL, NULL);
			if (res >= 0)
				hb_thids[i] = INVALID_UID;
		}
	}
}

int exitThread() {
	debugPrintf("Exiting 0x%08X...\n", sceKernelGetThreadId());

	signalDeleteSema();
	unlockDeleteMutex();
	deleteLwMutex();

	debugPrintf("Wait for threads...\n");
	waitThreadEnd();

	sceKernelDelayThread(50 * 1000);

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

	debugPrintf("%s %s 0x%08X: 0x%08X\n", __FUNCTION__, name, stackSize, thid);

	if (thid >= 0)
		INSERT_UID(hb_thids, thid);

	return thid;
}

SceUID sceKernelAllocMemBlockPatchedHB(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	void *addr = NULL;
	sceKernelGetMemBlockBase(blockid, &addr);
	debugPrintf("%s %s 0x%08X 0x%08X: 0x%08X, 0x%08X\n", __FUNCTION__, name, type, size, blockid, addr);

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

	{ "SceSysmem", 0xA91E15EE, sceKernelFreeMemBlockPatchedHB },
	{ "SceSysmem", 0xB9D5EBDE, sceKernelAllocMemBlockPatchedHB },
};

SceUID sceKernelCreateThreadPatchedUVL(const char *name, SceKernelThreadEntry entry, int initPriority, int stackSize, SceUInt attr, int cpuAffinityMask, const SceKernelThreadOptParam *option) {
	// debugPrintf("Module name: %s\n", hb_mod_info.name);

	if (strcmp(hb_mod_info.name, "VitaShell.elf") == 0) {
		// Pass address of the current code and data memblocks to unused stubs
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, (uint32_t)code_memory, "SceLibKernel", 0x894037E8), (void *)(uint32_t)&sceKernelCreateThreadPatchedUVL); // sceKernelBacktrace
		makeFunctionStub(findModuleImportByInfo(&hb_mod_info, (uint32_t)code_memory, "SceLibKernel", 0xD16C03B0), (void *)&hb_mod_info); // sceKernelBacktraceSelf

		restoreUVL();
		_free_vita_newlib();
	} else {
		exit_thid = sceKernelCreateThread("exit_thread", (SceKernelThreadEntry)exit_thread, 191, 0x1000, 0, 0, NULL);
		if (exit_thid >= 0)
			sceKernelStartThread(exit_thid, 0, NULL);

		int i;
		for (i = 0; i < (sizeof(patches_init) / sizeof(PatchNID)); i++) {
			makeFunctionStub(findModuleImportByInfo(&hb_mod_info, (uint32_t)code_memory, patches_init[i].library, patches_init[i].nid), patches_init[i].function);
		}
	}

	return sceKernelCreateThread(name, entry, initPriority, stackSize, attr, cpuAffinityMask, option);
}

int sceKernelWaitThreadEndPatchedUVL(SceUID thid, int *stat, SceUInt *timeout) {
	int res = sceKernelWaitThreadEnd(thid, stat, timeout);
	homebrewCleanUp();
	return res;
}

SceUID sceKernelAllocMemBlockPatchedUVL(const char *name, SceKernelMemBlockType type, int size, void *optp) {
	SceUID blockid = sceKernelAllocMemBlock(name, type, size, optp);

	void *addr = NULL;
	sceKernelGetMemBlockBase(blockid, &addr);
	debugPrintf("%s %s 0x%08X 0x%08X: 0x%08X, 0x%08X\n", __FUNCTION__, name, type, size, blockid, addr);

	if (strcmp(name, "UVLTemp") == 0) {
		UVLTemp_id = blockid;
	} else if (strcmp(name, "UVLHomebrew") == 0) {
		UVLHomebrew_id = blockid;
	}

	return blockid;
}

SceUID sceKernelFindMemBlockByAddrPatchedUVL(const void *addr, SceSize size) {
	debugPrintf("%s 0x%08X 0x%08X\n", __FUNCTION__, addr, size);

	if (addr == NULL)
		return code_blockid;

	return sceKernelFindMemBlockByAddr(addr, size);
}

int sceKernelFreeMemBlockPatchedUVL(SceUID uid) {
	debugPrintf("%s 0x%08X\n", __FUNCTION__, uid);

	if (uid == code_blockid) {
		return 0;
	} else if (uid == UVLTemp_id) {
		void *buf = NULL;
		int res = sceKernelGetMemBlockBase(UVLTemp_id, &buf);
		if (res < 0)
			return res;

		memcpy((void *)&hb_mod_info, (void *)getElfModuleInfo(buf), sizeof(SceModuleInfo));
	}

	return sceKernelFreeMemBlock(uid);
}

SceUID sceIoOpenPatchedUVL(const char *file, int flags, SceMode mode) {
	debugPrintf("%s %s\n", __FUNCTION__, file);
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
	{ -1, (uint32_t)&sceKernelFindMemBlockByAddr, sceKernelFindMemBlockByAddrPatchedUVL },
	{ -1, (uint32_t)&sceKernelFreeMemBlock, sceKernelFreeMemBlockPatchedUVL },
	{ -1, (uint32_t)&sceKernelCreateThread, sceKernelCreateThreadPatchedUVL },
	{ -1, (uint32_t)&sceKernelWaitThreadEnd, sceKernelWaitThreadEndPatchedUVL },
	{ -1, (uint32_t)&sceIoOpen, sceIoOpenPatchedUVL },
	{ -1, (uint32_t)&sceIoLseek, sceIoLseekPatchedUVL },
	{ -1, (uint32_t)&sceIoRead, sceIoReadPatchedUVL }, // COULD NOT BE PATCHED, DIFFERENT NID
	{ -1, (uint32_t)&sceIoWrite, sceIoWritePatchedUVL },
	{ -1, (uint32_t)&sceIoClose, sceIoClosePatchedUVL }, // COULD NOT BE PATCHED, DIFFERENT NID
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
	while (i < MAX_UVL_SIZE && count < N_UVL_PATCHES) {
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

	// Make uvl_alloc_code_mem return 0
	makeThumbDummyFunction0(extractFunctionStub((uint32_t)&uvl_alloc_code_mem) & ~0x1);
}

void getUVLTextAddr() {
	SceKernelMemBlockInfo info;
	findMemBlockByAddr(extractFunctionStub((uint32_t)&uvl_load), &info);
	uvl_addr = (uint32_t)info.mappedBase;
}

void backupUVL() {
	if (!uvl_backup) {
		uvl_backup = malloc(MAX_UVL_SIZE);
		memcpy(uvl_backup, (void *)uvl_addr, MAX_UVL_SIZE);
	}
}

void restoreUVL() {
	if (uvl_backup) {
		uvl_unlock_mem();

		memcpy((void *)uvl_addr, uvl_backup, MAX_UVL_SIZE);

		uvl_lock_mem();
		uvl_flush_icache((void *)uvl_addr, MAX_UVL_SIZE);

		free(uvl_backup);
		uvl_backup = NULL;
	}
}
