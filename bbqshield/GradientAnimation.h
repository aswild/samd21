/*******************************************************************************
 * Neopixel fire/glowing effect
 *
 * Copyright (C) Allen Wild <allenwild93@gmail.com>
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

#include <stdlib.h>
#ifndef RANDOM
#define RANDOM lrand48
#endif

#include "Color.h"
#include "Neostrip.h"
#include "NeostripAnimation.h"

template<size_t N, size_t FADE_STEPS>
class GradientAnimation : public NeostripAnimation<N>
{
    public:
        GradientAnimation(Neostrip<N>& _ns, const Color *_gradient, size_t _gradient_size)
            : NeostripAnimation<N>(_ns), gradient(_gradient), gradient_size(_gradient_size)
        {
            step = 0;
            cstart = colors1;
            cstop = colors2;
        }

        virtual void reset(void)
        {
            step = 0;
            cstart = colors1;
            cstop = colors2;
            for (size_t i = 0; i < N; i++)
            {
                colors1[i] = RANDOM() % gradient_size;
                colors2[i] = RANDOM() % gradient_size;
            }
        }

        virtual FrameResult next(void)
        {
            for (size_t i = 0; i < N; i++)
            {
                uint8_t gc = cstart[i] + ((step * (cstop[i] - cstart[i])) / (FADE_STEPS-1));
                this->ns[i] = gradient[gc];
            }

            if (++step >= FADE_STEPS)
            {
                for (size_t i = 0; i < N; i++)
                    cstart[i] = RANDOM() % gradient_size;
                if (cstart == colors1)
                {
                    cstart = colors2;
                    cstop = colors1;
                }
                else
                {
                    cstart = colors1;
                    cstop = colors2;
                }
                step = 0;
                return FR_RESET;
            }
            return FR_CONTINUE;
        }

    protected:
        const Color *gradient;
        size_t gradient_size;
        uint8_t colors1[N];
        uint8_t colors2[N];
        uint8_t *cstart, *cstop;
        size_t step;
};
