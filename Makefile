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
