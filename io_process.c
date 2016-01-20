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

#define DIALOG_WAIT 900 * 1000

void SetProgress(uint32_t value, uint32_t max) {
	double progress = (double)((100.0f * (double)value) / (double)max);
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, (int)progress);
}

int delete_thread(SceSize args_size, DeleteArguments *args) {
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
	uint32_t size = 0, folders = 0, files = 0;

	mark_entry = head;

	int i;
	for (i = 0; i < count; i++) {
		disableAutoSuspend();

		sprintf(path, "%s%s", args->cur_path, mark_entry->name);
		removeEndSlash(path);

		getPathInfo(path, &size, &folders, &files);

		mark_entry = mark_entry->next;
	}

	// Remove process
	uint32_t value = 0;

	mark_entry = head;

	for (i = 0; i < count; i++) {
		disableAutoSuspend();

		sprintf(path, "%s%s", args->cur_path, mark_entry->name);
		removeEndSlash(path);

		int res = removePath(path, &value, folders + files, SetProgress);
		if (res < 0) {
			sceMsgDialogClose();

			while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
				sceKernelDelayThread(100);
			}

			errorDialog(res);
			goto EXIT;
		}

		mark_entry = mark_entry->next;
	}

	sceMsgDialogClose();

	dialog_step = DIALOG_STEP_DELETED;

EXIT:
	if (mark_entry_one)
		free(mark_entry_one);

	return sceKernelExitDeleteThread(0);
}

int copy_thread(SceSize args_size, CopyArguments *args) {
	// Set progress to 0%
	sceMsgDialogProgressBarSetValue(SCE_MSG_DIALOG_PROGRESSBAR_TARGET_BAR_DEFAULT, 0);
	sceKernelDelayThread(DIALOG_WAIT); // Needed to see the percentage

	char src_path[MAX_PATH_LENGTH], dst_path[MAX_PATH_LENGTH];
	FileListEntry *copy_entry = NULL;

	if (args->copy_mode == COPY_MODE_MOVE) { // Move
		copy_entry = args->copy_list->head;

		int i;
		for (i = 0; i < args->copy_list->length; i++) {
			disableAutoSuspend();

			sprintf(src_path, "%s%s", args->copy_path, copy_entry->name);
			sprintf(dst_path, "%s%s", args->cur_path, copy_entry->name);

			int res = sceIoRename(src_path, dst_path);
			// TODO: if (res == 0x80010011) if folder
			if (res < 0) {
				sceMsgDialogClose();

				while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
					sceKernelDelayThread(100);
				}

				errorDialog(res);
				goto EXIT;
			}

			SetProgress(i + 1, args->copy_list->length);

			copy_entry = copy_entry->next;
		}

		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_MOVED;
	} else { // Copy
		if (args->copy_mode == COPY_MODE_EXTRACT) {
			archiveOpen(args->archive_path);
		}

		// Get src paths info
		uint32_t size = 0, folders = 0, files = 0;

		copy_entry = args->copy_list->head;

		int i;
		for (i = 0; i < args->copy_list->length; i++) {
			disableAutoSuspend();

			sprintf(src_path, "%s%s", args->copy_path, copy_entry->name);

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				addEndSlash(src_path);
				getArchivePathInfo(src_path, &size, &folders, &files);
			} else {
				removeEndSlash(src_path);
				getPathInfo(src_path, &size, &folders, &files);
			}

			copy_entry = copy_entry->next;
		}

		// Copy process
		uint32_t value = 0;

		copy_entry = args->copy_list->head;

		for (i = 0; i < args->copy_list->length; i++) {
			disableAutoSuspend();

			sprintf(src_path, "%s%s", args->copy_path, copy_entry->name);
			sprintf(dst_path, "%s%s", args->cur_path, copy_entry->name);

			if (args->copy_mode == COPY_MODE_EXTRACT) {
				addEndSlash(src_path);
				addEndSlash(dst_path);

				int res = extractArchivePath(src_path, dst_path, &value, size + folders, SetProgress);
				if (res < 0) {
					sceMsgDialogClose();

					while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
						sceKernelDelayThread(100);
					}

					errorDialog(res);
					goto EXIT;
				}
			} else {
				removeEndSlash(src_path);
				removeEndSlash(dst_path);

				int res = copyPath(src_path, dst_path, &value, size + folders, SetProgress);
				if (res < 0) {
					sceMsgDialogClose();

					while (updateMessageDialog() != MESSAGE_DIALOG_RESULT_NONE) {
						sceKernelDelayThread(100);
					}

					errorDialog(res);
					goto EXIT;
				}
			}

			copy_entry = copy_entry->next;
		}

		sceMsgDialogClose();

		dialog_step = DIALOG_STEP_COPIED;
	}

EXIT:
	return sceKernelExitDeleteThread(0);
}