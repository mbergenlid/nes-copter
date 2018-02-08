---
layout: post
title:  "Putting something on the screen"
date:   2018-02-02 08:01:00 +0100
---

Rendering to the screen is handled by the Picture Processing Unit (PPU), the PPU has it's own internal memory called VRAM with an address space of 2^16 bytes (same as the CPU memory). For each frame, the PPU draws "whatever" is currently in the PPU memory.

That's a pretty vague explanation so let's make it concrete by creating a sprite and draw it on the screen. I'm thinking to start with displaying the sprite that will represent our helicopter. Given my very artistic skills, I'm not going to attempt an actual helicopter but rather something like this:

![Sprite]({{ "/assets/sprite.png" | absolute_url }})

Drawing a sprite on the screen requires three things. A pattern, a colour palette and some meta data, also known as OAM (Object Attribute Memory), such as position. The pattern defines an 8x8 pixel matrix where each value can be `0-3`, these patterns are stored as 16 bytes in two "planes". We will see how this works when we look at an example. Each pixel (values between `0-3`) is used as an index into the colour palette to select the colour for the pixel, each colour palette holds three colours (a pixel value `0` means transparent).

Our image above is 16x16 pixel so it requires four sprites. Let's start by creating the pattern for the top left sprite, the 8x8 pattern should look something like this.

```
0 0 0 0 0 0 0 1
0 0 0 0 0 0 1 1
0 0 0 0 0 1 1 1
0 0 0 0 1 1 1 1
0 0 0 1 1 1 1 3
0 0 1 1 1 1 1 3
0 1 1 1 1 1 3 3
1 1 1 1 3 3 3 3
```

Every digit represents one pixel in our sprite, the PPU then uses these numbers to look up the actual colour from one of the colour palettes, the colour palette should then look something like this: 

```
0 => Transparent
1 => Green
2 => Don't care
3 => Blue
```

The pattern is encoded using two planes, where the 8 first bytes denotes the least significant bit of each pixel and the 8 high bytes denotes the most significant bit. Our pattern would therefore look like this:

```
$0xx0: 0 0 0 0 0 0 0 1   $0xx8: 0 0 0 0 0 0 0 0
$0xx1: 0 0 0 0 0 0 1 1   $0xx9: 0 0 0 0 0 0 0 0
$0xx2: 0 0 0 0 0 1 1 1   $0xxA: 0 0 0 0 0 0 0 0
$0xx3: 0 0 0 0 1 1 1 1   $0xxB: 0 0 0 0 0 0 0 0
$0xx4: 0 0 0 1 1 1 1 1   $0xxC: 0 0 0 0 0 0 0 1
$0xx5: 0 0 1 1 1 1 1 1   $0xxD: 0 0 0 0 0 0 0 1
$0xx6: 0 1 1 1 1 1 1 1   $0xxE: 0 0 0 0 0 0 1 1
$0xx7: 1 1 1 1 1 1 1 1   $0xxF: 0 0 0 0 1 1 1 1
```

The pattern data are located at address `0x0000`-`0x1FFF` in the PPU memory, half of that is for the background (which we don't care about right now) and the other half is for sprites. Which half is controlled by the PPU control register.

I'm going to use address `0x1000`-`0x1FFF` for sprite patterns which means I need to put those 16 bytes at PPU address `0x1000`. As mentioned in the [previous article]({{ site.baseurl }}{% post_url 2018-02-01-getting-started %}), when examining the iNES file, address `0x000-0x1FFF` in the PPU memory are mapped to the CHR ROM bank which essentially means that we just need to put the sprite pattern into the iNES file in the correct location. How do we do that? To answer that we should try to understand how this iNES file is generated.

### Configure the iNES output ###

It is the linker `ld65` that creates the iNES output. From our Makefile
```
ld65 -t nes -o target/a.nes ${OBJECTS} nes.lib
```

The `-t nes` here is what tells the linker that we want to target the NES and what this in turn does is that it will use the default NES configuration file located in `<cc65-root>/cfg/nes.cfg` and looks something like this (only included the things that are of interest right now).

{% highlight bash %}
...
MEMORY {
    ...
    # INES Cartridge Header
    HEADER: file = %O, start = $0000, size = $0010, fill = yes;

    # 2 16K ROM Banks
    # - startup
    # - code
    # - rodata
    # - data (load)
    ROM0:   file = %O, start = $8000, size = $7FFA, fill = yes, define = yes;

    # Hardware Vectors at End of 2nd 8K ROM
    ROMV:   file = %O, start = $FFFA, size = $0006, fill = yes;

    # 1 8k CHR Bank
    ROM2:   file = %O, start = $0000, size = $2000, fill = yes;
    ...
}
SEGMENTS {
    HEADER:   load = HEADER,          type = ro;
    ...
    CHARS:    load = ROM2,            type = rw;
    ...
}
...
{% endhighlight %}

The `MEMORY` section defines a number of memory areas. A memory area has a start address and a size, the start address denotes the CPU memory address where this memory is located at runtime. The optional file parameter for a memory area specifies which file to write this memory to (%O is the default file specified by the `-o` flag when invoking the linker). When the memory areas are written to the file (`target/a.nes` in our case) they are written in the order they appear in this configuration file.

The `SEGMENTS` section defines segments which can be used in the assembler files with the `segment` instruction. For example, the file `<cc65-root>/libsrc/nes/crt0.s` which are included by the linker contains the following:

{% highlight ca65 %}
segment        "HEADER"

        .byte   $4e,$45,$53,$1a ; "NES"^Z
        .byte   2               ; ines prg  - Specifies the number of 16k prg banks.
        .byte   1               ; ines chr  - Specifies the number of 8k chr banks.
        .byte   %00000011       ; ines mir  - Specifies VRAM mirroring of the banks.
        .byte   %00000000       ; ines map  - Specifies the NES mapper used.
        .byte   0,0,0,0,0,0,0,0 ; 8 zeroes
{% endhighlight %}

This section simply creates the iNES header by first switching to the HEADER segment and then outputting all the header bytes. I'm going to copy this file into our source folder so that we can modify the file later. Now our source folder looks like this:

{% highlight bash %}
src/
├── crt0.s
├── main.c
└── sprites.inc
{% endhighlight %}

I deleted the `test.c` file since that was just used for the initial testing. The end of `crt0.s` looks something like this:

{% highlight ca65 %}
.segment "CHARS"
    .include "neschars.inc"
{% endhighlight %}

Which we would like to replace with the following instead.

{% highlight ca65 %}
.segment "CHARS"
    .res $1000
    .include "sprites.inc"
{% endhighlight %}

Here the `.res` is used to skip the first 4096 bytes in the "CHARS" segment (`ca65` assembler uses the `$`-sign for hexadecimal values as opposed to `C` where `0x`-prefix is used instead). After skipping the first 4096 bytes, we include the file "sprites.inc" which will then put our sprite patterns starting at address 0x1000. 

Sprites.inc simply contains 16 bytes representing our pattern. Note that the '%'-character in ca65 denotes binary values similar to `0b` in `C`.

{% highlight ca65 %}
.byte %00000001
.byte %00000011
.byte %00000111
.byte %00001111
.byte %00011111
.byte %00111111
.byte %01111111
.byte %11111111
.byte %00000000
.byte %00000000
.byte %00000000
.byte %00000000
.byte %00000001
.byte %00000001
.byte %00000011
.byte %00001111
{% endhighlight %}

Finally, we just need to update our Makefile which now looks like this:
{% highlight Make %}

CC=cc65
CA=ca65
LD=ld65

SOURCES=${notdir ${wildcard src/*.c}}
ASSEMBLY_SOURCES=${notdir ${wildcard src/*.s}}
ASSEMBLIES=${addprefix target/assembly/,${SOURCES:.c=.s}}
OBJECTS=${addprefix target/objects/,${SOURCES:.c=.o}}
ASSEMBLY_OBJECTS=${addprefix target/objects/,${ASSEMBLY_SOURCES:.s=.o}}

all: target/a.nes

copy: src/*.s src/*.inc target/assembly
	cp src/*.s target/assembly/
	cp src/*.inc target/assembly/

target/objects/%.o: target/assembly/%.s target/objects
	${CA} -o $@ -t nes $<

target/assembly/%.s: src/%.c target/assembly
	${CC} $< -t nes -O3 -o $@

target/a.nes: copy ${OBJECTS} ${ASSEMBLY_OBJECTS}
	${LD} -t nes -o target/a.nes ${OBJECTS} ${ASSEMBLY_OBJECTS} nes.lib

target/assembly:
	mkdir -p target/assembly

target/objects:
	mkdir -p target/objects

clean:
	rm -rf target/*

.SECONDARY: ${ASSEMBLIES}
{% endhighlight %}

We've added `${ASSEMBLY_OBJECTS}` to the `ld65` command before `nes.lib`, ASSEMBLY_OBJECTS are all the object files compiled from `.s`-files.

### Colour palettes ###

Now that we have a pattern for a sprite, we need to create a colour palette. There are four palettes for sprites located at `0x3F11-0x3F13`, `0x3F15-0x3F17`, `0x3F19-0x3F1B` and `0x3F1D-0x3F1F`. These memory addresses are mapped to PPU RAM so we must populate the palettes from our code. The NES is using memory mapped IO which meanns that communication with IO devices (such as the PPU) is done by reading and writing to special CPU  memory locations. 

Writing to PPU RAM (also known as VRAM) is done via two registers located at PPUADDR (`0x2006`) and PPUDATA (`0x2007`). First, you have to load the PPU address you want to write into PPUADDR register and then write the actual data to PPUDATA register. Since VRAM uses 16 bit addresses we actually need to make two writes to set the address (high byte first). So, to write to the first sprite palette, we first have to make two writes to address `0x2006` with values `0x3F` followed by `0x11` (in order to set the VRAM pointer to `0x3F11`). Then we can write the actual values to address `0x2007`. Conveniently, after a write to `0x2007` the internal VRAM pointer will be incremented automatically by one so the next write will put a byte into VRAM address `0x2F12`.

{% highlight c %}
#include <nes.h>

char main() {
    PPU.vram.address = 0x3F;
    PPU.vram.address = 0x11;

    PPU.vram.data = 0x2A;
    PPU.vram.data = 0x00;
    PPU.vram.data = 0x12;
}
{% endhighlight %}

The header file `nes.h` is defined in `<cc65-root>/include/nes.h` and contains some useful constants. Here PPU points to a struct located at address `0x2000`. Take a look at the file for more details. The values we write to `PPU.vram.data` are the absolute colour values supported by the NES, those values can be found [here](http://wiki.nesdev.com/w/index.php/PPU_palettes}) but `0x2A` is a green colour and `0x12` is blue.

The only thing left now is to create the OAM entry for the sprite, OAM are located in a separate memory area in the PPU so we can't access it via the `0x2006` and `0x2007` registers. Instead, there are similar registers `OAMADDR` (`0x2003`) and `OAMDATA` (`0x2004`) for writing to the OAM memory. But it's also possible to transfer OAM via DMA (Direct Memory Access). By writing `XX` to register `0x4014` will cause 256 bytes between address `0xXX00-0xXXFF` in the CPU memory to be transferred to the OAM memory. So I'm going to create my OAM data at address `0x0200-0x02FF` (hoping that there is nothing important already there. Actually, `nes.lib` uses `0x0200-0x0500` for ppu memory write buffer but I'm not going to use that so we should be fine)  and then write `0x02` th `0x4014`

{% highlight c %}
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
{% endhighlight %}

We're defining a struct Sprite representing an OAM entry. An OAM entry is four bytes where the first byte is the `y` position, the second byte is the pattern to be used, the third byte defines some attributes and the fourth byte is the `x` position. The attributes byte is defined as follows:
```
76543210
||||||||
||||||++- Palette (4 to 7) of sprite
|||+++--- Unimplemented
||+------ Priority (0: in front of background; 1: behind background)
|+------- Flip sprite horizontally
+-------- Flip sprite vertically
```

The neat thing is that we can actually flip a sprite both horizontally and vertically which means that we can build our images (which consists of four sprites) by using only one actual pattern.

Running this in an emulator should show our sprite in the middle of the screen. I have tried it in both my own emulator and in Nestopia.

You can check out the code from this step using the git tag `putting-something-on-the-screen`
Next we're going to make the "helicopter" fall.
