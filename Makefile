PKGS=sdl2
PKG_FLAGS:=$(shell pkg-config --cflags $(PKGS))
PKG_LIBS:=$(shell pkg-config --libs $(PKGS))
SDL2_LIBS:=-lSDL2_ttf
CFLAGS:=-Wall -Wextra -std=c11 -pedantic -ggdb

build:
	$(CC) $(CFLAGS) $(PKG_FLAGS) -o smacs ./src/editor.c ./src/render.c ./src/main.c $(PKG_LIBS) $(SDL2_LIBS)
