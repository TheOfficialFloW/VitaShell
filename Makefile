TARGET = VitaShell
OBJS = main.o init.o homebrew.o io_process.o archive.o photo.o file.o text.o \
	hex.o ftp.o message_dialog.o ime_dialog.o language.o utils.o module.o \
	psp/pboot.o psp/libkirk/kirk_engine.o psp/libkirk/crypto.o \
	psp/libkirk/amctrl.o psp/libkirk/bn.o psp/libkirk/ec.o

FEXDIRS    = fex fex/7z_C fex/fex fex/unrar
FEXCSRCS   = $(foreach dir, $(FEXDIRS), $(wildcard $(dir)/*.c))
FEXCPPSRCS = $(foreach dir, $(FEXDIRS), $(wildcard $(dir)/*.cpp))
OBJS += $(FEXCSRCS:.c=.o) $(FEXCPPSRCS:.cpp=.o)

RESOURCES_PNG = resources/battery.png resources/battery_bar_green.png resources/battery_bar_red.png
RESOURCES_TXT = resources/english_us_translation.txt
OBJS += $(RESOURCES_PNG:.png=.o) $(RESOURCES_TXT:.txt=.o)

#stubs.o include this crashes fex!!!

LIBS =  -lvita2d -lpng -ljpeg -lz -lm -lc \
	-lSceAppMgr_stub -lSceAppUtil_stub -lSceAudio_stub -lSceCommonDialog_stub \
	-lSceCtrl_stub -lSceDisplay_stub -lSceGxm_stub -lSceIme_stub \
	-lSceKernel_stub -lSceMusicExport_stub -lSceNet_stub -lSceNetCtl_stub \
	-lSceSysmodule_stub -lScePower_stub -lSceTouch_stub -lScePgf_stub \
	-lScePvf_stub -lUVLoader_stub

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS   = -Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable \
	$(foreach dir, $(FEXDIRS), -I$(dir))
CXXFLAGS = $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions
ASFLAGS  = $(CFLAGS)

all: $(TARGET).velf

%.velf: %.elf
	$(PREFIX)-strip -g $<
	vita-elf-create $< $@ > null

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

%.o: %.png
	$(PREFIX)-objcopy -I binary -O elf32-littlearm -B arm $< $@
%.o: %.txt
	$(PREFIX)-objcopy -I binary -O elf32-littlearm -B arm $< $@

clean:
	@rm -rf $(TARGET).velf $(TARGET).elf $(OBJS)

copy: $(TARGET).velf
	@cp $(TARGET).velf ../Rejuvenate/$(TARGET).velf
	@echo "Copied!"
