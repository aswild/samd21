/*******************************************************************************
 * RGB Color Struct/Union
 *
 * Copyright (C) 2018-2019 Allen Wild <allenwild93@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, see <https://www.gnu.org/licenses/>.
 ******************************************************************************/

#ifndef NEOSTRIP_COLOR_H
#define NEOSTRIP_COLOR_H

#include <stdint.h>

#ifdef __cplusplus
union __attribute__((packed)) Color {
#else
typedef union __attribute__((packed)) {
#endif
    struct {
        uint8_t blue;
        uint8_t green;
        uint8_t red;
    } b;
    uint32_t i:24;

#ifdef __cplusplus
    Color(void) = default;
    Color(int i) : i(i) {}

    Color& operator=(const Color& c) = default;
    Color& operator=(const int i)
    {
        this->i = i;
        return *this;
    }

    operator int(void) const { return i; }
};
#else
} Color; // C typedef, name comes after definition
#endif

static const Color BLACK    = { .i = 0x000000 };
static const Color WHITE    = { .i = 0xFFFFFF };
static const Color RED      = { .i = 0xFF0000 };
static const Color YELLOW   = { .i = 0xFFFF00 };
static const Color GREEN    = { .i = 0x00FF00 };
static const Color CYAN     = { .i = 0x00FFFF };
static const Color BLUE     = { .i = 0x0000FF };
static const Color MAGENTA  = { .i = 0xFF00FF };

#endif
