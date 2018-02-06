---
layout: post
title:  "Getting Started"
date:   2018-02-01 13:41:00 +0100
---

The CPU in the NES is a version of a [6502 microprocesor](https://en.wikipedia.org/wiki/MOS_Technology_6502). [CC65](http://cc65.github.io/doc) is a C compiler and linker for this architecture which I'm going to use. To install it, follow the instructions [here](http://cc65.github.io/cc65/getting-started.html)

Now, when we have a compiler, let's create our project structure and try to use the CC65 toolchain to compile some C-code.

{% highlight bash %}
$ tree
.
├── Makefile
├── src
│   ├── main.c
│   └── test.c
└── target
{% endhighlight %}

Pretty straightforward, sources goes into the `src` folder and the compiled binaries ends up in the `target` folder. I have created two simple source files `main.c` and `test.c` just to verify that we can cmpile something. They look like this:

**main.c**
{% highlight c %}
extern char fastcall add(char, char);

char main() {
    char res = add(1,2);
    return res;
}
{% endhighlight %}

**test.c**
{% highlight c %}
char fastcall add(char a, char b);

char add(char a, char b) {
    return a+b;
}
{% endhighlight %}

So the main function is just calling `add(1, 2)` and returns the result. The `Makefile` looks like this:

{% highlight make %}
CC=cc65
CA=ca65
LD=ld65

SOURCES=${notdir ${wildcard src/*}}
ASSEMBLIES=${addprefix target/assembly/,${SOURCES:.c=.s}}
OBJECTS=${addprefix target/objects/,${SOURCES:.c=.o}}

all: target/a.nes

target/objects/%.o: target/assembly/%.s target/objects
	${CA} -o $@ -t nes $<

target/assembly/%.s: src/%.c target/assembly
	${CC} $< -t nes -O3 -o $@

target/a.nes: ${OBJECTS}
	${LD} -t nes -o target/a.nes ${OBJECTS} nes.lib

target/assembly:
	mkdir -p target/assembly

target/objects:
	mkdir -p target/objects

clean:
	rm -rf target/*

.SECONDARY: ${ASSEMBLIES}
{% endhighlight %}

The file above should be pretty easy to understand, refer to the [Make documentation](https://www.gnu.org/software/make/manual/make.html) for any questions. I should probably say that I am in no way an expert in Make and this task could probably be done in a much better way but this Makefile serves the purpose for now.

So, let's run it:
{% highlight bash %}
$ make
mkdir -p target/assembly
mkdir -p target/objects
cc65 src/main.c -t nes -O3 -o target/assembly/main.s
ca65 -o target/objects/main.o -t nes target/assembly/main.s
cc65 src/test.c -t nes -O3 -o target/assembly/test.s
ca65 -o target/objects/test.o -t nes target/assembly/test.s
ld65 -t nes -o target/a.nes target/objects/main.o target/objects/test.o nes.lib
{% endhighlight %}

It compiles both sources into `.s` files which contain 6502 assembler which are then in turn converted to object files. All object files are then linked into `target/a.nes` which is a ROM in [iNES](http://wiki.nesdev.com/w/index.php/INES) format.

To me, someone who has been spending a lot of time deep diving into NES emulator development, this file looks like a correct iNES ROM file which is promising. Looking at the first 16 bytes of it shows:

{% highlight bash %}
$ head -c 16 target/a.nes |xxd
00000000: 4e45 531a 0201 0300 0000 0000 0000 0000  NES.............
{% endhighlight %}

The first four bytes is a the characters "NES" followed by MS-DOS end-of-file character. After that comes the number of 16Kb PRG ROM banks (`2` in this case), this is where our code will end up and these banks will be mapped to address `0x8000-0xFFFF`. Next comes the number of 8Kb CHR ROM banks (`1` in this case). CHR ROM stands for Character ROM and is used for storing sprite and background information. These banks will be mapped to address `0x0000-0x1FFF` in the PPU memory. Then we have a bunch of other information that are not so important right now, you can read the wiki link above for more details.

 But just to be sure it's correct, I loaded it into my own emulator which has a debugger and stepped through the code.and I can see that our code above is in fact running. I won't go into more details on how to run my emulator here, that's a topic for another post, instead I want to get something displayed on the screen.
