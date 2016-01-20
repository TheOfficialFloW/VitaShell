.arch armv7a

.macro STUB name
.global \name
.type \name, %function
\name:
mvn r0, #0xFFFFFFFF
bx lr
nop
.endm

.section .text
.arm

STUB sceGenSyscall

STUB sceAppMgrLaunchAppByUri
STUB sceAppMgrAppParamGetString
STUB sceAppMgrGetDevInfo
STUB sceAppMgrConvertVs0UserDrivePath
STUB sceAppMgrPspSaveDataRootMount
STUB sceAppMgrMmsMount

STUB _sceSysmoduleUnloadModuleInternalWithArg
STUB _sceSysmoduleLoadModuleInternalWithArg
STUB sceKernelGetMemBlockInfoByRange

STUB sceKernelGetMemBlockInfoByAddr
STUB sceKernelOpenMemBlock
STUB sceKernelCloseMemBlock

STUB sceKernelGetSystemSwVersion
STUB sceKernelGetModelForCDialog