#ifndef ARCHIVERAR_H_INC
#define ARCHIVERAR_H_INC

#include <psp2/types.h>
#include <psp2/io/stat.h>
#include <psp2/rtc.h>

#include "file.h"
#include "unrarlib/unrarlibutils.h"
#include "utils.h"


int fileListGetRARArchiveEntries(FileList *list, char *path, int sort);

int getRARArchivePathInfo(char *path, uint64_t *size, uint32_t *folders, uint32_t *files);
int extractRARArchivePath(char *src, char *dst, FileProcessParam *param);

int archiveRARFileGetstat(const char *file, SceIoStat *stat);
int archiveRARFileOpen(const char *file, int flags, SceMode mode);
int archiveRARFileRead(SceUID fd, void *data, SceSize size);
int archiveRARFileClose(SceUID fd);

int ReadArchiveRARFile(char *file, void *buf, int size);

int archiveRARClose();
int archiveRAROpen(char *file);

#endif // ARCHIVERAR_H_INC
