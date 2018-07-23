#!/usr/bin/env python3

import sys
from PIL import Image

HEADER_START = """#ifndef GRADIENT_H
#define GRADIENT_H

static const Color gradient[%d] = {
"""

if len(sys.argv) > 1:
    image_file = sys.argv[1]
else:
    image_file = '/run/media/allen/521D-6077/bbq gradient.bmp'

if len(sys.argv) > 2:
    outfile = sys.argv[2]
else:
    outfile = 'gradient.h'

img = Image.open(image_file)
width = img.width
data = list(img.getdata())
img.close()

grad = data[:width]

with open(outfile, 'w') as fp:
    fp.write(HEADER_START%width)
    for p in grad:
        c = (p[0] << 16) | (p[1] << 8) | p[2]
        fp.write('    { .i = 0x%06x },\n'%c)
    fp.write('};\n\n#endif\n')
