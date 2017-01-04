#include "archiveRAR.h"
#include "main.h"

static ArchiveFileNode* root = NULL;
static int32_t filePathLength = -1;


int archiveRAROpen(char *file) {
	root = openRARFile(file);
	if(!root) {
		return -1;
	}
	filePathLength = strlen(file);
	return 1;
}

int fileListGetRARArchiveEntries(FileList *list, char *path, int sort) {
	if(!root)
		return -1;

	FileListEntry *entry = malloc(sizeof(FileListEntry));
	strcpy(entry->name, DIR_UP);
	entry->name_length = strlen(DIR_UP);
	entry->is_folder = 1;
	entry->type = FILE_TYPE_UNKNOWN;
	fileListAddEntry(list, entry, sort);

	ArchiveFileNode* node = NULL;
	if(strlen(path) == (filePathLength + sizeof(char)))
		node = root;
	else
		node = getFloderNodeFromPath(root,path + filePathLength + sizeof(char));

	uint32_t i = 0;
	for(; i < node->childCount; i++) {
		ArchiveFileNode* _t = (ArchiveFileNode*)(node->childPt[i]);
		FileListEntry * childentry = malloc(sizeof(FileListEntry));

		strcpy(childentry->name,_t->nodeName);
		childentry->name_length = strlen(childentry->name);
		childentry->size = _t->data.UnpSize;
		childentry->size2 = _t->data.PackSize;

		sceRtcSetDosTime(&childentry->atime,_t->data.FileTime);
		sceRtcSetDosTime(&childentry->ctime,_t->data.FileTime);
		sceRtcSetDosTime(&childentry->mtime,_t->data.FileTime);

		if(_t->data.Flags == 0x20) {
			addEndSlash(childentry->name);
			childentry->is_folder = 1;
			list->folders++;
		} else {
			childentry->type = getFileType(childentry->name);
			childentry->is_folder = 0;
			list->files++;
		}
		fileListAddEntry(list, childentry, sort);
	}
	return 1;
}

int ReadArchiveRARFile(char *file, void *buf, int size) {
	if(filePathLength < 0)
		return -1;

	char filepath[filePathLength + sizeof(char)];
	strncpy(filepath,file,filePathLength);
	filepath[filePathLength] = '\0';
struct ExtractHeader header = {memPtr:buf,offset:0,bufferSize:size,file:0,param:NULL};
	if(extractRAR(filepath,file + (filePathLength + sizeof(char)),&header) > 0)
		return header.offset;
	else
		return -1;
}

int archiveRARFileGetstat(const char *file, SceIoStat *stat) {
	if(!root)
		return -1;

	const char* p = file + (filePathLength + sizeof(char));
	uint32_t p_length = strlen(p);

	if(p[p_length - 1] == '/')
		return -1;

	ArchiveFileNode* filenode = getFileNodeFromFilePath(root,p);
	if(!filenode)
		return -1;

	if(stat) {
		stat->st_size = filenode->data.UnpSize;
		sceRtcSetDosTime(&stat->st_atime,filenode->data.FileTime);
		sceRtcSetDosTime(&stat->st_ctime,filenode->data.FileTime);
		sceRtcSetDosTime(&stat->st_mtime,filenode->data.FileTime);
	}

	return 1;
}

int getRARArchivePathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files) {
	if(!root)
		return -1;

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (archiveRARFileGetstat(path, &stat) < 0) {
		FileList list;
		memset(&list, 0, sizeof(FileList));
		fileListGetRARArchiveEntries(&list, path, SORT_NONE);

		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *new_path = malloc(strlen(path) + strlen(entry->name) + 2);
			snprintf(new_path, MAX_PATH_LENGTH, "%s%s", path, entry->name);

			getRARArchivePathInfo(new_path, size, folders, files);

			free(new_path);

			entry = entry->next;
		}

		if (folders)
			(*folders)++;

		fileListEmpty(&list);
	} else {
		if (size)
			(*size) += stat.st_size;

		if (files)
			(*files)++;
	}

	return 0;
}

int extractRARArchivePath(char *src, char *dst, FileProcessParam *param) {
	if(!root)
		return -1;

	SceIoStat stat;
	memset(&stat, 0, sizeof(SceIoStat));
	if (archiveRARFileGetstat(src, &stat) < 0) {
		FileList list;
		memset(&list, 0, sizeof(FileList));
		fileListGetRARArchiveEntries(&list, src, SORT_NONE);

		int ret = sceIoMkdir(dst, 0777);
		if (ret < 0 && ret != SCE_ERROR_ERRNO_EEXIST) {
			fileListEmpty(&list);
			return ret;
		}

		if (param) {
			if (param->value)
				(*param->value) += DIRECTORY_SIZE;

			if (param->SetProgress)
				param->SetProgress(param->value ? *param->value : 0, param->max);

			if (param->cancelHandler && param->cancelHandler()) {
				fileListEmpty(&list);
				return 0;
			}
		}

		FileListEntry *entry = list.head->next; // Ignore ..

		int i;
		for (i = 0; i < list.length - 1; i++) {
			char *src_path = malloc(strlen(src) + strlen(entry->name) + 2);
			snprintf(src_path, MAX_PATH_LENGTH, "%s%s", src, entry->name);

			char *dst_path = malloc(strlen(dst) + strlen(entry->name) + 2);
			snprintf(dst_path, MAX_PATH_LENGTH, "%s%s", dst, entry->name);

			int ret = extractRARArchivePath(src_path, dst_path, param);

			free(dst_path);
			free(src_path);

			if (ret <= 0) {
				fileListEmpty(&list);
				return ret;
			}

			entry = entry->next;
		}

		fileListEmpty(&list);
	} else {
		char filepath[filePathLength + sizeof(char)];
		strncpy(filepath,src,filePathLength);
		filepath[filePathLength] = '\0';

		SceUID fddst = sceIoOpen(dst, SCE_O_WRONLY | SCE_O_CREAT | SCE_O_TRUNC, 0777);

		struct ExtractHeader header = {file:fddst,offset:0,memPtr:NULL,bufferSize:0,param:param};

		int result = extractRAR(filepath,src + (filePathLength + sizeof(char)),&header);
		sceIoClose(fddst);

		return result;
	}

	return 1;
}

int archiveRARClose() {
	filePathLength = -1;
	if (!root) {
		closeRARFile(root);
		root = NULL;
	}
	return 1;
}