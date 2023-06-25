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
#include <psp2/vshbridge.h>
#include "sha256.h"
#include "main.h"
#include "pbp.h"
#include "sfo.h"

int read_content_id_data_psp(SceUID pbp_fd, char* content_id) {
  DataPspHeader data_psp_header;

  // read data.psp header
  int read_sz = sceIoRead(pbp_fd, &data_psp_header, sizeof(DataPspHeader));
  if(read_sz < sizeof(DataPspHeader)) return 0;

  // copy the content id from data.psp header to this content_id buffer
  strncpy(content_id, data_psp_header.content_id, 0x30);
  
  return (strlen(content_id) == 36);
}

int read_content_id_npumdimg(SceUID pbp_fd, char* content_id) {
  NpUmdImgHeader npumdimg_header;
  
  // read npumd header
  int read_sz = sceIoRead(pbp_fd, &npumdimg_header, sizeof(NpUmdImgHeader));
  if(read_sz < sizeof(NpUmdImgHeader)) return 0;
  
  // copy the content id from npumdimg_header to this content_id buffer
  strncpy(content_id, npumdimg_header.content_id, 0x30);
  
  return (strlen(content_id) == 36);
}

int is_psx_signed(SceUID pbp_fd, int data_psar_offset, int pbp_type) { // check psx eboot is signed.
  char pgd_magic[0x4];                                                 // this is to filter out psx2psp eboots, and other such tools.
                                                                       // which could never work from the vita's LiveSpace.
  // seek to iso header
  if(pbp_type == PBP_TYPE_PSISOIMG)
    sceIoLseek(pbp_fd, data_psar_offset+0x400, SCE_SEEK_SET);
  else          
    sceIoLseek(pbp_fd, data_psar_offset+0x200, SCE_SEEK_SET);

  // read magic
  int read_len = sceIoRead(pbp_fd, pgd_magic, sizeof(pgd_magic));
  if(read_len < sizeof(pgd_magic))
	  return PBP_TYPE_UNKNOWN;
  
  // if is not PGD, then it is not a signed PSX PBP.
  if(memcmp(pgd_magic, "\0PGD", sizeof(pgd_magic)) == 0)
    return pbp_type;
  else
    return PBP_TYPE_UNKNOWN;
}

int determine_pbp_type(SceUID pbp_fd, PbpHeader* pbp_header) {
  char data_psar_magic[0x8];
  
  int read_sz = sceIoRead(pbp_fd, pbp_header, sizeof(PbpHeader));
  if(read_sz < sizeof(PbpHeader)) return 0;
  
  // seek to data.psar
  sceIoLseek(pbp_fd, pbp_header->data_psar_ptr, SCE_SEEK_SET);
  
  // read magic value to determine pbp type
  read_sz = sceIoRead(pbp_fd, data_psar_magic, sizeof(data_psar_magic));
  if(read_sz < sizeof(data_psar_magic)) return 0;
  
  if(memcmp(data_psar_magic, "NPUMDIMG", 0x8) == 0) { // psp
    return PBP_TYPE_NPUMDIMG;
  }
  else if(memcmp(data_psar_magic, "PSISOIMG", 0x8) == 0) { // ps1 single disc
    return is_psx_signed(pbp_fd, pbp_header->data_psar_ptr, PBP_TYPE_PSISOIMG);
  }
  else if(memcmp(data_psar_magic, "PSTITLEI", 0x8) == 0) { // ps1 multi disc
    return is_psx_signed(pbp_fd, pbp_header->data_psar_ptr, PBP_TYPE_PSTITLEIMG);
  }
  else{ // update package, homebrew, etc, 
    return PBP_TYPE_UNKNOWN; 
  }
  
  if(read_sz < sizeof(PbpHeader)) return PBP_TYPE_UNKNOWN;
}

int read_data_psar_header(SceUID pbp_fd, char* content_id) {
  PbpHeader pbp_header;
  int pbp_type = determine_pbp_type(pbp_fd, &pbp_header);
  if(pbp_type == PBP_TYPE_NPUMDIMG) {
    // seek to start of npumdimg
    sceIoLseek(pbp_fd, pbp_header.data_psar_ptr, SCE_SEEK_SET);
    
    // read content_id from npumdimg
    return read_content_id_npumdimg(pbp_fd, content_id);
  }
  else if(pbp_type == PBP_TYPE_PSISOIMG || pbp_type == PBP_TYPE_PSTITLEIMG) {
    // seek to start of data.psp
    sceIoLseek(pbp_fd, pbp_header.data_psp_ptr, SCE_SEEK_SET);

    // read content_id from data.psp
    return read_content_id_data_psp(pbp_fd, content_id);	  
  }
  else {
    return 0; 
  }  
}

void get_sce_discinfo_sig(char* sce_discinfo, char* disc_id) {
  memset(sce_discinfo, 0x00, 0x100);
  SceUID discinfo_fd = sceIoOpen("vs0:app/NPXS10028/__sce_discinfo", SCE_O_RDONLY, 0);

  if(discinfo_fd < 0)
    return;

  int read_size = 0;
  do{
    read_size = sceIoRead(discinfo_fd, sce_discinfo, 0x100);
    if(strncmp(sce_discinfo, disc_id, 0x9) == 0) {
	  break;
    }
  } while(read_size >= 0x100);
  
  sceIoClose(discinfo_fd);
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
  sceIoLseek(pbp_fd, pbp_header.param_sfo_ptr, SCE_SEEK_SET);
  
  // read the param.sfo file
  read_sz = sceIoRead(pbp_fd, *param_sfo_buffer,  param_sfo_size);
  if(read_sz < param_sfo_size) {
     free(*param_sfo_buffer);
     *param_sfo_buffer = NULL;
     return 0;
  }
  
  return param_sfo_size;
}

int hash_pbp(SceUID pbp_fd, unsigned char* out_hash) {
  unsigned char wbuf[0x7c0]; 
  
  // seek to the start of the eboot.pbp
  sceIoLseek(pbp_fd, 0x00, SCE_SEEK_SET);
  
  // inital read
  int read_sz = sceIoRead(pbp_fd, wbuf, sizeof(wbuf));
  if(read_sz < sizeof(PbpHeader)) return read_sz;
  
  // calculate data hash size
  size_t hash_sz = (((PbpHeader*)wbuf)->data_psar_ptr + 0x1C0000);

  // initalize hash
  SHA256_CTX ctx;
  sha256_init(&ctx);
  
  // first hash
  sha256_update(&ctx, wbuf, read_sz);
  size_t total_hashed = read_sz;  
  
  do {
    read_sz = sceIoRead(pbp_fd, wbuf, sizeof(wbuf));
    
    if((total_hashed + read_sz) > hash_sz)
      read_sz = (hash_sz - total_hashed); // calculate remaining 
    
    sha256_update(&ctx, wbuf, read_sz);
    total_hashed += read_sz;
    
    if(read_sz < sizeof(wbuf)) // treat EOF as complete
      total_hashed = hash_sz;
    
  } while(total_hashed < hash_sz);
  
  sha256_final(&ctx, out_hash);
  
  return 1;
}


int get_pbp_sfo(const char* pbp_file, void** param_sfo_buffer) {
  PbpHeader pbp_header;
  
  if(param_sfo_buffer == NULL) return 0;
  *param_sfo_buffer = NULL;
  
  int res = 0;
  
  if(pbp_file != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0);
    if(pbp_fd < 0) return 0;
    
    // read param.sfo from pbp
    res = read_sfo(pbp_fd, param_sfo_buffer);
    
    sceIoClose(pbp_fd);
  }
  
  return res;
}

int get_pbp_type(const char* pbp_file) {
  int res = 0;
  PbpHeader pbp_header;
  if(pbp_file != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0);
    if(pbp_fd < 0)
      return 0;
    
    res = determine_pbp_type(pbp_fd, &pbp_header);
    
    sceIoClose(pbp_fd);
  }
  
  return res;
}

int get_pbp_content_id(const char* pbp_file, char* content_id) {  
  int res = 0;
  
  if(pbp_file != NULL && content_id != NULL) {
    SceUID pbp_fd = sceIoOpen(pbp_file, SCE_O_RDONLY, 0);
    if(pbp_fd < 0)
      return 0;
    res = read_data_psar_header(pbp_fd, content_id);
    
    // check the content id is valid
    if(res) {
      int content_id_len = strnlen(content_id, 0x30);
      if(content_id_len != 0x24) res = 0;
    }
   
    sceIoClose(pbp_fd);
  }
  
  return res;
  
}

int gen_sce_ebootpbp(const char* psp_game_folder, char* disc_id) {
  int res = 0;
  
  unsigned char pbp_hash[0x20];
  char sce_ebootpbp[0x200];
  char sce_discinfo[0x100];
  
  int sw_version = 0;
  PbpHeader pbp_header;
  
  if(psp_game_folder != NULL) {
    char ebootpbp_path[MAX_PATH_LENGTH];  
    char sce_ebootpbp_path[MAX_PATH_LENGTH];
  
    snprintf(ebootpbp_path,     MAX_PATH_LENGTH, "%s/EBOOT.PBP",      psp_game_folder);
    snprintf(sce_ebootpbp_path, MAX_PATH_LENGTH, "%s/__sce_ebootpbp", psp_game_folder); 

    memset(pbp_hash, 0x00, sizeof(pbp_hash));
    memset(sce_ebootpbp, 0x00, sizeof(pbp_hash));
 
    SceUID pbp_fd = sceIoOpen(ebootpbp_path, SCE_O_RDONLY, 0);
    
    if(pbp_fd < 0)
      return res;
    
    int pbp_type = determine_pbp_type(pbp_fd, &pbp_header); // determine pbp header

    if(pbp_type == PBP_TYPE_PSISOIMG || pbp_type == PBP_TYPE_NPUMDIMG)
      res = hash_pbp(pbp_fd, pbp_hash); // hash eboot.pbp
    if(pbp_type == PBP_TYPE_PSTITLEIMG)
      get_sce_discinfo_sig(sce_discinfo, disc_id); // read sce_discinfo

    sceIoClose(pbp_fd);

    if(pbp_type == PBP_TYPE_UNKNOWN)
      return res;
  
    
    // actually generate the __sce_ebootpbp 
    if(pbp_type == PBP_TYPE_NPUMDIMG)
      res = _vshNpDrmEbootSigGenPsp(ebootpbp_path, pbp_hash, sce_ebootpbp, &sw_version);
    else if(pbp_type == PBP_TYPE_PSISOIMG)                              
      res = _vshNpDrmEbootSigGenPs1(ebootpbp_path, pbp_hash, sce_ebootpbp, &sw_version);
    else if(pbp_type == PBP_TYPE_PSTITLEIMG)
      res = _vshNpDrmEbootSigGenMultiDisc(ebootpbp_path, sce_discinfo, sce_ebootpbp, &sw_version);
   
    if(res >= 0) { // write __sce_ebootpbp
      res = WriteFile(sce_ebootpbp_path, sce_ebootpbp, 0x200);
    }
  }
  return res;
}