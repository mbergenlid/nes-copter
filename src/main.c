#include<nes.h>

typedef struct _Sprite {
    unsigned char y_position;
    unsigned char pattern_index;
    unsigned char attributes;
    unsigned char x_position;
} Sprite;

Sprite *sprites = (Sprite*)0x200;

typedef struct _Copter {
    unsigned char velocity;
    unsigned char position;
} Copter;

Copter copter;

#define x 128
#define y 120
void updateCopterSprite() {
    sprites[0].y_position = copter.position-8;
    sprites[0].x_position = x-8;
    sprites[0].pattern_index = 0;
    sprites[0].attributes = 0;

    sprites[1].y_position = copter.position-8;
    sprites[1].x_position = x;
    sprites[1].pattern_index = 0;
    sprites[1].attributes = 0x40;

    sprites[2].y_position = copter.position;
    sprites[2].x_position = x-8;
    sprites[2].pattern_index = 0;
    sprites[2].attributes = 0x80;

    sprites[3].y_position = copter.position;
    sprites[3].x_position = x;
    sprites[3].pattern_index = 0;
    sprites[3].attributes = 0xC0;

    SPRITE_DMA = 0x02;
}

char main() {
    copter.velocity = 0;
    copter.position = y;

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

    updateCopterSprite();

    PPU.control = 0x88;
    PPU.mask = 0x18;

    while(1);
    return 0;
}

char tickCount = 0;
void nmi() {
    tickCount += 1;
    if(tickCount > 2) {
        tickCount = 0;
        copter.velocity += 1;
        copter.position += copter.velocity;

        updateCopterSprite();
    }
}
