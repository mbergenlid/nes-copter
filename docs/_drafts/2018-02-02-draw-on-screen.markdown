---
layout: post
title:  "Putting something on the screen"
date:   2018-02-02 08:01:00 +0100
---

Rendering to the screen is handled by the Picture Processing Unit (PPU), the PPU has it's own internal memory called VRAM with an address space of 2^16 bytes (same as the CPU memory). Each frame, the PPU draws "whatever" is currently in the PPU memory. The PPU is controlled by a set of registers which are mapped to memory locations in the CPU memory.

That's a pretty vague explanation but let's make it concrete by creating a sprite and draw it on the screen.

![Sprite]({{ "/assets/sprite.png" | absolute_url }})
The PPU memory can hold 64 sprites, each occupying 4 bytes
...

So, the pattern for the image above could look like this

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

and the corresponding colour palette:

```
0 => Transparent
1 => Green
2 => Don't care
3 => Red
```

Splitting the pattern into the two planes gives

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

I'm going to use address `0x1000`-`0x1FFF` for sprite patterns which means I need to put those 16 bytes at PPU address `0x1000`. As mentioned in the [previous]({{ site.baseurl }}{% post_url 2018-02-01-getting-started %}), when examining the iNES file, address `0x000-0x1FFF` in the PPU memory are mapped to the CHR ROM bank which essentially means that we just need to put the sprite pattern into the iNES file in the correct location. How do we do that? To answer that we should try to understand how this iNES file is generated. For those of you who are not at all interested in that can just skip the [next section](#continuing).

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

The `MEMORY` section defines a number of memory areas. A memory area has a start address and a size, the start address denotes the the CPU memory address where this memory are is located at runtime. The optional file parameter for a memory area specifies which file to write this memory to (%O is the default file specified by the `-o` flag when invoking the linker). When the memory areas are written to the file (`target/a.nes` in our case) they are written in the order they appear in this configuration file.

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

### Continuing ###

Now that we have a pattern for a sprite, we need to create a colour palette. There are four palettes for sprites located at `0x3F11-0x3F13`, `0x3F15-0x3F17`, `0x3F19-0x3F1B` and `0x3F1D-0x3F1F`. These memory addresses are mapped to PPU RAM so we must populate the palettes from our code. The NES is using memory mapped IO which meanns that communication with IO devices is done by reading and writing to special CPU  memory locations. 

Writing to PPU RAM (also known as VRAM)is done via two registers located at CPU PPUADDR (`0x2006`) and PPUDATA (`0x2007`). First, you have to load the address you want to write to into  PPUADDR register and then write the actual data to PPUDATA register. Since VRAM uses 16 bit addresses we actually need to make two writes to set the address (high byte first). So, to write to the first sprite palette, we first have to make two writes to address `0x2006` with values `0x3F` followed by `0x11` (in order to set the VRAM pointer to `0x3F11`). Then we can write the actual values to address `0x2007`. Conveniently, after a write to `0x2007` the internal VRAM pointer will be incremented automatically by one so the next write to will put a byte into VRAM address `0x2F12`.

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

The header file `nes.h` is defined in `<cc65-root>/include/nes.h` and contains some useful constants. Here PPU points to a struct located at address `0x2000`. Take a look at the file for more details. The values we write to `PPU.vram.data` are the absolute colour values supported by the NES, those values can be found [here](http://wiki.nesdev.com/w/index.php/PPU_palettes})

The only thing left now is to create the OAM entry for the sprite, OAM are located in a separate memory area in the PPU so we can't access it via the `0x2006` and `0x2007` registers. Instead, there are similar registers `OAMADDR` (`0x2003`) and `OAMDATA` (`0x2004`) for writing to the OAM memory. But it's also possible to transfer OAM via DMA (Direct Memory Access). By writing `XX` to register `0x4014` will cause 256 bytes between address `0xXX00-0xXXFF` in the CPU memory to be transferred to the OAM memory. So I'm going to create my OAM data at address `0x0200-0x02FF` (hoping that there is nothing important already there :) and then write `0x02` to `0x4014`

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
    sprites[1].attributes = 0x40;

    sprites[2].y_position = y;
    sprites[2].x_position = x-8;
    sprites[2].pattern_index = 0;
    sprites[2].attributes = 0x80;

    sprites[3].y_position = y;
    sprites[3].x_position = x;
    sprites[3].pattern_index = 0;
    sprites[3].attributes = 0xC0;

    SPRITE_DMA = 0x02;

    PPU.control = 0x08;
    PPU.mask = 0x18;

    while(1);
    return 0;
}
{% endhighlight %}


