# Makefile, Library, and Sketches for the SparkFun SAMD21 Mini board

I only have one Arduino board, so why have tons of dependencies and generic
build scripts for tons of targets?

This project builds for the [SparkFun SAMD21 Mini Breakout](https://www.sparkfun.com/products/13664),
a SAMD21G18A board compatible with the Arduino Zero. It should be usable for other
SAMD21 boards with minimal tweaking.

I started with the Arduino IDE to get the SAMD boards and SparkFun board
definitions and the [Arduino-Makefile](https://github.com/sudar/Arduino-Makefile)
project, then adapted the parts I need into my own Makefile.

## Layout and Usage
- Sketch applications get their own directories at the top-level, and all
  c/cpp/S files within are compiled and linked into the final binary.
- Run make with `SKETCH=name` or `S=name` to chose a sketch. For convenience
  those variables can be exported from the environment or set in a top-level
  `config.mk` file.
- Sketch programs should have the .cpp extension (not .ino) and must include
  `Arduino.h` (the Makefile doesn't `-include` any headers automatically)
- `lib/core` contains the Arduino core libraries (with various modifications)
- `lib/variant` contains the SparkFun board variant files
- `lib/CMSIS` and `lib/CMSIS-Atmel` contain the low-level ARM and SAM header files.
- Other directories in `lib/` are specific libraries enabled via `LIBRARIES` in
  the Makefile
  - All libraries are usually enabled and compiled, but only functions and
    objects actually
    referenced by the application code will be linked in the final binary.
- All c/cpp/S files in the lib directories are put in `libcore.a`, and each
  directory gets a -I option on the GCC command-line.
- All objects and the final elf/bin output are put in the `obj/` directory
- `obj/` has only one level of files and `VPATH` is used to add all the various
  source directories. This means there can't be multiple source files with the
  same basename

## Components

### lib/core
- Arduino SAMD core v1.6.20
- https://github.com/arduino/ArduinoCore-samd
- SparkFun provides their own core and libraries, but they're not noticeably
  different than the upstream Arduino code.

### lib/variant
- SparkFun SAMD Arduino core v1.4.0
- https://github.com/sparkfun/Arduino_Boards
- variant files only

### lib/CMSIS
- ARM CMSIS 5.4.0
- https://github.com/ARM-software/CMSIS_5
- just the Include directory, and Lib/GCC/libarm_cortexM0l_math.a

### lib/CMSIS-Atmel
- https://github.com/arduino/ArduinoModule-CMSIS-Atmel
- The Atmel (Microchip) download link is broken, so I used the Arduino module
  from github
- Version 1.2.0
- Just the files I need from the CMSIS-Atmel/CMSIS/Device/ATMEL dir:
  sam.h, samd.h, samd21/

### bootloader
- https://github.com/sparkfun/SAMD21_Mini_Breakout
- Firmware directory of that repo
- I rewrote the Makefile to taste and fixed compiler errors so it builds
- Changed the version extended capabilities to [Arduino:XYZ] instead of
  [SparkFun:XYZ] so that BOSSA recognizes the extra commands like chip-erase
  and write-buffer.
- Allow compiling the SAM-BA monitor "bootloader" as an application, which
  enables updating the real bootloader without a JTAG/SWD programmer.

### Arduino-Makefile
- https://github.com/sudar/Arduino-Makefile
- The inspiration and reference for the first versions of this repo
- Ran this makefile to see how the toolchains were called and what was
  compiled so I could write my own makefile
- The `ard-reset-arduino` python script is copied directly from that repo,
  though on Linux I wrote a shell script that calls `stty` and is a bit faster.
- This project is great, but too automagic for my taste and too complex to
  easily customize

## Notable changes to the core
- Fix compiler warnings in the Arduino and CMSIS code (builds with `-Wall -Wextra -Werror`)
- Fix LTO builds, which enables optimizations like inlining across compilation
  units.  I had to add `__attribute__((used))` to a couple functions (the
  CDC_Setup function was perplexing and got included anyway in LTO builds, but
  for whatever reason the 1200 baud bootloader reset only works when marking
  that function with the used attribute.
- Add a printf method for the Print class using `vasprintf` from newlib,
  which makes serial use way nicer. Adding `-u _printf_float` to `LDFLAGS` should
  add support for `%f`, but it hasn't been working for me (based on some brief
  searching it seems that may be related to heap starvation).
- De-template RingBuffer so that applications can resize the serial buffer
- Sacrifice 4 bytes of memory so that the 1200 baud USB bootloader reset doesn't
  erase the app start address. Instead it sets the same magic register that the
  bootloader uses to detect a reset button double-tap, allowing going back to
  the application without reprogramming.

## Arch Linux Packages
- arm-none-eabi-gcc
- arm-none-eabi-binutils
- arm-none-eabi-newlib
- arm-none-eabi-gdb
- bossa-git (AUR) 1.9.1.r2.g3279031-1
  - the internet says that Arduino forked bossa and the upstream version
    doesn't work, but the AUR package (that I made) worked just fine for me
- jlink-software-and-documentation (AUR)
  - GDB server for [JLink SWD programmer](https://www.segger.com/products/debug-probes/j-link/models/j-link-edu-mini/)
  - Awesome hardware programmer/debugger for only $20!

## License
- Most core libraries are licensed under the GNU Lesser General Public License
  Version 2.1
- Some libraries and header files are licensed under an MIT or BSD-style license
- My application sketches and Makefiles are licensed under the GNU General Public
  License Version 3
- Refer to individual source files for specific copyright and license details
- Copies of the LGPLv2.1 and GPLv3 license text are avilable in the `docs/`
  directory of this repository
