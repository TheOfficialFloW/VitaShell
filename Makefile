TARGET = VitaShell
OBJS = main.o init.o homebrew.o io_process.o archive.o photo.o file.o text.o hex.o ftp.o message_dialog.o ime_dialog.o language.o utils.o module.o psp/pboot.o psp/libkirk/kirk_engine.o psp/libkirk/crypto.o psp/libkirk/amctrl.o psp/libkirk/bn.o psp/libkirk/ec.o

#stubs.o include this crashes fex!!!

LIBS = -lfex -lvita2d -lpng -ljpeg -lz -lm -lc -lSceAppMgr_stub -lSceAppUtil_stub -lSceAudio_stub -lSceCommonDialog_stub -lSceCtrl_stub -lSceDisplay_stub -lSceGxm_stub -lSceIme_stub -lSceKernel_stub -lSceMusicExport_stub -lSceNet_stub -lSceNetCtl_stub -lSceSysmodule_stub -lScePower_stub -lSceTouch_stub -lScePgf_stub -lScePvf_stub -lUVLoader_stub

PREFIX		= arm-vita-eabi
CC			= $(PREFIX)-gcc
CXX			= $(PREFIX)-g++
CFLAGS  	= -Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS	= $(CFLAGS) -std=c++11
ASFLAGS		= $(CFLAGS)

all: $(TARGET).velf
	make copy

%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)

copy: $(TARGET).velf
	@cp $(TARGET).velf ../Rejuvenate/$(TARGET).velf
	@echo "Copied!"