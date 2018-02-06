#include<nes.h>

typedef struct _Sprite {
    unsigned char y_position;
    unsigned char pattern_index;
    unsigned char attributes;
    unsigned char x_position;
} Sprite;

Sprite *sprites = (Sprite*)0x200;

#define x 128
#define y 120
#define HORIZONTAL_FLIP 0x40
#define VERTICAL_FLIP 0x80
char main() {
    PPU.vram.address = 0x3F;
    PPU.vram.address = 0x00;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x0F;
    PPU.vram.data = 0x0F;

    PPU.vram.address = 0x3F;
    PPU.vram.address = 0x11;

    PPU.vram.data = 0x2A;
    PPU.vram.data = 2;
    PPU.vram.data = 0x12;

    sprites[0].y_position = y-8;
    sprites[0].x_position = x-8;
    sprites[0].pattern_index = 0;
    sprites[0].attributes = 0;

    sprites[1].y_position = y-8;
    sprites[1].x_position = x;
    sprites[1].pattern_index = 0;
    sprites[1].attributes = HORIZONTAL_FLIP;

    sprites[2].y_position = y;
    sprites[2].x_position = x-8;
    sprites[2].pattern_index = 0;
    sprites[2].attributes = VERTICAL_FLIP;

    sprites[3].y_position = y;
    sprites[3].x_position = x;
    sprites[3].pattern_index = 0;
    sprites[3].attributes = HORIZONTAL_FLIP | VERTICAL_FLIP;

    SPRITE_DMA = 0x02;

    PPU.control = 0x08;
    PPU.mask = 0x18;

    while(1);
    return 0;
}
