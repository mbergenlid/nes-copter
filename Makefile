CC=cc65
CA=ca65
LD=ld65

SOURCES=${notdir ${wildcard src/*.c}}
ASSEMBLY_SOURCES=${notdir ${wildcard src/*.s}}
ASSEMBLIES=${addprefix target/assembly/,${SOURCES:.c=.s}}
OBJECTS=${addprefix target/objects/,${SOURCES:.c=.o}}
ASSEMBLY_OBJECTS=${addprefix target/objects/,${ASSEMBLY_SOURCES:.s=.o}}

all: target/a.nes

copy: src/*.s target/assembly
	cp src/*.s target/assembly/

target/objects/%.o: target/assembly/%.s target/objects
	${CA} -o $@ -t nes $<

target/assembly/%.s: src/%.c target/assembly
	${CC} $< -t nes -O3 -o $@

target/a.nes: copy ${OBJECTS} ${ASSEMBLY_OBJECTS}
	${LD} -t nes -o target/a.nes ${OBJECTS} nes.lib ${ASSEMBLY_OBJECTS} 

target/assembly:
	mkdir -p target/assembly

target/objects:
	mkdir -p target/objects

clean:
	rm -rf target/*

.SECONDARY: ${ASSEMBLIES}
