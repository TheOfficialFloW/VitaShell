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

#include <psp2kern/kernel/modulemgr.h>
#include <taihen.h>

static SceUID hooks[2];

void _start() __attribute__ ((weak, alias("module_start")));
int module_start(SceSize args, void *argp) {
  // Get tai module info
  tai_module_info_t info;
  info.size = sizeof(tai_module_info_t);
  if (taiGetModuleInfoForKernel(KERNEL_PID, "SceAppMgr", &info) < 0)
    return SCE_KERNEL_START_SUCCESS;

  // Patch to allow Memory Card remount
  uint32_t nop_nop_opcode = 0xBF00BF00;
  switch (info.module_nid) {
    case 0x94CEFE4B: // 3.55 retail
    case 0xDFBC288C: // 3.57 retail
    case 0xDBB29DB7: // 3.60 retail
    case 0x1C9879D6: // 3.65 retail
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB338, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB368, &nop_nop_opcode, 2);
      break;
      
    case 0x54E2E984: // 3.67 retail
    case 0xC3C538DE: // 3.68 retail
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB344, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB374, &nop_nop_opcode, 2);
      break;
      
    case 0x321E4852: // 3.69 retail
    case 0x700DA0CD: // 3.70 retail
    case 0xF7846B4E: // 3.71 retail
    case 0xA8E80BA8: // 3.72 retail
    case 0xB299D195: // 3.73 retail
      hooks[0] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB34C, &nop_nop_opcode, 4);
      hooks[1] = taiInjectDataForKernel(KERNEL_PID, info.modid, 0, 0xB37C, &nop_nop_opcode, 2);
      break;
  }

  return SCE_KERNEL_START_SUCCESS;
}

int module_stop(SceSize args, void *argp) {
  if (hooks[1] >= 0)
    taiInjectReleaseForKernel(hooks[1]);

  if (hooks[0] >= 0)
    taiInjectReleaseForKernel(hooks[0]);

  return SCE_KERNEL_STOP_SUCCESS;
}
