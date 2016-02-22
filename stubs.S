	.section .text
	.arm

.macro STUB name
.align 4
.global \name
.type \name, %function
\name:
	mvn r0, #0xFFFFFFFF
	bx lr
	nop
.align 4
.endm

STUB _sceIoOpen
STUB _sceIoClose
STUB _sceIoRead
STUB _sceIoWrite
STUB _sceIoLseek
STUB _sceIoLseek32
STUB _sceIoRemove
STUB _sceIoMkdir
STUB _sceIoRmdir
STUB _sceIoRename
STUB _sceIoDopen
STUB _sceIoDread
STUB _sceIoDclose
STUB _sceIoGetstat
STUB _sceIoGetstatByFd
STUB _sceIoChstat