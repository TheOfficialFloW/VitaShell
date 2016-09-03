TITLE_ID = VITASHELL
TARGET   = VitaShell
OBJS     = main.o init.o io_process.o package_installer.o archive.o photo.o file.o text.o hex.o sfo.o \
		   uncommon_dialog.o message_dialog.o ime_dialog.o config.o theme.o language.o utils.o sha1.o \
		   audioplayer.o minizip/unzip.o minizip/ioapi.o

RESOURCES_PNG = resources/folder_icon.png resources/file_icon.png resources/archive_icon.png resources/image_icon.png resources/ftp.png \
				resources/battery.png resources/battery_bar_green.png resources/battery_bar_red.png resources/battery_bar_charge.png \
				resources/headphone.png resources/audio_previous.png resources/audio_pause.png resources/audio_play.png \
				resources/audio_next.png
RESOURCES_TXT = resources/theme.txt resources/colors.txt resources/english_us.txt
OBJS += $(RESOURCES_PNG:.png=.o) $(RESOURCES_TXT:.txt=.o)

LIBS = -lftpvita -lvita2d -lpng -ljpeg -lz -lm -lc \
	   -lSceAppMgr_stub -lSceAppUtil_stub -lSceCommonDialog_stub \
	   -lSceCtrl_stub -lSceDisplay_stub -lSceGxm_stub -lSceIme_stub \
	   -lSceKernel_stub -lSceNet_stub -lSceNetCtl_stub \
	   -lSceSysmodule_stub -lScePower_stub -lScePgf_stub libpromoter/libScePromoterUtil_stub.a \
	   -lSceAudio_stub -lSceAudiodec_stub -lSceTouch_stub

#NETDBG_IP ?= 192.168.1.50

ifdef NETDBG_IP
CFLAGS += -DNETDBG_ENABLE=1 -DNETDBG_IP="\"$(NETDBG_IP)\""
endif
ifdef NETDBG_PORT
CFLAGS += -DNETDBG_PORT=$(NETDBG_PORT)
endif

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
CXX      = $(PREFIX)-g++
CFLAGS   = -Wl,-q -Wall -O3 -Wno-unused-variable -Wno-unused-but-set-variable
CXXFLAGS = $(CFLAGS) -std=c++11 -fno-rtti -fno-exceptions
ASFLAGS  = $(CFLAGS)

all: $(TARGET).vpk

%.vpk: eboot.bin
	vita-mksfoex -d PARENTAL_LEVEL=1 -s APP_VER=00.90 -s TITLE_ID=$(TITLE_ID) "$(TARGET)" param.sfo
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

clean:
	@rm -rf $(TARGET).vpk $(TARGET).velf $(TARGET).elf $(OBJS) \
		eboot.bin param.sfo

vpksend: $(TARGET).vpk
	curl -T $(TARGET).vpk ftp://$(PSVITAIP):1337/ux0:/
	@echo "Sent."

send: eboot.bin
	curl -T eboot.bin ftp://$(PSVITAIP):1337/ux0:/app/$(TITLE_ID)/
	@echo "Sent."
