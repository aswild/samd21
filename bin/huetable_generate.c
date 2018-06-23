#include <stdio.h>
#include <stdint.h>

static uint32_t hue_to_color(float h)
{
    // lots of floating point magic from the internet and scratching my head
    float r, g, b;
    if (h > 360.0f)
        h -= 360.0f;
    if (h < 0.0f)
        h += 360.0f;
    int i = (int)(h / 60.0f);
    float f = (h / 60.0f) - i;
    float q = 1.0f - f;

    switch (i % 6)
    {
        case 0: r = 1; g = f; b = 0; break;
        case 1: r = q; g = 1; b = 0; break;
        case 2: r = 0; g = 1; b = f; break;
        case 3: r = 0; g = q; b = 1; break;
        case 4: r = f; g = 0; b = 1; break;
        case 5: r = 1; g = 0; b = q; break;
        default: r = 0; g = 0; b = 0; break;
    }

    // scale to integers and save in the color
    uint8_t R = (uint8_t)(r * 255.0f);
    uint8_t G = (uint8_t)(g * 255.0f);
    uint8_t B = (uint8_t)(b * 255.0f);

    return (R<<16) | (G<<8) | B;
}

int main()
{
    printf("#ifndef HUETABLE_H\n"
           "#define HUETABLE_H\n"
           "\n"
           "static const uint32_t huetable[360] = {\n"
           );

    for (int i = 0; i < 360; i++)
    {
        printf("    0x%08x,\n", hue_to_color((float)i));
    }

    printf("};\n"
           "\n"
           "#endif // HUETABLE_H\n");
    return 0;
}
