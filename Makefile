################################################################################
# Arduino SAMD21 Makefile
#
# Copyright (C) 2018 Allen Wild <allenwild93@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.
################################################################################

# If a sketch subdir makefile set SKETCH, use it as an override.
# This lets config.mk define a default sketch, but running make from
# a sketch subdir will take precedence over that.
ifeq ($(SKETCH_FROM_SUBDIR),1)
override SKETCH := $(SKETCH)
endif

# allow for persistent config
-include config.mk

# Logic to figure out sketch name. (unless running make clean)
# S=<something> on the command line overrides everything else
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
ifneq ($(S),)
ifeq ($(origin S),command line)
override SKETCH := $(S)
endif
endif
# here we allow SKETCH to be set externally via other means (environment, config.mk)
# if still empty, fall back to S
ifeq ($(SKETCH),)
ifneq ($(S),)
SKETCH := $(S)
endif
endif
# confirm that SKETCH is set
ifeq ($(SKETCH),)
$(error S or SKETCH must be defined to use this Makefile)
endif
endif # clean

# TARGET is typically the sketch name, but could be overridden I guess
TARGET     := $(SKETCH)

# Directory Configuration
OBJDIR      = obj
CORE        = core

# all these directories will be used as CPP include paths, and
# all c/cpp/S sources will be compiled into libcore
LIBRARIES   = variant $(CORE) $(CORE)/USB
LIBRARIES  += Adafruit_ZeroDMA DigitalIO MPR121 Neostrip PWM SPI Timeout Timer Wire

CORESRCDIRS = $(addprefix lib/,$(LIBRARIES))
COREINCS    = $(addprefix -I,$(CORESRCDIRS))

VARIANT_DIR = lib/variant
CMSIS_DIR   = lib/CMSIS
SAM_DIR     = lib/CMSIS-Atmel

COMPORT     ?= /dev/ttyACM0
BOSSAC      ?= bossac
BOSSAC_FLAGS = --erase --write --verify --reset --port=$(COMPORT)
OS          := $(shell uname -s)
ifeq ($(OS),Linux)
RESET_SCRIPT = bin/reset-arduino-linux.sh -q $(COMPORT)
else
RESET_SCRIPT = bin/ard-reset-arduino --zero $(COMPORT)
endif

# my bossa-git AUR package sets the version to the Arch pkgver, which is 1.8.rXX.gYYYYYYY since
# there hasn't been a v1.9 tag yet, so figure out the version by checking for the availability
# of the --arduino-erase option
BOSSA_19 := $(shell $(BOSSAC) --help 2>/dev/null | grep -q -e '--arduino-erase' && echo y || echo n)
ifeq ($(BOSSA_19),y)
# we have auto-erase available
BOSSAC_FLAGS := $(filter-out --erase,$(BOSSAC_FLAGS))
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
CXXLD   = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size
GDB     = $(TOOLCHAIN_BIN)arm-none-eabi-gdb

SOURCE_VERSION := $(SKETCH)-$(shell bin/get_version.sh)

LCPPFLAGS   = -D__SAMD21G18A__ -DUSBCON -DSOURCE_VERSION='"$(SOURCE_VERSION)"'
LCPPFLAGS  += -I$(SKETCH) $(COREINCS) -I$(CMSIS_DIR)/Include -I$(SAM_DIR)
LCPPFLAGS  += -MMD -MP

# used everywhere
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb -ggdb3 -Os -pipe
ifneq ($(LTO),0)
CPUFLAGS   += -flto
else
$(info >>> LTO is disabled!)
endif

# used in CFLAGS/CXXFLAGS/ASFLAGS, but not LDFLAGS
CCXXFLAGS   = $(CPUFLAGS) -Wall -Wextra -Werror
ifeq ($(shell test "$(shell $(CC) -dumpversion | cut -d. -f1)" -ge 7 && echo 1),1)
CCXXFLAGS  += -Wno-expansion-to-defined
endif
CCXXFLAGS  += -fno-exceptions -ffunction-sections -fdata-sections

LCFLAGS     = $(CCXXFLAGS) -std=gnu11

LCXXFLAGS   = $(CCXXFLAGS) -std=gnu++11 -fno-rtti -fno-threadsafe-statics

LASFLAGS    = $(CCXXFLAGS)

LDSCRIPT   ?= $(VARIANT_DIR)/linker_scripts/gcc/flash_with_bootloader.ld
LLDFLAGS    = $(CPUFLAGS) -fuse-linker-plugin -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LLDFLAGS   += -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LLDFLAGS   += -Wl,--warn-common -Wl,--warn-section-align
LLDFLAGS   += -Wl,-Map=$(OBJDIR)/$(TARGET).map
LLDFLAGS   += -L$(OBJDIR) -L$(CMSIS_DIR)

CORELIB     = $(OBJDIR)/libcore.a
LLIBS       = -lcore -lm -larm_cortexM0l_math

ifeq ($(CLANG),1)
$(info >>> Building with clang!)
SYSROOT     = /usr/arm-none-eabi
CC          = clang -target arm-none-eabi --sysroot=$(SYSROOT)
CXX         = clang++ -target arm-none-eabi --sysroot=$(SYSROOT)
AR          = llvm-ar

CXX_INCDIR := $(shell find $(SYSROOT)/include/c++ -mindepth 1 -maxdepth 1 -type d | tail -n1)
CPUFLAGS   := $(filter-out -flto,$(CPUFLAGS))
# turn on -Weverything, then disable a ton of warnings I won't fix
#CCXXFLAGS  += -Weverything -Wno-reserved-id-macro -Wno-c++98-compat -Wno-c++98-compat-pedantic -Wno-undef
#CCXXFLAGS  += -Wno-non-virtual-dtor -Wno-old-style-cast -Wno-zero-as-null-pointer-constant -Wno-padded
#CCXXFLAGS  += -Wno-documentation -Wno-gnu-anonymous-struct -Wno-nested-anon-types -Wno-unused-function
#CCXXFLAGS  += -Wno-shadow -Wno-shadow-field-in-constructor -Wno-c++14-binary-literal
#CCXXFLAGS  += -Wno-c99-extensions -Wno-global-constructors -Wno-weak-vtables -Wno-sign-conversion -Wno-conversion
#CCXXFLAGS  += -Wno-error

# GNU linker expects short enums, but clang uses 32 bits by default
# clang wrongly thinks some symbol-aliased functions are unused
CCXXFLAGS  += -fshort-enums -Wno-unused-function
LCXXFLAGS  += -I$(CXX_INCDIR) -I$(CXX_INCDIR)/arm-none-eabi
endif # clang

define override_flags
override $(1) := $$(strip $$(L$(1)) $$($(1)))
endef
$(foreach f,CPPFLAGS CFLAGS CXXFLAGS ASFLAGS LDFLAGS LIBS,$(eval $(call override_flags,$(f))))

# don't look for a sketch when running make clean
ifneq ($(findstring clean,$(MAKECMDGOALS)),clean)
ifneq ($(findstring newsketch,$(MAKECMDGOALS)),newsketch)
TARGET_CC_SRC  = $(notdir $(wildcard $(SKETCH)/*.c))
TARGET_CXX_SRC = $(notdir $(wildcard $(SKETCH)/*.cpp))
TARGET_AS_SRC  = $(notdir $(wildcard $(SKETCH)/*.S))

TARGET_OBJ = $(patsubst %.cpp,$(OBJDIR)/%.o,$(TARGET_CXX_SRC)) \
             $(patsubst %.c,$(OBJDIR)/%.o,$(TARGET_CC_SRC)) \
             $(patsubst %.S,$(OBJDIR)/%.o,$(TARGET_AS_SRC))

ifeq ($(strip $(TARGET_OBJ)),)
$(error No source files found for sketch $(SKETCH))
endif
endif
endif

CORE_CC_SRC  = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.c)))
CORE_CXX_SRC = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.cpp)))
CORE_AS_SRC  = $(foreach dir,$(CORESRCDIRS),$(notdir $(wildcard $(dir)/*.S)))

CORE_OBJ = $(patsubst %.cpp,$(OBJDIR)/%.o,$(CORE_CXX_SRC)) \
           $(patsubst %.c,$(OBJDIR)/%.o,$(CORE_CC_SRC)) \
           $(patsubst %.S,$(OBJDIR)/%.o,$(CORE_AS_SRC))

TARGET_ELF  = $(OBJDIR)/$(TARGET).elf
TARGET_BIN  = $(OBJDIR)/$(TARGET).bin
TARGET_HEX  = $(OBJDIR)/$(TARGET).hex

V ?= 0
_V_CC_0        = @echo "  CC      " $<;
_V_CXX_0       = @echo "  CXX     " $<;
_V_AS_0        = @echo "  AS      " $<;
_V_LD_0        = @echo "  LD      " $@;
_V_AR_0        = @echo "  AR      " $@;
_V_BIN_0       = @echo "  BIN     " $@;
_V_HEX_0       = @echo "  HEX     " $@;
_V_SIZE_0      = @echo "Program Size:";
_V_RESET_0     = @echo "  RESET   " $(COMPORT);
_V_UPLOAD_0    = @echo "  UPLOAD  " $<;
_V_CLEAN_0     = @echo "  CLEAN";
_V_DISTCLEAN_0 = @echo "  DISTCLEAN";

vpath %.c   $(SKETCH) $(CORESRCDIRS)
vpath %.cpp $(SKETCH) $(CORESRCDIRS)
vpath %.S   $(SKETCH) $(CORESRCDIRS)

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

.PHONY: distclean
distclean: clean
	$(_V_DISTCLEAN_$(V))rm -f config.mk

.PHONY: config
config:
	if [ -f config.mk ]; then sed -i '/^SKETCH\b/d' config.mk; fi
	echo 'SKETCH = $(SKETCH)' >>config.mk

.PHONY: gdb
gdb: $(TARGET_ELF)
	$(GDB) -q $(TARGET_ELF) -ex "target extended-remote :2331" -ex "load" -ex "mon reset"

.PHONY: newsketch
newsketch:
	mkdir $(SKETCH)
	cp lib/sketch-template.cpp $(SKETCH)/$(SKETCH).cpp
	ln -s ../lib/Makefile.sketch $(SKETCH)/Makefile

.PHONY: dis
dis: $(TARGET_ELF)
	@$(OBJDUMP) -d $(TARGET_ELF)

.PHONY: disvim
disvim: $(TARGET_ELF)
	$(OBJDUMP) -d $(TARGET_ELF) | vim -R -c ':set ft=asm' -

$(TARGET_ELF): $(TARGET_OBJ) $(CORELIB) $(LDSCRIPT)
	+$(_V_LD_$(V))$(CXXLD) $(LDFLAGS) -o $@ $(TARGET_OBJ) -Wl,--as-needed $(LIBS)

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

$(CORELIB): $(CORE_OBJ)
	$(_V_AR_$(V))$(AR) rcs $@ $^

-include $(wildcard $(OBJDIR)/*.d)
