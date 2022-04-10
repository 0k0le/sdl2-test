#
# Build file
#

CC = g++
CFLAGS = -DDEBUG -std=gnu++2a -Wall -Wextra -c -O2 -pedantic -DBASE_DIR=\"`pwd`/\" -o
LIBS = `pkg-config --libs sdl2` `pkg-config --libs x11` `pkg-config --libs xcb` `pkg-config --libs xcb-randr`

SRCDIR = src
OBJDIR = bin-int
BINDIR = bin

MAIN-CPP = ${SRCDIR}/main.cpp
MAIN-OBJ = ${OBJDIR}/main.o

OBJ = ${MAIN-OBJ}

build: prereq ${MAIN-OBJ}
	${CC} ${OBJ} ${LIBS} -o ${BINDIR}/sdl2-test

prereq:
	mkdir -p ${OBJDIR}
	mkdir -p ${BINDIR}

clean:
	rm -rf ${OBJDIR} ${BINDIR}

${MAIN-OBJ}: ${MAIN-CPP}
	${CC} ${MAIN-CPP} `pkg-config --cflags sdl2`  `pkg-config --cflags x11` `pkg-config --cflags xcb` `pkg-config --cflags xcb-randr` ${CFLAGS} ${MAIN-OBJ}
