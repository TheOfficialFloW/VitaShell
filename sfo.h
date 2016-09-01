#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <psp2/io/stat.h>
#include <psp2/io/fcntl.h>
#include <psp2/kernel/processmgr.h>

// Struct from : http://www.vitadevwiki.com/index.php?title=System_File_Object_(SFO)_(PSF)

typedef struct{
    int magic; //PSF
    int version; //1.1
    int keyTableOffset;
    int dataTableOffset;
    int indexTableEntries;
} sfo_header_t;

typedef struct{
    uint16_t keyOffset; //offset of keytable + keyOffset
    uint16_t param_fmt; //enum (see below)
    uint32_t paramLen;
    uint32_t paramMaxLen;
    uint32_t dataOffset; //offset of datatable + dataOffset
} sfo_index_t;

int SFOReader(char* file);