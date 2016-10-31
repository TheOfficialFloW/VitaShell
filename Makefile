TITLE_ID = VITASHELL
TARGET   = VitaShell
OBJS     = main.o init.o io_process.o makezip.o package_installer.o network_update.o context_menu.o archive.o photo.o \
		   audioplayer.o file.o text.o hex.o sfo.o settings.o property_dialog.o uncommon_dialog.o message_dialog.o ime_dialog.o \
		   config.o theme.o language.o utils.o elf.o sha1.o list_dialog.o UI2.o touch_shell.o \
		   minizip/zip.o minizip/unzip.o minizip/ioapi.o bm.o audio/vita_audio.o audio/player.o audio/id3.o \
		   audio/oggplayer.o audio/mp3player.o audio/mp3xing.o audio/lrcparse.o \
		   libmad/bit.o libmad/decoder.o libmad/fixed.o libmad/frame.o \
		   libmad/huffman.o libmad/layer12.o libmad/layer3.o  \
		   libmad/stream.o libmad/synth.o libmad/timer.o

RESOURCES = resources
RESOURCES_PNG := $(foreach dir,$(RESOURCES), $(wildcard $(dir)/*.png))
RESOURCES_TXT := $(foreach dir,$(RESOURCES), $(wildcard $(dir)/*.txt))
RESOURCES_BIN := $(foreach dir,$(RESOURCES), $(wildcard $(dir)/*.bin))

OBJS += $(RESOURCES_PNG:.png=.o) $(RESOURCES_TXT:.txt=.o) $(RESOURCES_BIN:.bin=.o)

LIBS = -lvorbisfile -lvorbis -logg -lftpvita -lvita2d -lpng -ljpeg -lz -lm -lc -lonig \
	   -lSceAppMgr_stub -lSceAppUtil_stub -lSceCommonDialog_stub \
	   -lSceCtrl_stub -lSceDisplay_stub -lSceGxm_stub -lSceIme_stub \
	   -lSceHttp_stub -lSceKernel_stub -lSceMusicExport_stub -lSceNet_stub -lSceNetCtl_stub \
	   -lSceSsl_stub -lSceSysmodule_stub -lScePhotoExport_stub -lScePower_stub \
	   -lScePgf_stub libpromoter/libScePromoterUtil_stub.a \
	   -lSceAudio_stub -lSceAudiodec_stub -lSceTouch_stub

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS   = -Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS = $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions
ASFLAGS  = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -d PARENTAL_LEVEL=1 -s APP_VER=01.41 -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
	vita-pack-vpk -s param.sfo -b eboot.bin \
		--add pkg/sce_sys/icon0.png=sce_sys/icon0.png \
		--add pkg/sce_sys/livearea/contents/bg.png=sce_sys/livearea/contents/bg.png \
		--add pkg/sce_sys/livearea/contents/startup.png=sce_sys/livearea/contents/startup.png \
		--add pkg/sce_sys/livearea/contents/template.xml=sce_sys/livearea/contents/template.xml \
	$(TARGET).vpk
	
eboot.bin: $(TARGET).velf
	vita-make-fself $< $@

%.velf: %.elf
	vita-elf-create $< $@ libpromoter/promoterutil.json

$(TARGET).elf: $(OBJS)
	$(CC) $(CFLAGS) $^ $(LIBS) -o $@

%.o: %.png
	$(PREFIX)-ld -r -b binary -o $@ $^
%.o: %.txt
	$(PREFIX)-ld -r -b binary -o $@ $^
%.o: %.bin
	$(PREFIX)-ld -r -b binary -o $@ $^

clean:
	@rm -rf $(TARGET).vpk $(TARGET).velf $(TARGET).elf $(OBJS) \
		eboot.bin param.sfo

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."
