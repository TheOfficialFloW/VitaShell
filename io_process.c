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
#include "io_process.h"
#include "archive.h"
#include "file.h"
#include "message_dialog.h"
#include "utils.h"

static uint64_t current_value = 0;

void closeWaitDialog() {
	sceMsgDialogClose();

	while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
		sceKernelDelayThread(1000);
	}
}

int cancelHandler() {
	return (updateMessageDialog() != MESSAGE_DIALOG_RESULT_RUNNING);
}

void SetProgress(uint64_t value, uint64_t max) {
	current_value = value;
}

int update_thread(SceSize args_size, UpdateArguments *args) {
/*
	uint64_t previous_value = current_value;
	SceUInt64 cur_micros = 0, delta_micros = 0, last_micros = 0;
	double kbs = 0;
*/
	while (current_value < args->max && isMessageDialogRunning()) {
/*
		// Show KB/s
		cur_micros = sceKernelGetProcessTimeWide();
		if (cur_micros >= (last_micros + 1000000)) {
			delta_micros = cur_micros - last_micros;
			last_micros = cur_micros;
			kbs = (double)(current_value - previous_value) / 1024.0f;
			previous_value = current_value;

			char msg[32];
			sprintf(msg, "%.2f KB/s", kbs);
			sceMsgDialogProgressBarSetMsg(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (SceChar8 *)msg);
		}
*/
		double progress = (double)((100.0f * (double)current_value) / (double)args->max);
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (int)progress);

		sceKernelDelayThread(COUNTUP_WAIT);
	}

	return sceKernelExitDeleteThread(0);
}

SceUID createStartUpdateThread(uint64_t max) {
	current_value = 0;

	UpdateArguments args;
	args.max = max;

	SceUID thid = sceKernelCreateThread("update_thread", (SceKernelThreadEntry)update_thread, 0xBF, 0x4000, 0, 0, NULL);
	if (thid >= 0)
		sceKernelStartThread(thid, sizeof(UpdateArguments), &args);

	return thid;
}

int delete_thread(SceSize args_size, DeleteArguments *args) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	FileListEntry *file_entry = fileListGetNthEntry(args->file_list, args->index);

	int count = 0;
	FileListEntry *head = NULL;
	FileListEntry *mark_entry_one = NULL;

	if (fileListFindEntry(args->mark_list, file_entry->name)) { // On marked entry
		count = args->mark_list->length;
		head = args->mark_list->head;
	} else {
		count = 1;
		mark_entry_one = malloc(sizeof(FileListEntry));
		strcpy(mark_entry_one->name, file_entry->name);
		head = mark_entry_one;
	}

	char path[MAX_PATH_LENGTH];
	FileListEntry *mark_entry = NULL;

	// Get paths info
	uint32_t folders = 0, files = 0;

	mark_entry = head;

	int i;
	for (i = 0; i < count; i++) {
		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);
		removeEndSlash(path);

		getPathInfo(path, NULL, &folders, &files);

		mark_entry = mark_entry->next;
	}

	// Update thread
	thid = createStartUpdateThread(folders + files);

	// Remove process
	uint64_t value = 0;

	mark_entry = head;

	for (i = 0; i < count; i++) {
		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);
		removeEndSlash(path);

		FileProcessParam param;
		param.value = &value;
		param.max = folders + files;
		param.SetProgress = SetProgress;
		param.cancelHandler = cancelHandler;
		int res = removePath(path, &param);
		if (res <= 0) {
			closeWaitDialog();
			dialog_step = DIALOG_STEP_CANCELLED;
			errorDialog(res);
			goto EXIT;
		}

		mark_entry = mark_entry->next;
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_DELETED;

EXIT:
	if (mark_entry_one)
		free(mark_entry_one);

	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}

int copy_thread(SceSize args_size, CopyArguments *args) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	char src_path[MAX_PATH_LENGTH], dst_path[MAX_PATH_LENGTH];
	FileListEntry *copy_entry = NULL;

	if (args->copy_mode == COPY_MODE_MOVE) { // Move
		// Update thread
		thid = createStartUpdateThread(args->copy_list->length);

		copy_entry = args->copy_list->head;

		int i;
		for (i = 0; i < args->copy_list->length; i++) {
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", args->copy_list->path, copy_entry->name);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, copy_entry->name);
			removeEndSlash(src_path);
			removeEndSlash(dst_path);

			int res = movePath(src_path, dst_path, MOVE_INTEGRATE | MOVE_REPLACE, NULL);
			if (res < 0) {
				closeWaitDialog();
				errorDialog(res);
				goto EXIT;
			}

			SetProgress(i + 1, args->copy_list->length);

			if (cancelHandler()) {
				closeWaitDialog();
				dialog_step = DIALOG_STEP_CANCELLED;
				goto EXIT;
			}

			copy_entry = copy_entry->next;
		}

		// Set progress to 100%
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
		sceKernelDelayThread(COUNTUP_WAIT);

		// Close
		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_MOVED;
	} else { // Copy
		// Open archive, because when you copy from an archive, you leave the archive to paste
		if (args->copy_mode == COPY_MODE_EXTRACT) {
			int res = archiveOpen(args->archive_path);
			if (res < 0) {
				closeWaitDialog();
				errorDialog(res);
				goto EXIT;
			}
		}

		// Get src paths info
		uint64_t size = 0;
		uint32_t folders = 0, files = 0;

		copy_entry = args->copy_list->head;

		int i;
		for (i = 0; i < args->copy_list->length; i++) {
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", args->copy_list->path, copy_entry->name);
			removeEndSlash(src_path);

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				getArchivePathInfo(src_path, &size, &folders, &files);
			} else {
				getPathInfo(src_path, &size, &folders, &files);
			}

			copy_entry = copy_entry->next;
		}

		// Update thread
		thid = createStartUpdateThread(size + folders);

		// Copy process
		uint64_t value = 0;

		copy_entry = args->copy_list->head;

		for (i = 0; i < args->copy_list->length; i++) {
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", args->copy_list->path, copy_entry->name);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, copy_entry->name);
			removeEndSlash(src_path);
			removeEndSlash(dst_path);

			FileProcessParam param;
			param.value = &value;
			param.max = size + folders;
			param.SetProgress = SetProgress;
			param.cancelHandler = cancelHandler;

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				int res = extractArchivePath(src_path, dst_path, &param);
				if (res <= 0) {
					closeWaitDialog();
					dialog_step = DIALOG_STEP_CANCELLED;
					errorDialog(res);
					goto EXIT;
				}
			} else {
				int res = copyPath(src_path, dst_path, &param);
				if (res <= 0) {
					closeWaitDialog();
					dialog_step = DIALOG_STEP_CANCELLED;
					errorDialog(res);
					goto EXIT;
				}
			}

			copy_entry = copy_entry->next;
		}

		// Close archive
		if (args->copy_mode == COPY_MODE_EXTRACT) {
			int res = archiveClose();
			if (res < 0) {
				closeWaitDialog();
				errorDialog(res);
				goto EXIT;
			}
		}

		// Set progress to 100%
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
		sceKernelDelayThread(COUNTUP_WAIT);

		// Close
		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_COPIED;
	}

EXIT:
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}

int hash_thread(SceSize args_size, HashArguments *args) {
	SceUID thid = -1;

	// Lock power timers
	powerLock();

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	uint64_t max = (uint64_t) (getFileSize(args->file_path)/(TRANSFER_SIZE));

	// SHA1 process
	uint64_t value = 0;

	// Spin off a thread to update the progress dialog 
	thid = createStartUpdateThread(max);

	FileProcessParam param;
	param.value = &value;
	param.max = max;
	param.SetProgress = SetProgress;
	param.cancelHandler = cancelHandler;

	uint8_t sha1out[20];
	int res = getFileSha1(args->file_path, sha1out, &param);
	if (res <= 0) {
		// SHA1 Didn't complete successfully, or was cancelled
		closeWaitDialog();
		dialog_step = DIALOG_STEP_CANCELLED;
		errorDialog(res);
		goto EXIT;
	}

	// Since we hit here, we're done. Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	closeWaitDialog();

	char sha1msg[42];
	memset(sha1msg, 0, sizeof(sha1msg));

	// Construct SHA1 sum string
	int i;
	for (i = 0; i < 20; i++) {
		char string[4];
		sprintf(string, "%02X", sha1out[i]);
		strcat(sha1msg, string);

		if (i == 9)
			strcat(sha1msg, "\n");
	}

	sha1msg[41] = '\0';

	initMessageDialog(SCE_MSG_DIALOG_BUTTON_TYPE_OK, sha1msg);
	dialog_step = DIALOG_STEP_HASH_DISPLAY;

	// Wait for response
	while (dialog_step == DIALOG_STEP_HASH_DISPLAY) {
		sceKernelDelayThread(1000);
	}

	closeWaitDialog();
	sceMsgDialogClose();

EXIT:

	// Ensure the update thread ends gracefully
	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	powerUnlock();

	// Kill current thread
	return sceKernelExitDeleteThread(0);
}
