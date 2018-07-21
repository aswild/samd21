/*******************************************************************************
 * SAMD21 Timeout library. SW-triggered oneshot timer with interrupt on timeout
 *
 * Copyright (C) 2018 Allen Wild <allenwild93@gmail.com>
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

#ifndef TIMEOUT_H
#define TIMEOUT_H

#ifdef __cplusplus
extern "C" {
#endif

void timeout_init(void);
void timeout_set_us(uint32_t timeout_us);
void timeout_start(void);
void timeout_stop(void);
void timeout_set_callback(void(*callback)(void));

#ifdef __cplusplus
}
#endif
#endif // TIMEOUT_H
