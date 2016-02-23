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

#define COUNTUP_WAIT 100 * 1000
#define DIALOG_WAIT 900 * 1000

static uint32_t current_value = 0;

void closeWaitDialog() {
	sceMsgDialogClose();

	while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
		sceKernelDelayThread(1000);
	}
}

void SetProgress(uint32_t value, uint32_t max) {
	current_value = value;
}

int update_thread(SceSize args_size, UpdateArguments *args) {
/*
	uint32_t previous_value = current_value;
	SceUInt64 cur_micros = 0, delta_micros = 0, last_micros = 0;
	double kbs = 0;
*/
	while (current_value < args->max && isMessageDialogRunning()) {
		disableAutoSuspend();
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

SceUID createStartUpdateThread(uint32_t max) {
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
		disableAutoSuspend();

		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);
		removeEndSlash(path);

		getPathInfo(path, NULL, &folders, &files);

		mark_entry = mark_entry->next;
	}

	// Update thread
	thid = createStartUpdateThread(folders + files);

	// Remove process
	uint32_t value = 0;

	mark_entry = head;

	for (i = 0; i < count; i++) {
		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);
		removeEndSlash(path);

		int res = removePath(path, &value, folders + files, SetProgress);
		if (res < 0) {
			closeWaitDialog();
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

	return sceKernelExitDeleteThread(0);
}

int copy_thread(SceSize args_size, CopyArguments *args) {
	SceUID thid = -1;

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

			int res = sceIoRename(src_path, dst_path);
			// TODO: if (res == SCE_ERROR_ERRNO_EEXIST) if folder
			if (res < 0) {
				closeWaitDialog();
				errorDialog(res);
				goto EXIT;
			}

			SetProgress(i + 1, args->copy_list->length);

			copy_entry = copy_entry->next;
		}

		// Set progress to 100%
		sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
		sceKernelDelayThread(COUNTUP_WAIT);

		// Close
		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_MOVED;
	} else { // Copy
		if (args->copy_mode == COPY_MODE_EXTRACT)
			archiveOpen(args->archive_path);

		// Get src paths info
		uint32_t size = 0, folders = 0, files = 0;

		copy_entry = args->copy_list->head;

		int i;
		for (i = 0; i < args->copy_list->length; i++) {
			disableAutoSuspend();

			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", args->copy_list->path, copy_entry->name);

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				addEndSlash(src_path);
				getArchivePathInfo(src_path, &size, &folders, &files);
			} else {
				removeEndSlash(src_path);
				getPathInfo(src_path, &size, &folders, &files);
			}

			copy_entry = copy_entry->next;
		}

		// Update thread
		thid = createStartUpdateThread(size + folders);

		// Copy process
		uint32_t value = 0;

		copy_entry = args->copy_list->head;

		for (i = 0; i < args->copy_list->length; i++) {
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", args->copy_list->path, copy_entry->name);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, copy_entry->name);

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				addEndSlash(src_path);
				addEndSlash(dst_path);

				int res = extractArchivePath(src_path, dst_path, &value, size + folders, SetProgress);
				if (res < 0) {
					closeWaitDialog();
					errorDialog(res);
					goto EXIT;
				}
			} else {
				removeEndSlash(src_path);
				removeEndSlash(dst_path);

				int res = copyPath(src_path, dst_path, &value, size + folders, SetProgress);
				if (res < 0) {
					closeWaitDialog();
					errorDialog(res);
					goto EXIT;
				}
			}

			copy_entry = copy_entry->next;
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

	return sceKernelExitDeleteThread(0);
}

int split_thread(SceSize args_size, SplitArguments *args) {
	SceUID thid = -1;

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	SceUID fd = -1;
	void *buf = NULL;

	FileListEntry *file_entry = fileListGetNthEntry(args->file_list, args->index);

	char path[MAX_PATH_LENGTH];
	snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, file_entry->name);

	// Get file size
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	int res = sceIoGetstat(path, &stat);
	if (res < 0) {
		closeWaitDialog();
		errorDialog(res);
		goto EXIT;
	}

	// Update thread
	thid = createStartUpdateThread((uint32_t)stat.st_size);

	// Make folder
	char folder[MAX_PATH_LENGTH];
	snprintf(folder, MAX_PATH_LENGTH, "%s%s", path, SPLIT_SUFFIX);
	sceIoMkdir(folder, 0777);

	// Split process
	fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0) {
		closeWaitDialog();
		errorDialog(fd);
		goto EXIT;
	}

	buf = malloc(SPLIT_PART_SIZE);

	uint32_t seek = 0;
	while (seek < (uint32_t)stat.st_size) {
		int read = sceIoRead(fd, buf, SPLIT_PART_SIZE);
		if (read < 0) {
			closeWaitDialog();
			errorDialog(read);
			goto EXIT;
		}

		int part_num = seek / SPLIT_PART_SIZE;
		int folder_num = part_num / SPLIT_MAX_FILES;
		int file_num = part_num % SPLIT_MAX_FILES;

		char new_path[MAX_PATH_LENGTH];
		snprintf(new_path, MAX_PATH_LENGTH, "%s%s/%04d/%04d", path, SPLIT_SUFFIX, folder_num, file_num);

		int written = WriteFile(new_path, buf, read);

		// Failed, maybe folder doesn't exist yet?
		if (written < 0) {
			// Make folder
			char folder[MAX_PATH_LENGTH];
			snprintf(folder, MAX_PATH_LENGTH, "%s%s/%04d", path, SPLIT_SUFFIX, folder_num);
			sceIoMkdir(folder, 0777);

			// Retry
			written = WriteFile(new_path, buf, read);
		}

		if (written < 0) {
			closeWaitDialog();
			errorDialog(written);
			goto EXIT;
		}

		seek += read;

		SetProgress(seek, stat.st_size);
	}

	// Write size
	char size_path[MAX_PATH_LENGTH];
	snprintf(size_path, MAX_PATH_LENGTH, "%s%s/%s", path, SPLIT_SUFFIX, SPLIT_SIZE_NAME);
	WriteFile(size_path, &seek, sizeof(uint32_t));

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_SPLITTED;

EXIT:
	if (buf)
		free(buf);

	if (fd >= 0)
		sceIoClose(fd);

	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	return sceKernelExitDeleteThread(0);
}

int join_thread(SceSize args_size, JoinArguments *args) {
	SceUID thid = -1;

	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	SceUID fd = -1;
	void *buf = NULL;

	FileListEntry *file_entry = fileListGetNthEntry(args->file_list, args->index);

	char path[MAX_PATH_LENGTH];
	snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, file_entry->name);
	removeEndSlash(path);

	// Get file size
	char size_path[MAX_PATH_LENGTH];
	snprintf(size_path, MAX_PATH_LENGTH, "%s/%s", path, SPLIT_SIZE_NAME);

	uint32_t size = 0;
	int res = ReadFile(size_path, &size, sizeof(uint32_t));
	if (res < 0) {
		closeWaitDialog();
		errorDialog(res);
		goto EXIT;
	}

	// Update thread
	thid = createStartUpdateThread((uint32_t)size);

	// Join process
	char dst_path[MAX_PATH_LENGTH];
	strcpy(dst_path, path);

	char *p = strrchr(dst_path, '.');
	if (p)
		*p = '\0';

	fd = sceIoOpen(dst_path, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);
	if (fd < 0) {
		closeWaitDialog();
		errorDialog(fd);
		goto EXIT;
	}

	buf = malloc(SPLIT_PART_SIZE);

	uint32_t seek = 0;
	while (seek < size) {
		int part_num = seek / SPLIT_PART_SIZE;
		int folder_num = part_num / SPLIT_MAX_FILES;
		int file_num = part_num % SPLIT_MAX_FILES;

		char src_path[MAX_PATH_LENGTH];
		snprintf(src_path, MAX_PATH_LENGTH, "%s/%04d/%04d", path, folder_num, file_num);

		int read = ReadFile(src_path, buf, SPLIT_PART_SIZE);
		if (read < 0) {
			closeWaitDialog();
			errorDialog(read);
			goto EXIT;
		}

		int written = sceIoWrite(fd, buf, read);
		if (written < 0) {
			closeWaitDialog();
			errorDialog(written);
			goto EXIT;
		}

		seek += read;

		SetProgress(seek, size);
	}

	// Set progress to 100%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 100);
	sceKernelDelayThread(COUNTUP_WAIT);

	// Close
	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_JOINED;

EXIT:
	if (buf)
		free(buf);

	if (fd >= 0)
		sceIoClose(fd);

	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	return sceKernelExitDeleteThread(0);
}