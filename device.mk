BUILD = build
OBJ = $(BUILD)/device
BIN = $(OBJ)
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

GENERATED = vector_table.cc bluetooth_init_cc2564.cc
C_SOURCES = $(wildcard *.c) $(wildcard $(BUILD)/*.c)
CC_SOURCES = $(wildcard *.cc) $(wildcard $(BUILD)/*.cc) $(addprefix $(BUILD)/,$(GENERATED))
OBJECTS = $(addprefix $(OBJ)/,$(sort $(notdir $(C_SOURCES:.c=.o) $(CC_SOURCES:.cc=.o))))

STELLARIS = ../StellarisWare

CFLAGS  = -I. -I$(BUILD) -I/usr/local/arm-gcc/include
CFLAGS += -I$(TI_CMSIS)/Include -I$(ARM_CMSIS)/Include -I$(STELLARIS)/inc
CFLAGS += -I$(STELLARISWARE)
CFLAGS += -mcpu=cortex-m3 -mthumb -g -Dgcc -DPART_LM3S9D96 -fms-extensions -Wall
CFLAGS += -DDEBUG=1
CFLAGS += -Wno-pmf-conversions -Wno-psabi -std=gnu++0x

LDFLAGS = -nostdlib -fno-builtin -nostartfiles -nodefaultlibs -T lm3s9d96.ld

vpath %.c . $(BUILD) $(TI_CMSIS)/Source
vpath %.cc . $(BUILD)

default : $(BIN)/$(NAME).bin

clean :
	rm -rf $(BUILD)
	rm -rf *.o *.bin *.elf *.asm *.a *.pp vector_table.cc

.PRECIOUS : %/.sentinel

%/.sentinel :
	mkdir -p $(dir $@)
	touch $@

%.pp : %.c
	$(CC) -c $(CFLAGS) -std=c99 -E $< -o $@

%.pp : %.cc
	$(CC) -c $(CFLAGS)  -E $< -o $@

$(OBJ)/%.o : %.c $(OBJ)/.sentinel
	$(CC) -c $(CFLAGS) -std=c99 $< -o $@

$(OBJ)/%.o : %.cc $(OBJ)/.sentinel
	$(CC) -c $(CFLAGS) -fno-rtti -fno-exceptions $< -o $@

$(BUILD)/%.S : %.cc $(BUILD)/.sentinel
	$(CC) -S -c $(CFLAGS) -fno-rtti -fno-exceptions $< -o $@

$(OBJ)/bluetooth_init_cc2564.o : $(BUILD)/bluetooth_init_cc2564.cc

$(BUILD)/bluetooth_init_cc2564.cc :
	$(MAKE) -f host.mk $@

$(BIN)/.gdbinit : .gdbinit
	cp .gdbinit $(BIN)

$(OBJ)/%.o : %.S $(OBJ)/.sentinel
	$(AS) $< -o $@

$(BUILD)/vector_table.cc : generate_vector_table.py $(BUILD)/.sentinel
	python generate_vector_table.py lm3s9d96 > $(BUILD)/vector_table.cc

$(BIN)/$(NAME).bin : $(BIN)/$(NAME).elf $(BIN)/.sentinel
	$(OBJCOPY) -Obinary $< $@

$(BIN)/$(NAME).elf : $(OBJECTS) lm3s9d96.ld $(BIN)/.gdbinit $(BIN)/.sentinel 
	$(CC) $(LDFLAGS) -o $(BIN)/$(NAME).elf $(OBJECTS) -L$(ARMLIBS) -L. -lstellaris
	$(OBJDUMP) -d $(BIN)/$(NAME).elf >$(BUILD)/$(NAME).asm
