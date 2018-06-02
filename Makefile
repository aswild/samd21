TARGET = blinky

TARGET_OBJS = blinky.o

OBJDIR      = obj
COREDIR     = SparkFun-SAMD-core
CMSIS_DIR   = CMSIS
SAM_DIR     = CMSIS-Atmel

TOOLCHAIN_BIN ?=
CC      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
CXX     = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size

COMPORT      = /dev/ttyACM0
BOSSAC       = bossac
BOSSAC_FLAGS = --erase --write --verify --reset --port=$(COMPORT)
RESET_SCRIPT = bin/ard-reset-arduino --zero $(COMPORT)

USER_CFLAGS     := $(CFLAGS)
USER_CXXFLAGS   := $(CXXFLAGS)
USER_ASFLAGS    := $(ASFLAGS)
USER_LDFLAGS    := $(LDFLAGS)
USER_LIBS       := $(LIBS)

CPPFLAGS    = -MMD -D__SAMD21G18A__ -DF_CPU=48000000L -DUSBCON -DMD
CPPFLAGS   += -DARDUINO=185 -DARDUINO_ARCH_SAMD
CPPFLAGS   += -DUSB_PRODUCT='"SFE SAMD21"' -DUSB_MANUFACTURER='"SparkFun"' -DUSB_VID=0x1B4F -DUSB_PID=0x8D21
CPPFLAGS   += -I$(COREDIR) -I$(COREDIR)/variant -I$(CMSIS_DIR)/Include -I$(SAM_DIR)

# used everywhere
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb
OPTFLAGS    = -Os

# common for CFLAGS and CXXFLAGS
CCXXFLAGS   = $(CPUFLAGS) $(OPTFLAGS) -Wall -Wextra -Wno-expansion-to-defined
CCXXFLAGS  += -fno-exceptions -ffunction-sections -fdata-sections -fno-devirtualize

CFLAGS      = $(CCXXFLAGS) -std=gnu11
CFLAGS     += $(USER_CFLAGS)

CXXFLAGS    = $(CCXXFLAGS) -std=gnu++11 -fno-rtti -fno-threadsafe-statics -fpermissive
CXXFLAGS   += $(USER_CXXFLAGS)

ASFLAGS     = $(CCXXFLAGS) -x assembler-with-cpp
ASFLAGS    += $(USER_ASFLAGS)

LDSCRIPT    = $(COREDIR)/variant/linker_scripts/gcc/flash_with_bootloader.ld
LDFLAGS     = $(CPUFLAGS) $(OPTFLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LDFLAGS    += -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LDFLAGS    += -Wl,--warn-common -Wl,--warn-section-align
LDFLAGS    += -Wl,-Map=$(OBJDIR)/$(TARGET).map
LDFLAGS    += -L$(CMSIS_DIR)
LDFLAGS    += $(USER_LDFLAGS)

CORELIB     = $(OBJDIR)/libcore.a
LIBS        = -larm_cortexM0l_math -lm $(CORELIB)
LIBS       += $(USER_LIBS)

CORE_SRC_DIRS = $(COREDIR) $(COREDIR)/USB $(COREDIR)/variant
vpath %.c   $(CORE_SRC_DIRS)
vpath %.cpp $(CORE_SRC_DIRS)
vpath %.S   $(CORE_SRC_DIRS)

CORE_CC_SRC  = $(foreach dir,$(CORE_SRC_DIRS),$(notdir $(wildcard $(dir)/*.c)))
CORE_CXX_SRC = $(foreach dir,$(CORE_SRC_DIRS),$(notdir $(wildcard $(dir)/*.cpp)))
CORE_AS_SRC  = $(foreach dir,$(CORE_SRC_DIRS),$(notdir $(wildcard $(dir)/*.S)))

CORE_OBJS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(CORE_CXX_SRC)) \
            $(patsubst %.c,$(OBJDIR)/%.o,$(CORE_CC_SRC)) \
            $(patsubst %.S,$(OBJDIR)/%.o,$(CORE_AS_SRC))

_TARGET_OBJ = $(patsubst %,$(OBJDIR)/%,$(TARGET_OBJS))
TARGET_ELF  = $(OBJDIR)/$(TARGET).elf
TARGET_BIN  = $(OBJDIR)/$(TARGET).bin
TARGET_HEX  = $(OBJDIR)/$(TARGET).hex

all: $(TARGET_BIN) size
.PHONY: all

size: $(TARGET_ELF) $(TARGET_HEX)
	@echo ''
	@$(SIZE) $^
.PHONY: size

upload: $(TARGET_BIN)
	$(RESET_SCRIPT)
	$(BOSSAC) $(BOSSAC_FLAGS) $<
.PHONY: upload

clean:
	rm -rf $(OBJDIR)
.PHONY: clean

$(TARGET_ELF): $(_TARGET_OBJ) $(CORELIB) $(LDSCRIPT)
	$(CC) $(LDFLAGS) -o $@ $(_TARGET_OBJ) -Wl,--start-group $(CORELIB) $(LIBS) -Wl,--end-group

$(TARGET_BIN): $(TARGET_ELF)
	$(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(OBJCOPY) -O ihex $< $@

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

$(CORELIB): $(CORE_OBJS)
	$(AR) rcs $@ $^

-include $(wildcard $(OBJDIR)/*.d)
