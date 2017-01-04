#include "unrarlibutils.h"

int getcwd() {
	return 0;
}

int fsync() {
	return 0;
}

static	void* rar_open(const char *filename, unsigned int om) {
	struct RAROpenArchiveData arc_open;
	arc_open.ArcName = (char*)filename;
	arc_open.OpenMode = om;
	arc_open.CmtBuf = NULL;
	HANDLE rar_file = RAROpenArchive(&arc_open);
	if (arc_open.OpenResult != 0) {
		return NULL;
	}
	return rar_file;
}

ArchiveFileNode* createNode(const char* name) {
	ArchiveFileNode* node = malloc(sizeof(ArchiveFileNode));
	node->nodeName = malloc((strlen(name) + 2) * sizeof(char));
	strcpy(node->nodeName, name);
	node->childCount = 0;
	node->childPt = NULL;
	node->fatherPt = NULL;
	node->maxCount = 0;
	return node;
}

void add_node(ArchiveFileNode* father,ArchiveFileNode* child) {
	uint32_t count = father->childCount;
	if( (count + 1) > father->maxCount ) {
		if(father->maxCount == 0) {
			father->maxCount = 16;
			father->childPt = malloc(16 * sizeof(void*));
		} else {
			void* n_ptr = realloc((void*)father->childPt,2 * father->maxCount * sizeof(void*));
			if(n_ptr) {
				father->childPt = n_ptr;
				father->maxCount *= 2;
			}
		}
	}
	father->childPt[count] = (void*)child;
	father->childCount++;

	child->fatherPt = father;
}

ArchiveFileNode* getChildNodeByName(ArchiveFileNode* node,char* name) {
	if(node->childCount <= 0 )
		return NULL;
	uint32_t i;
	for(i = 0; i < node->childCount; i++) {
		ArchiveFileNode* _n = (ArchiveFileNode*)(node->childPt[i]);
		if(strcmp(_n->nodeName,name) == 0) {
			return _n;
		}
	}
	return NULL;
}

void free_node(ArchiveFileNode* node) {
	uint32_t count = node->childCount;
	int i = 0;

	for(; i < count; i++) {
		ArchiveFileNode* _n = (ArchiveFileNode*)(node->childPt[i]);
		free_node(_n);
	}
	if(node->childPt != NULL) {
		free(node->childPt);
	}
	if(node->nodeName != NULL) {
		free(node->nodeName);
	}
	free(node);
}

void parse_path(const char* name, struct filelayer* layer) {
	char* p;
	char _name[RARMAX_FILENAME];
	strcpy(_name, name);
	uint32_t depth = 0;
	while(1) {
		p = strrchr(_name, '/');
		if(!p)
			break;
		strcpy(&(layer->name[depth][0]), p + sizeof(char));
		layer->depth = ++depth;
		*p = '\0';
	}
	strcpy(&(layer->name[depth][0]), _name);
}

ArchiveFileNode* openRARFile(char* filename) {
	ArchiveFileNode* root = createNode("root");
	HANDLE RARFile = rar_open(filename,RAR_OM_LIST);
	if(!RARFile) {
		free_node(root);
		return NULL;
	}

	struct RARHeaderData data;
	while (!RARReadHeader(RARFile, &data)) {
		struct filelayer layer = {0};
		parse_path(data.FileName,&layer);
		ArchiveFileNode* _up = root;
		int i = layer.depth;
		for(; i > 0; i--) {
			ArchiveFileNode* father = getChildNodeByName(_up, &(layer.name[i][0]));
			if(!father) {
				father = createNode(&(layer.name[i][0]));
				add_node(_up,father);
			}
			_up = father;
		}
		ArchiveFileNode* filenode = getChildNodeByName(_up,&(layer.name[0][0]));

		if(!filenode) {
			filenode = createNode(&(layer.name[0][0]));
			add_node(_up, filenode);
		}

		filenode->data = data;

		RARProcessFile(RARFile, RAR_SKIP,NULL,NULL,false);
	}

	RARCloseArchive(RARFile);
	return root;
}

int extractToMem(struct ExtractHeader* header,char* extractBuffer,uint64_t bufferSize) {
	if(header->offset >= header->bufferSize)
		return -1;

	if((header->offset + bufferSize) > header->bufferSize)
		bufferSize = (header->bufferSize - header->offset);//no enough memory

	memcpy((header->memPtr + header->offset),extractBuffer,bufferSize);
	header->offset += bufferSize;
	return 1;
}

int extractToFile(struct ExtractHeader* header,char* extractBuffer,uint64_t bufferSize) {
	if(!header->file)
		return -1;

	FileProcessParam* param = header->param;
	if (param) {
		if (param->value)
			(*param->value) += bufferSize;

		if (param->SetProgress)
			param->SetProgress(param->value ? *param->value : 0, param->max);

		if (param->cancelHandler && param->cancelHandler()) {
			return -1;
		}
	}

	sceIoLseek(header->file,header->offset,SCE_SEEK_SET);
	sceIoWrite(header->file,extractBuffer,bufferSize);
	header->offset += bufferSize;


	return 1;
}

static int CALLBACK rarCallback(UINT msg,LPARAM UserData,LPARAM P1,LPARAM P2) {
	struct ExtractHeader* header;
	int rtn = 1;
	switch(msg) {
	case UCM_CHANGEVOLUME:
		if(P2 != RAR_VOL_NOTIFY) {
			rtn = -1;
		}
		break;
	case UCM_PROCESSDATA:
		header = (struct ExtractHeader*)UserData;
		if(header->memPtr)
			extractToMem(header,(char*)P1,P2);
		else
			rtn = extractToFile(header,(char*)P1,P2);
		break;
	default:
		rtn = -1;
		break;
	}
	return rtn;
}

int extractRAR(char* path,char* filename,struct ExtractHeader* header) {

	HANDLE RARFile = rar_open(path,RAR_OM_EXTRACT);
	if(!RARFile)
		return -1;

	struct RARHeaderData data;
	int rtn = -1;
	while (!RARReadHeader(RARFile, &data)) {
		if(!(strcmp(data.FileName,filename))) {
			rtn = 1;
			break;
		}
		RARProcessFile(RARFile, RAR_SKIP,NULL,NULL,false);
	}

	if(rtn > 0) {
		RARSetCallback(RARFile,rarCallback,(LPARAM)header);
		int result = RARProcessFile(RARFile, RAR_TEST,NULL,NULL,false);
		if(result >= 0)
			rtn = 1;
		else if(result == -2)
			rtn = 0;
	}

	RARCloseArchive(RARFile);
	return rtn;
}

ArchiveFileNode* getFileNodeFromFilePath(ArchiveFileNode* root,const char* filepath) {
	struct filelayer layer = {0};
	parse_path(filepath,&layer);
	int i = layer.depth;
	ArchiveFileNode* _up = root;

	for(; i > 0; i--) {
		_up = getChildNodeByName(_up, &(layer.name[i][0]));
		if(!_up)
			return NULL;
	}

	return getChildNodeByName(_up,&(layer.name[0][0]));
}

int getRARArchiveNodeInfo(ArchiveFileNode* root, uint64_t *size, uint32_t *folders, uint32_t *files) {
	if(root->childCount > 0) {
		uint32_t count = root->childCount;
		int i = 0;
		for(; i < count; i++) {
			ArchiveFileNode* _n = (ArchiveFileNode*)(root->childPt[i]);
			getRARArchiveNodeInfo(_n,size,folders,files);
		}
	}
	if(root->data.Flags == 0x20) {
		if(folders)
			(*folders)++;
	} else {
		if(size)
			(*size) += root->data.UnpSize;
		if(files)
			(*files) ++;
	}

	return 1;
}

ArchiveFileNode* getFloderNodeFromPath(ArchiveFileNode* root,const char* path) {
	uint32_t pathlen = strlen(path);
	char temp[pathlen];
	strcpy(temp,path);
	temp[pathlen - 1] ='\0';
	return getFileNodeFromFilePath(root,temp);
}


void closeRARFile(ArchiveFileNode* root) {
	free_node(root);
}
