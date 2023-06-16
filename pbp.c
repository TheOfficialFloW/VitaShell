/*
  VitaShell
  Copyright (C) 2015-2018, TheFloW

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
#include "pbp.h"
#include "sfo.h"

int read_content_id_data_psp(SceUID pbp_fd, char* content_id) {
  DataPspHeader data_psp_header;

  // read data.psp header
  int read_sz = sceIoRead(pbp_fd, &data_psp_header, sizeof(DataPspHeader));
  if(read_sz < sizeof(DataPspHeader)) return 0;

  // copy the content id from data.psp header to this content_id buffer
  strncpy(content_id, data_psp_header.content_id, sizeof(npumdimg_header.content_id));
  
  return 0;
}

int read_content_id_npumdimg(SceUID pbp_fd, char* content_id) {
  NpUmdImgHeader npumdimg_header;
  
  // read npumd header
  int read_sz = sceIoRead(pbp_fd, &npumdimg_header, sizeof(NpUmdImgHeader));
  if(read_sz < sizeof(NpUmdImgHeader)) return 0;
  
  // copy the content id from npumdimg_header to this content_id buffer
  strncpy(content_id, npumdimg_header.content_id, sizeof(npumdimg_header.content_id));
  
  return 1;
}

int read_data_psar_header(SceUID pbp_fd, char* content_id) {
  PbpHeader pbp_header;
  char data_psar_magic[0x8];
  
  int read_sz = sceIoRead(pbp_fd, &pbp_header, sizeof(PbpHeader));
  if(read_sz < sizeof(PbpHeader)) return 0;
  
  // seek to data.psar
  sceIoLseek(pbp_fd, pbp_header.data_psar_ptr, SCE_SEEK_SET);
  
  // read magic value to determine pbp type
  read_sz = sceIoRead(pbp_fd, data_psar_magic, sizeof(data_psar_magic));
  if(read_sz < sizeof(data_psar_magic)) return 0;
  
  if(memcmp(data_psar_magic, "NPUMDIMG") == 0) { // psp
    // seek to start of npumdimg
    sceIoLseek(pbp_fd, pbp_header.data_psar_ptr, SCE_SEEK_SET);
    
    // read content_id from npumdimg
    return read_content_id_data_psp(pbp_fd, &pbp_header, content_id);
  }
  else if(memcmp(data_psar_magic, "PSISOIMG") == 0 || memcmp(data_psar_magic, "PSTITLEI") == 0) { // ps1
    sceIoLseek(pbp_fd, pbp_header.data_psp_ptr, SCE_SEEK_SET);
  }
  else{ // update package, homebrew, etc, 
    return 0; 
  }
  
  if(read_sz < sizeof(PbpHeader)) return 0;
}

int read_sfo(SceUID pbp_fd, void** param_sfo_buffer){
  PbpHeader pbp_header;
  // read pbp header
  int read_sz = sceIoRead(pbp_fd, &pbp_header, sizeof(PbpHeader));
  if(read_sz < sizeof(PbpHeader)) return 0;

  // get sfo size
  int param_sfo_size = pbp_header.icon0_png_ptr - pbp_header.param_sfo_ptr;
  if(param_sfo_size <= 0) return 0;
  
  // allocate a buffer for the param.sfo file
  *param_sfo_buffer = malloc(param_sfo_size);
  if(*param_sfo_buffer == NULL) return 0;
  
  // seek to the start of param.sfo
  sceIoLseek(pbp_header.param_sfo_ptr, SCE_SEEK_SET);
  
  // read the param.sfo file
  read_sz = sceIoRead(pbp_fd, *param_sfo_buffer,  param_sfo_size);
  if(read_sz < param_sfo_size) {
	  free(*param_sfo_buffer);
	  *param_sfo_buffer = NULL;
	  return 0;
  }
  
  return param_sfo_size;
}

int get_pbp_sfo(const char pbp_file, void** param_sfo_buffer) {
  PbpHeader pbp_header;
  
  if(param_sfo_buffer == NULL) return;
  *param_sfo_buffer = NULL;
  
  int res = 0;
  
  if(pbp_file != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file);
    if(pbp_fd < 0) return NULL;
    
    // read param.sfo from pbp
    res = read_sfo(pbp_fd, param_sfo_buffer);
    
    sceIoClose(pbp_fd);	
  }
  
  return res;
}

int get_pbp_content_id(const char* pbp_file, char* content_id) {  
  int res = 0;
  
  if(pbp_file != NULL && content_id != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file);
    if(pbp_fd < 0) return 0;
    res = read_content_id(pbp_fd, content_id);
    
    // check the content id is valid
    if(res) {
      int content_id_len = strnlen(content_id, 0x30);
      if(content_id_len != 0x24) res = 0;
    }
    
    sceIoClose(pbp_fd);	
  }
  
  return res;
  
}