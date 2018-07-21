/*******************************************************************************
 * SAMD21 timing debug pin macros
 *
 * Copyright (C) 2018 Allen Wild <allenwild93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef DEBUG_MACROS_H
#define DEBUG_MACROS_H

#if defined(DEBUG_PORT) && defined(DEBUG_PIN)
#define DBGINIT() do { PORT->Group[DEBUG_PORT].DIRSET.reg = 1UL << (DEBUG_PIN); } while (0)
#define DBGHIGH() do { PORT->Group[DEBUG_PORT].OUTSET.reg = 1UL << (DEBUG_PIN); } while (0)
#define DBGLOW()  do { PORT->Group[DEBUG_PORT].OUTCLR.reg = 1UL << (DEBUG_PIN); } while (0)
#else
#define DBGINIT() do { } while(0)
#define DBGHIGH() do { } while(0)
#define DBGLOW()  do { } while(0)
#endif

#endif
