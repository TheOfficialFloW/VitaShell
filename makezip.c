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
#include "io_process.h"
#include "makezip.h"
#include "file.h"
#include "utils.h"

#include "minizip/zip.h"

void convertToZipTime(SceDateTime *time, tm_zip *tmzip) {
	SceDateTime time_local;
	convertUtcToLocalTime(&time_local, time);

	tmzip->tm_sec  = time_local.second;
	tmzip->tm_min  = time_local.minute;
	tmzip->tm_hour = time_local.hour;
	tmzip->tm_mday = time_local.day;
	tmzip->tm_mon  = time_local.month;
	tmzip->tm_year = time_local.year;
}

int zipAddFile(zipFile zf, char *path, int filename_start, int level, FileProcessParam *param) {
	int res;

	// Get file stat
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	res = sceIoGetstat(path, &stat);
	if (res < 0)
		return res;

	// Get file local time
	zip_fileinfo zi;
	memset(&zi, 0, sizeof(zip_fileinfo));
	convertToZipTime(&stat.st_mtime, &zi.tmz_date);

	// Large file?
	int use_zip64 = (stat.st_size >= 0xFFFFFFFF);

	// Open new file in zip
	char filename[MAX_PATH_LENGTH];
	strcpy(filename, path + filename_start);

	res = zipOpenNewFileInZip3_64(zf, filename, &zi,
				NULL, 0, NULL, 0, NULL,
				(level != 0) ? Z_DEFLATED : 0,
				level, 0,
				-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
				NULL, 0, use_zip64);

	if (res < 0)
		return res;

	// Open file to add
	SceUID fd = sceIoOpen(path, SCE_O_RDONLY, 0);
	if (fd < 0) {
		zipCloseFileInZip(zf);
		return fd;
	}

	// Add file to zip
	void *buf = malloc(TRANSFER_SIZE);

	uint64_t seek = 0;

	while (1) {
		int read = sceIoRead(fd, buf, TRANSFER_SIZE);
		if (read == SCE_ERROR_ERRNO_ENODEV) {
			fd = sceIoOpen(path, SCE_O_RDONLY, 0);
			if (fd >= 0) {
				sceIoLseek(fd, seek, SCE_SEEK_SET);
				read = sceIoRead(fd, buf, TRANSFER_SIZE);
			}
		}

		if (read < 0) {
			free(buf);

			sceIoClose(fd);
			zipCloseFileInZip(zf);

			return read;
		}

		if (read == 0)
			break;

		int written = zipWriteInFileInZip(zf, buf, read);
		if (written < 0) {
			free(buf);

			sceIoClose(fd);
			zipCloseFileInZip(zf);

			return written;
		}

		seek += written;

		if (param) {
			if (param->value)
				(*param->value) += read;

			if (param->SetProgress)
				param->SetProgress(param->value ? *param->value : 0, param->max);

			if (param->cancelHandler && param->cancelHandler()) {
				free(buf);

				sceIoClose(fd);
				zipCloseFileInZip(zf);

				return 0;
			}
		}
	}

	free(buf);

	sceIoClose(fd);
	zipCloseFileInZip(zf);

	return 1;
}

int zipAddFolder(zipFile zf, char *path, int filename_start, int level, FileProcessParam *param) {
	int res;

	// Get file stat
	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	res = sceIoGetstat(path, &stat);
	if (res < 0)
		return res;

	// Get file local time
	zip_fileinfo zi;
	memset(&zi, 0, sizeof(zip_fileinfo));
	convertToZipTime(&stat.st_mtime, &zi.tmz_date);

	// Open new file in zip
	char filename[MAX_PATH_LENGTH];
	strcpy(filename, path + filename_start);
	addEndSlash(filename);

	res = zipOpenNewFileInZip3_64(zf, filename, &zi,
				NULL, 0, NULL, 0, NULL,
				(level != 0) ? Z_DEFLATED : 0,
				level, 0,
				-MAX_WBITS, DEF_MEM_LEVEL, Z_DEFAULT_STRATEGY,
				NULL, 0, 0);

	if (res < 0)
		return res;

	if (param) {
		if (param->value)
			(*param->value)++;

		if (param->SetProgress)
			param->SetProgress(param->value ? *param->value : 0, param->max);

		if (param->cancelHandler && param->cancelHandler()) {
			zipCloseFileInZip(zf);
			return 0;
		}
	}

	zipCloseFileInZip(zf);

	return 1;
}

int zipAddPath(zipFile zf, char *path, int filename_start, int level, FileProcessParam *param) {
	SceUID dfd = sceIoDopen(path);
	if (dfd >= 0) {
		int ret = zipAddFolder(zf, path, filename_start, level, param);
		if (ret <= 0)
			return ret;

		int res = 0;

		do {
			SceIoDirent dir;
			memset(&dir, 0, sizeof(SceIoDirent));

			res = sceIoDread(dfd, &dir);
			if (res > 0) {
				char *new_path = malloc(strlen(path) + strlen(dir.d_name) + 2);
				snprintf(new_path, MAX_PATH_LENGTH, "%s%s%s", path, hasEndSlash(path) ? "" : "/", dir.d_name);

				int ret = 0;

				if (SCE_S_ISDIR(dir.d_stat.st_mode)) {
					ret = zipAddPath(zf, new_path, filename_start, level, param);
				} else {
					ret = zipAddFile(zf, new_path, filename_start, level, param);
				}

				free(new_path);

				// Some folders are protected and return 0x80010001. Bypass them
				if (ret <= 0 && ret != 0x80010001) {
					sceIoDclose(dfd);
					return ret;
				}
			}
		} while (res > 0);

		sceIoDclose(dfd);
	} else {
		return zipAddFile(zf, path, filename_start, level, param);
	}

	return 1;
}

int makeZip(char *zip_file, char *src_path, int filename_start, int level, int append, FileProcessParam *param) {
	zipFile zf = zipOpen64(zip_file, append);
	if (zf == NULL)
		return -1;

	int res = zipAddPath(zf, src_path, filename_start, level, param);

	zipClose(zf, NULL);

	return res;
}

int compress_thread(SceSize args_size, CompressArguments *args) {
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
		mark_entry_one->type = file_entry->type;
		head = mark_entry_one;
	}

	char path[MAX_PATH_LENGTH];
	FileListEntry *mark_entry = NULL;

	// Get paths info
	uint64_t size = 0;
	uint32_t folders = 0, files = 0;

	mark_entry = head;

	int i;
	for (i = 0; i < count; i++) {
		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);

		getPathInfo(path, &size, &folders, &files, NULL);

		mark_entry = mark_entry->next;
	}

	// Check memory card free space
	double guessed_size = (double)size * 0.7f;
	if (checkMemoryCardFreeSpace((uint64_t)guessed_size))
		goto EXIT;

	// Update thread
	thid = createStartUpdateThread(size + folders);

	// Remove process
	uint64_t value = 0;

	mark_entry = head;

	for (i = 0; i < count; i++) {
		snprintf(path, MAX_PATH_LENGTH, "%s%s", args->file_list->path, mark_entry->name);

		FileProcessParam param;
		param.value = &value;
		param.max = size;
		param.SetProgress = SetProgress;
		param.cancelHandler = cancelHandler;

		int res = makeZip(args->path, path, strlen(args->file_list->path), args->level, i == 0 ? APPEND_STATUS_CREATE : APPEND_STATUS_ADDINZIP, &param);
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

	dialog_step = DIALOG_STEP_COMPRESSED;

EXIT:
	if (mark_entry_one)
		free(mark_entry_one);

	if (thid >= 0)
		sceKernelWaitThreadEnd(thid, NULL, NULL);

	// Unlock power timers
	powerUnlock();

	return sceKernelExitDeleteThread(0);
}
