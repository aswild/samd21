# Application Configuration
TARGET = samd_sam_ba_sparkfun

SOURCES = main.c sam_ba_monitor.c startup_samd21.c usart_sam_ba.c \
          drivers/cdc_enumerate.c drivers/uart_driver.c \
          utils/interrupt/interrupt_sam_nvic.c

CMSIS_DIR = ../lib/CMSIS
INCLUDES = drivers utils utils/interrupt utils/preprocessor \
           $(CMSIS_DIR)/Include ../lib/CMSIS-Atmel

# Tools Configuration
TOOLCHAIN_BIN ?=
CC      = $(TOOLCHAIN_BIN)arm-none-eabi-gcc
CXX     = $(TOOLCHAIN_BIN)arm-none-eabi-g++
AR      = $(TOOLCHAIN_BIN)arm-none-eabi-ar
OBJCOPY = $(TOOLCHAIN_BIN)arm-none-eabi-objcopy
OBJDUMP = $(TOOLCHAIN_BIN)arm-none-eabi-objdump
SIZE    = $(TOOLCHAIN_BIN)arm-none-eabi-size

CPPFLAGS    = -D__SAMD21G18A__ $(addprefix -I,$(INCLUDES))
CPPFLAGS   += -MMD -MP

# used everywhere
CPUFLAGS    = -mcpu=cortex-m0plus -mthumb -ggdb3 -Os

# used in CFLAGS/CXXFLAGS/ASFLAGS, but not LDFLAGS
CFLAGS      = -std=gnu99 $(CPUFLAGS) -Wall -Wextra -Werror -Wno-expansion-to-defined
CFLAGS     += -fno-exceptions -ffunction-sections -fdata-sections

LDSCRIPT   ?= samd21g18a_flash.ld
LDFLAGS     = $(CFLAGS) -T$(LDSCRIPT) --specs=nano.specs --specs=nosys.specs
LDFLAGS    += -Wl,--cref -Wl,--check-sections -Wl,--gc-sections -Wl,--unresolved-symbols=report-all
LDFLAGS    += -Wl,--warn-common -Wl,--warn-section-align
LDFLAGS    += -Wl,-Map=$(TARGET).map
LDFLAGS    += -L$(CMSIS_DIR)

LIBS        = -larm_cortexM0l_math

TARGET_OBJ  = $(SOURCES:.c=.o)
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
	$(_V_CLEAN_$(V))rm -rf $(TARGET_OBJ) $(TARGET_ELF) $(TARGET_BIN) $(TARGET_HEX) $(TARGET).map .size_done

$(TARGET_ELF): $(TARGET_OBJ) $(LDSCRIPT)
	$(_V_LD_$(V))$(CC) $(LDFLAGS) -o $@ $(TARGET_OBJ) -Wl,--as-needed $(LIBS)

$(TARGET_BIN): $(TARGET_ELF)
	$(_V_BIN_$(V))$(OBJCOPY) -O binary $< $@

$(TARGET_HEX): $(TARGET_ELF)
	$(_V_HEX_$(V))$(OBJCOPY) -O ihex $< $@

%.o: %.c
	$(_V_CC_$(V))$(CC) $(CPPFLAGS) $(CFLAGS) -c -o $@ $<

-include $(wildcard $(OBJDIR)/*.d)
