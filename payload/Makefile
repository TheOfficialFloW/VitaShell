TARGET = payload
OBJS = main.o

PREFIX   = arm-vita-eabi
CC       = $(PREFIX)-gcc
LD       = $(PREFIX)-gcc
OBJCOPY  = $(PREFIX)-objcopy
CFLAGS   = -fPIE -fno-zero-initialized-in-bss -std=gnu99 -mcpu=cortex-a9 -mthumb-interwork -mthumb
LDFLAGS  = -T linker.x -nodefaultlibs -nostdlib -pie

all: $(TARGET)

%.o: %.c
	$(CC) -c -o $@ $< $(CFLAGS) $(CFLAGS_THUMB)

$(TARGET): $(OBJS)
	$(LD) -o $@ $^ $(LDFLAGS)
	$(OBJCOPY) -O binary $@ $@.bin

clean:
	rm -rf *~ *.elf *.bin *.s $(TARGET) $(OBJS)
