/*******************************************************************************
 * NeoPixel library for Arduino SAMD21 using SPI and DMA
 *
 * Copyright (C) 2019 Allen Wild <allenwild93@gmail.com>
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

#ifndef NEOSTRIP_ANIMATION_H
#define NEOSTRIP_ANIMATION_H

#include "Neostrip.h"

/*
 * enum for the return value of the next() method.
 */
enum FrameResult
{
    FR_CONTINUE,    // more frames to come
    FR_DONE,        // animation complete, call to reset() needed
    FR_RESET,       // one cycle of an infinite looping animation complete
};

/*
 * Abstract class to represent neostrip animations
 */
template<size_t N>
class NeostripAnimation
{
    public:

        NeostripAnimation(Neostrip<N>& _ns) : ns(_ns) {}
        virtual ~NeostripAnimation(void) {}

        /*
         * Reset the animation, preparing for the first frame.
         * Don't write anything to the strip yet.
         */
        virtual void reset(void) = 0;

        /*
         * Compute the next frame and update the strip object, but don't write it yet.
         */
        virtual FrameResult next(void) = 0;

        inline void write(bool sync=false) { ns.write(sync); }

    protected:
        Neostrip<N>& ns;
};

#endif // NEOSTRIP_ANIMATION_H
