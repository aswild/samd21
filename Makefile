# Application Configuration
TARGET = blinky
TARGET_OBJS = blinky.o

# Directory Configuration
OBJDIR      = obj
CORE        = core

# all these directories will be used as CPP include paths, and
# all c/cpp/S sources will be compiled into libcore
LIBRARIES   = variant $(CORE) $(CORE)/USB

CORESRCDIRS = $(addprefix lib/,$(LIBRARIES))
COREINCS    = $(addprefix -I,$(CORESRCDIRS))

VARIANT_DIR = lib/variant
CMSIS_DIR   = lib/CMSIS
SAM_DIR     = lib/CMSIS-Atmel

COMPORT     ?= /dev/ttyACM0
BOSSAC       = bossac
BOSSAC_FLAGS = --erase --write --verify --reset --port=$(COMPORT)
RESET_SCRIPT = bin/ard-reset-arduino --zero $(COMPORT)

# Tools Configuration
TOOLCHAIN_BIN ?=
CC      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
CXX     = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size

USER_CFLAGS     := $(CFLAGS)
USER_CXXFLAGS   := $(CXXFLAGS)
USER_ASFLAGS    := $(ASFLAGS)
USER_LDFLAGS    := $(LDFLAGS)
USER_LIBS       := $(LIBS)

CPPFLAGS    = -D__SAMD21G18A__ -DF_CPU=48000000L -DUSBCON
CPPFLAGS   += -DUSB_MANUFACTURER='"SparkFun"' -DUSB_PRODUCT='"SFE SAMD21"' -DUSB_VID=0x1B4F -DUSB_PID=0x8D21
CPPFLAGS   += $(COREINCS) -I$(CMSIS_DIR)/Include -I$(SAM_DIR)
CPPFLAGS   += -MMD -MP

# used everywhere
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb -ggdb3 -Os

# used in CFLAGS/CXXFLAGS/ASFLAGS, but not LDFLAGS
CCXXFLAGS   = $(CPUFLAGS) -Wall -Wextra -Werror -Wno-expansion-to-defined
CCXXFLAGS  += -fno-exceptions -ffunction-sections -fdata-sections

CFLAGS      = $(CCXXFLAGS) -std=gnu11
CFLAGS     += $(USER_CFLAGS)

CXXFLAGS    = $(CCXXFLAGS) -std=gnu++11 -fno-rtti -fno-threadsafe-statics
CXXFLAGS   += $(USER_CXXFLAGS)

ASFLAGS     = $(CCXXFLAGS) -x assembler-with-cpp
ASFLAGS    += $(USER_ASFLAGS)

LDSCRIPT    = $(VARIANT_DIR)/linker_scripts/gcc/flash_with_bootloader.ld
LDFLAGS     = $(CPUFLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
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
	$(_V_RESET_$(V))$(RESET_SCRIPT)
	$(_V_UPLOAD_$(V))$(BOSSAC) $(BOSSAC_FLAGS) $<

.PHONY: clean
clean:
	$(_V_CLEAN_$(V))rm -rf $(OBJDIR) .size_done

$(TARGET_ELF): $(_TARGET_OBJ) $(CORELIB) $(LDSCRIPT)
	$(_V_LD_$(V))$(CC) $(LDFLAGS) -o $@ $(_TARGET_OBJ) -Wl,--as-needed $(LIBS)

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
	$(_V_AS_$(V))$(CC) $(CPPFLAGS) $(ASFLAGS) -c -o $@ $<

$(CORELIB): $(CORE_OBJS)
	$(_V_AR_$(V))$(AR) rcs $@ $^

-include $(wildcard $(OBJDIR)/*.d)
