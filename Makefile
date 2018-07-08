# Application Configuration
TARGET = neostrip-demo
TARGET_OBJS = sketch.o

# Directory Configuration
OBJDIR      = obj
CORE        = core

# all these directories will be used as CPP include paths, and
# all c/cpp/S sources will be compiled into libcore
LIBRARIES   = variant $(CORE) $(CORE)/USB
LIBRARIES  += Adafruit_ZeroDMA DigitalIO MPR121 Neostrip SPI Wire

CORESRCDIRS = $(addprefix lib/,$(LIBRARIES))
COREINCS    = $(addprefix -I,$(CORESRCDIRS))

VARIANT_DIR = lib/variant
CMSIS_DIR   = lib/CMSIS
SAM_DIR     = lib/CMSIS-Atmel

COMPORT     ?= /dev/ttyACM0
BOSSAC      ?= bossac
BOSSAC_FLAGS = --erase --write --verify --reset --port=$(COMPORT)
RESET_SCRIPT = bin/ard-reset-arduino --zero $(COMPORT)

# my bossa-git AUR package sets the version to the Arch pkgver, which is 1.8.rXX.gYYYYYYY since
# there hasn't been a v1.9 tag yet, so figure out the version by checking for the availability
# of the --arduino-erase option
BOSSA_19 := $(shell $(BOSSAC) --help 2>/dev/null | grep -q -e '--arduino-erase' && echo y || echo n)
ifeq ($(BOSSA_19),y)
# we have auto-erase available
BOSSAC_FLAGS := $(filter-out --erase,$(BOSSAC_FLAGS))
# use bossac to do the reset when possible.
BOSSAC_FLAGS += --arduino-erase
# BOSSA v1.8 hard-coded the flash starting address as 0x2000, so the command-line offset
# must be zero (the default) or else the program would get written to 0x4000.
# BOSSA v1.9 doesn't do that, so we must set the offset to 0x2000 or else the bootloader
# will get overwritten, bricking the board.
BOSSAC_FLAGS += --offset=0x2000
endif

# Tools Configuration
TOOLCHAIN_BIN ?=
CC      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
CXX     = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AS      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc -x assembler-with-cpp
CCLD    = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size
GDB     = $(TOOLCHAIN_BIN)arm-none-eabi-gdb

USER_CFLAGS     := $(CFLAGS)
USER_CXXFLAGS   := $(CXXFLAGS)
USER_ASFLAGS    := $(ASFLAGS)
USER_LDFLAGS    := $(LDFLAGS)
USER_LIBS       := $(LIBS)

CPPFLAGS    = -D__SAMD21G18A__ -DUSBCON $(COREINCS) -I$(CMSIS_DIR)/Include -I$(SAM_DIR)
CPPFLAGS   += -MMD -MP

# used everywhere
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb -ggdb3 -Os
ifneq ($(LTO),0)
CPUFLAGS   += -flto
else
$(info >>> LTO is disabled!)
endif

# used in CFLAGS/CXXFLAGS/ASFLAGS, but not LDFLAGS
CCXXFLAGS   = $(CPUFLAGS) -Wall -Wextra -Werror -Wno-expansion-to-defined
CCXXFLAGS  += -fno-exceptions -ffunction-sections -fdata-sections

CFLAGS      = $(CCXXFLAGS) -std=gnu11
CFLAGS     += $(USER_CFLAGS)

CXXFLAGS    = $(CCXXFLAGS) -std=gnu++11 -fno-rtti -fno-threadsafe-statics
CXXFLAGS   += $(USER_CXXFLAGS)

ASFLAGS     = $(CCXXFLAGS)
ASFLAGS    += $(USER_ASFLAGS)

LDSCRIPT   ?= $(VARIANT_DIR)/linker_scripts/gcc/flash_with_bootloader.ld
LDFLAGS     = $(CPUFLAGS) -fuse-linker-plugin -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LDFLAGS    += -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LDFLAGS    += -Wl,--warn-common -Wl,--warn-section-align
LDFLAGS    += -Wl,-Map=$(OBJDIR)/$(TARGET).map
LDFLAGS    += -L$(OBJDIR) -L$(CMSIS_DIR)
LDFLAGS    += $(USER_LDFLAGS)

CORELIB     = $(OBJDIR)/libcore.a
LIBS        = -lcore -lm -larm_cortexM0l_math
LIBS       += $(USER_LIBS)

vpath %.c   $(CORESRCDIRS)
vpath %.cpp $(CORESRCDIRS)
vpath %.S   $(CORESRCDIRS)

CORE_CC_SRC  = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.c)))
CORE_CXX_SRC = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.cpp)))
CORE_AS_SRC  = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.S)))

CORE_OBJS = $(patsubst %.cpp,$(OBJDIR)/%.o,$(CORE_CXX_SRC)) \
            $(patsubst %.c,$(OBJDIR)/%.o,$(CORE_CC_SRC)) \
            $(patsubst %.S,$(OBJDIR)/%.o,$(CORE_AS_SRC))

_TARGET_OBJ = $(patsubst %,$(OBJDIR)/%,$(TARGET_OBJS))
TARGET_ELF  = $(OBJDIR)/$(TARGET).elf
TARGET_BIN  = $(OBJDIR)/$(TARGET).bin
TARGET_HEX  = $(OBJDIR)/$(TARGET).hex

V ?= 0
_V_CC_0     = @echo "  CC      " $<;
_V_CXX_0    = @echo "  CXX     " $<;
_V_AS_0     = @echo "  AS      " $<;
_V_LD_0     = @echo "  LD      " $@;
_V_AR_0     = @echo "  AR      " $@;
_V_BIN_0    = @echo "  BIN     " $@;
_V_HEX_0    = @echo "  HEX     " $@;
_V_SIZE_0   = @echo "Program Size:";
_V_RESET_0  = @echo "  RESET   " $(COMPORT);
_V_UPLOAD_0 = @echo "  UPLOAD  " $<;
_V_CLEAN_0  = @echo "  CLEAN";

.PHONY: all
all: $(TARGET_BIN) .size_done

SIZE_CMD = $(_V_SIZE_$(V))$(SIZE) $(TARGET_ELF) $(TARGET_HEX)
.PHONY: size
size: $(TARGET_HEX)
	$(SIZE_CMD)

.size_done: $(TARGET_HEX) | $(TARGET_BIN)
	$(SIZE_CMD)
	@touch $@

.PHONY: upload
upload: $(TARGET_BIN) all
ifneq ($(BOSSA_19),y)
	$(_V_RESET_$(V))$(RESET_SCRIPT)
endif
	$(_V_UPLOAD_$(V))$(BOSSAC) $(BOSSAC_FLAGS) $<

.PHONY: clean
clean:
	$(_V_CLEAN_$(V))rm -rf $(OBJDIR) .size_done

.PHONY: gdb
gdb: $(TARGET_ELF)
	$(GDB) -q $(TARGET_ELF) -ex "target extended-remote :2331" -ex "load" -ex "mon reset"

$(TARGET_ELF): $(_TARGET_OBJ) $(CORELIB) $(LDSCRIPT)
	$(_V_LD_$(V))$(CCLD) $(LDFLAGS) -o $@ $(_TARGET_OBJ) -Wl,--as-needed $(LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(_V_BIN_$(V))$(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(_V_HEX_$(V))$(OBJCOPY) -O ihex $< $@

$(OBJDIR):
	@mkdir -p $@

$(OBJDIR)/%.o: %.c | $(OBJDIR)
	$(_V_CC_$(V))$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.cpp | $(OBJDIR)
	$(_V_CXX_$(V))$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c -o $@ $<

$(OBJDIR)/%.o: %.S | $(OBJDIR)
	$(_V_AS_$(V))$(AS) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

$(CORELIB): $(CORE_OBJS)
	$(_V_AR_$(V))$(AR) rcs $@ $^

-include $(wildcard $(OBJDIR)/*.d)
