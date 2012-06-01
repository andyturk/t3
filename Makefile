NAME = t

TRIPLE = arm-none-eabi
AR = $(TRIPLE)-ar
AS = $(TRIPLE)-as
CC = $(TRIPLE)-gcc
LD = $(TRIPLE)-ld
NM = $(TRIPLE)-nm
OBJCOPY = $(TRIPLE)-objcopy
RANLIB = $(TRIPLE)-ranlib
OBJDUMP = $(TRIPLE)-objdump

ARMLIBS = /usr/local/arm-gcc/lib
TI_CMSIS = ../TI-CMSIS-Devices-8636/TI/LM3S
ARM_CMSIS = /Users/andy/Downloads/CMSIS_V3P00/CMSIS
QPCPP = ../qpcpp
STELLARISWARE = ../StellarisWare

C_SOURCES = $(wildcard *.c)
CC_SOURCES = $(wildcard *.cc) 
OBJECTS = $(notdir $(C_SOURCES:.c=.o) $(CC_SOURCES:.cc=.o))

STELLARIS = ../StellarisWare

CFLAGS  = -I. -I/usr/local/arm-gcc/include
CFLAGS += -I$(TI_CMSIS)/Include -I$(ARM_CMSIS)/Include -I$(STELLARIS)/inc
CFLAGS += -I$(QPCPP)/include -I$(QPCPP)/ports/arm-cortex/vanilla/gnu
CFLAGS += -I$(STELLARISWARE)
CFLAGS += -mcpu=cortex-m3 -mthumb -g -Dgcc -DPART_LM3S9D96 -fms-extensions

LDFLAGS = -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -T lm3s9d96.ld

vpath %.c . $(TI_CMSIS)/Source

default : $(NAME).bin

clean :
	rm -rf *.o *.bin *.elf *.asm *.a *.pp

%.pp : %.c
	$(CC) -c $(CFLAGS) -std=c99 -E $< -o $@

%.pp : %.cc
	$(CC) -c $(CFLAGS)  -E $< -o $@

%.o : %.c
	$(CC) -c $(CFLAGS) -std=c99 $< -o $@

%.o : %.cc
	$(CC) -c $(CFLAGS) -fno-rtti -fno-exceptions $< -o $@

%.o : %.S
	$(AS) $< -o $@

$(NAME).bin : $(NAME).elf
	$(OBJCOPY) -Obinary $< $@

$(NAME).elf : $(OBJECTS) lm3s9d96.ld 
	$(CC) $(LDFLAGS) -o $(NAME).elf $(OBJECTS) -L$(ARMLIBS) -L. -lstellaris
	$(OBJDUMP) -d $(NAME).elf >$(NAME).asm
