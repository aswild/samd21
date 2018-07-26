#!/usr/bin/env python3

# adapted from http://jared.geek.nz/2013/feb/linear-led-pwm

INPUT_SIZE = 255
OUTPUT_SIZE = 255

def cie1931(L):
    L = L * 100.0
    if L <= 8:
        return L / 903.3
    else:
        return ((L + 16.0) / 116.0) ** 3

table = (round(cie1931(float(y)/INPUT_SIZE) * OUTPUT_SIZE) for y in range(INPUT_SIZE+1))

print("static const uint8_t cie1931_table[256] = {")
for i, y in enumerate(table):
    if i % 16 == 0:
        print('   ', end='')
    print(' %3d,'%int(y), end='')
    if i % 16 == 15:
        print('')
print('};')
