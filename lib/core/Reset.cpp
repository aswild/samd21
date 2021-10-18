/*
  Copyright (c) 2015 Arduino LLC.  All right reserved.

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Lesser General Public
  License as published by the Free Software Foundation; either
  version 2.1 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  See the GNU Lesser General Public License for more details.

  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
*/

#include <Arduino.h>
#include "Reset.h"

#ifdef __cplusplus
extern "C" {
#endif

#define NVM_MEMORY ((volatile uint16_t *)0x000000)

#if (ARDUINO_SAMD_VARIANT_COMPLIANCE >= 10610)

extern uint32_t __text_start__;
#define APP_START ((uint32_t)(&__text_start__) + 4)

#else
#define APP_START 0x00002004
#endif

// rather than erasing the reset handler address of the application, which permanently
// kills it, we can trick the bootloader into thinking the user double-tapped the reset
// button by writing the same magic value to the same RAM location.
// __bootloader_trap_reg may be provided by the linker script and must match BOOT_DOUBLE_TAP_ADDRESS
// in the bootloader code. It's at the very top of RAM and should be 0x20007FFC.
// If not provided by the linker script, this weak symbol definition will be used, default
// to NULL, and we'll fall back to the old method of resetting to the bootloader by
// erasing the reset handler address.
extern uint32_t __attribute__((weak)) __bootloader_trap_reg;
volatile uint32_t *bootloader_trap_reg = &__bootloader_trap_reg;

static volatile bool bootloaderReset = false;

// obviously, this magic value must match the bootloader code
#define BOOTLOADER_TRAP_MAGIC 0x07738135

static inline bool nvmReady(void) {
        return NVMCTRL->INTFLAG.reg & NVMCTRL_INTFLAG_READY;
}

void banzai(void) {
	// Disable all interrupts
	__disable_irq();

	// Avoid erasing the application if APP_START is < than the minimum bootloader size
	// This could happen if without_bootloader linker script was chosen
	// Minimum bootloader size in SAMD21 family is 512bytes (RM section 22.6.5)
	if (!bootloaderReset || (APP_START < (0x200 + 4))) {
		goto reset;
	}

	if (bootloader_trap_reg != NULL) {
		*bootloader_trap_reg = BOOTLOADER_TRAP_MAGIC;
	} else {
		// Erase application
		while (!nvmReady())
			;
		NVMCTRL->STATUS.reg |= NVMCTRL_STATUS_MASK;
		NVMCTRL->ADDR.reg  = (uintptr_t)&NVM_MEMORY[APP_START / 4];
		NVMCTRL->CTRLA.reg = NVMCTRL_CTRLA_CMD_ER | NVMCTRL_CTRLA_CMDEX_KEY;
		while (!nvmReady())
			;
	}

reset:
	// Reset the device
	NVIC_SystemReset() ;

	while (true);
}

static int ticks = -1;

void initiateReset(int _ticks, int _bootloaderReset) {
	resetExternalChip();
	bootloaderReset = static_cast<bool>(_bootloaderReset);
	ticks = _ticks;
}

void cancelReset() {
	ticks = -1;
}

void tickReset() {
	if (ticks == -1)
		return;
	ticks--;
	if (ticks == 0)
		banzai();
}

#ifdef __cplusplus
}
#endif
