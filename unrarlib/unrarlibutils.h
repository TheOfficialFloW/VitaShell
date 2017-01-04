#ifndef UNRARLIBUTILS_H_INC
#define UNRARLIBUTILS_H_INC

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>

#include "../file.h"

#define _UNIX
#include "unrar_c_wrapper.h"

#define RARMAX_HIERARCHY 32//max hierarchy for folder
#define RARMAX_FILENAME 262


typedef struct {
	char* nodeName;
	void* fatherPt;
	void** childPt;
	uint32_t childCount;
	uint32_t maxCount;
	struct RARHeaderData data;
} ArchiveFileNode;

struct filelayer {
	uint32_t depth;
	char name[RARMAX_HIERARCHY][RARMAX_FILENAME];
};

struct ExtractHeader {
	SceUID file;
	uint64_t offset;
	char* memPtr;
	uint64_t bufferSize;
	FileProcessParam * param;
};


ArchiveFileNode* openRARFile(char* filename);//returns the number of files in the rar document archive
void closeRARFile(ArchiveFileNode* root);
ArchiveFileNode* createNode(const char* name);
ArchiveFileNode* getChildNodeByName(ArchiveFileNode* node,char* name);
int extractRAR(char* path,char* filename,struct ExtractHeader* header);
ArchiveFileNode* getFileNodeFromFilePath(ArchiveFileNode* root, const char* filepath);
int getRARArchiveNodeInfo(ArchiveFileNode* root, uint64_t *size, uint32_t *folders, uint32_t *files);
void free_node(ArchiveFileNode* node);
ArchiveFileNode* getFloderNodeFromPath(ArchiveFileNode* root,const char* path);

#endif // UNRARLIBUTILS_H_INC
