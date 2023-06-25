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

#ifndef __PBP_H__
#define __PBP_H__


typedef enum PbpType{
  PBP_TYPE_NPUMDIMG   = 0,
  PBP_TYPE_PSISOIMG   = 1,
  PBP_TYPE_PSTITLEIMG = 2,
  PBP_TYPE_UNKNOWN    = 3
} PbpType;

// for PSISOIMG and PSTITLEIMG contents

typedef struct DataPspHeader{
  SceUInt8 magic[0x4];
  SceUInt8 unk[0x7C];
  SceUInt8 unk2[0x32];
  SceUInt8 unk3[0xE];
  SceUInt8 hash[0x14];
  SceUInt8 reserved[0x58];
  SceUInt8 unk4[0x434];
  char content_id[0x30];
} DataPspHeader;

// for NPUMDIMG content
typedef struct NpUmdImgBody {
  SceUInt16 sector_size;   // 0x0800
  SceUInt16 unk_2;      // 0xE000
  SceUInt32 unk_4;
  SceUInt32 unk_8;
  SceUInt32 unk_12;
  SceUInt32 unk_16;
  SceUInt32 lba_start;
  SceUInt32 unk_24;
  SceUInt32 nsectors;
  SceUInt32 unk_32;
  SceUInt32 lba_end;
  SceUInt32 unk_40;
  SceUInt32 block_entry_offset;
  char disc_id[0x10];
  SceUInt32 header_start_offset;
  SceUInt32 unk_68;
  SceUInt8 unk_72;
  SceUInt8 bbmac_param;
  SceUInt8 unk_74;
  SceUInt8 unk_75;
  SceUInt32 unk_76;
  SceUInt32 unk_80;
  SceUInt32 unk_84;
  SceUInt32 unk_88;
  SceUInt32 unk_92;
} NpUmdImgBody;

typedef struct NpUmdImgHeader{
  SceUInt8 magic[0x08];  // "NPUMDIMG"
  SceUInt32 key_index; // usually 2, or 3.
  SceUInt32 block_basis;
  char content_id[0x30];
  NpUmdImgBody body;  
  SceUInt8 header_key[0x10];
  SceUInt8 data_key[0x10];
  SceUInt8 header_hash[0x10];
  SceUInt8 padding[0x8];
  SceUInt8 ecdsa_sig[0x28];
}  NpUmdImgHeader;

// generic eboot.pbp header

typedef struct PbpHeader
{
  char magic[0x4];
  SceUInt32 version;
  SceUInt32 param_sfo_ptr;  
  SceUInt32 icon0_png_ptr;  
  SceUInt32 icon1_pmf_ptr;
  SceUInt32 pic0_png_ptr;
  SceUInt32 pic1_png_ptr;  
  SceUInt32 snd0_at3_ptr;  
  SceUInt32 data_psp_ptr;  
  SceUInt32 data_psar_ptr;
} PbpHeader;

int get_pbp_type(const char* pbp_file);
int get_pbp_sfo(const char* pbp_file, void** param_sfo_buffer);
int get_pbp_content_id(const char* pbp_file, char* content_id);
int gen_sce_ebootpbp(const char* psp_game_folder, char* disc_id);
#endif