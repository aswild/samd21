# Application Configuration
TARGET = samd_sam_ba_sparkfun

SOURCES = main.c sam_ba_monitor.c startup_samd21.c usart_sam_ba.c \
          drivers/cdc_enumerate.c drivers/uart_driver.c \
          utils/interrupt/interrupt_sam_nvic.c

CMSIS_DIR = ../lib/CMSIS
INCLUDES = drivers utils utils/interrupt utils/preprocessor \
           $(CMSIS_DIR)/Include ../lib/CMSIS-Atmel

ifeq ($(APP),1)
# link as an application at 0x2000 and disable application loading (SAM-BA only)
LCPPFLAGS  += -DAPPMODE
FLASH_START = 0x00002000
FLASH_SIZE  = 0x0003E000
else
FLASH_START = 0x00000000
FLASH_SIZE  = 0x00040000
endif

COMPORT     ?= /dev/ttyACM0
BOSSAC      ?= bossac
BOSSAC_FLAGS = --erase --write --verify --reset --port=$(COMPORT)
OS          := $(shell uname -s)
ifeq ($(OS),Linux)
RESET_SCRIPT = ../bin/reset-arduino-linux.sh -q $(COMPORT)
else
RESET_SCRIPT = ../bin/ard-reset-arduino --zero $(COMPORT)
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
BOSSAC_FLAGS += --offset=$(FLASH_START)
endif

# Tools Configuration
TOOLCHAIN_BIN ?=
CC      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
CXX     = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size

OPENOCD = openocd
OPENOCD_CMD = -f ../openocd.cfg -c "program $(TARGET_BIN) verify reset exit"

VERSION    := $(shell ../scripts/get_version.sh --sam-ba)
USB_PRODUCT = SAMD21G18A SAM-BA

LCPPFLAGS   = -D__SAMD21G18A__ -DUSB_PRODUCT='"$(USB_PRODUCT)"' -DSOURCE_VERSION='"$(VERSION)"'
LCPPFLAGS  += $(addprefix -I,$(INCLUDES)) -MMD -MP
ifeq ($(APP),1)
USB_PRODUCT += APP
LCPPFLAGS  += -DAPPMODE
endif

# used in CFLAGS/LDFLAGS
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb -ggdb3 -Os

# not used in LDFLAGS
LCFLAGS     = -std=gnu99 $(CPUFLAGS) -Wall -Wextra -Werror -Wno-expansion-to-defined
LCFLAGS    += -fno-exceptions -ffunction-sections -fdata-sections

LDSCRIPT   ?= samd21g18a_flash.ld

LLDFLAGS    = $(CPUFLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LLDFLAGS   += -Wl,--defsym,FLASH_START=$(FLASH_START) -Wl,--defsym,FLASH_SIZE=$(FLASH_SIZE)
LLDFLAGS   += -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LLDFLAGS   += -Wl,--warn-common -Wl,--warn-section-align
LLDFLAGS   += -Wl,-Map=$(TARGET).map
LLDFLAGS   += -L$(CMSIS_DIR)

LLIBS       = -larm_cortexM0l_math

define override_flags =
override $(1) := $$(strip $$(L$(1)) $$($(1)))
endef
$(foreach f,CPPFLAGS CFLAGS LDFLAGS LIBS,$(eval $(call override_flags,$(f))))

TARGET_OBJ  = $(SOURCES:.c=.o)
DEPS        = $(TARGET_OBJ:.o=.d)
TARGET_ELF  = $(TARGET).elf
TARGET_BIN  = $(TARGET).bin
TARGET_HEX  = $(TARGET).hex

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

SIZE_CMD = $(_V_SIZE_$(V))../scripts/print-size.py -q $(TARGET_ELF)
.PHONY: size
size: $(TARGET_ELF)
	$(SIZE_CMD)

.size_done: $(TARGET_ELF) | $(TARGET_BIN)
	$(SIZE_CMD)
	@touch $@

.PHONY: upload
upload: $(TARGET_BIN) all
	$(_V_UPLOAD_$(V))$(OPENOCD) $(OPENOCD_CMD)

.PHONY: upload-bossa
upload-bossa: $(TARGET_BIN) all
	$(_V_RESET_$(V))$(RESET_SCRIPT)
	$(_V_UPLOAD_$(V))$(BOSSAC) $(BOSSAC_FLAGS) $<

.PHONY: hex
hex: $(TARGET_HEX)

.PHONY: clean
clean:
	$(_V_CLEAN_$(V))rm -f $(TARGET_OBJ) $(DEPS) $(TARGET_ELF) $(TARGET_BIN) $(TARGET_HEX) $(TARGET).map .size_done

$(TARGET_ELF): $(TARGET_OBJ) $(LDSCRIPT)
	$(_V_LD_$(V))$(CC) $(LDFLAGS) -o $@ $(TARGET_OBJ) -Wl,--as-needed $(LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(_V_BIN_$(V))$(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(_V_HEX_$(V))$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(_V_CC_$(V))$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

-include $(DEPS)
