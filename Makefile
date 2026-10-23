PATH := $(DEVKITARM)/bin:$(PATH)

PREFIX  ?= arm-none-eabi-
CC      := $(PREFIX)gcc
OBJCOPY := $(PREFIX)objcopy

TARGET  := doodle_jump
SRCS    := main.c

CFLAGS  := -mthumb -mthumb-interwork -mcpu=arm7tdmi -O2 -Wall -fomit-frame-pointer
LDFLAGS := -mthumb -mthumb-interwork -specs=gba.specs

all: $(TARGET).gba

$(TARGET).elf: $(SRCS)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@

$(TARGET).gba: $(TARGET).elf
	$(OBJCOPY) -O binary $< $@
	-gbafix $@

clean:
	rm -f $(TARGET).elf $(TARGET).gba
