PKGS=sdl2
PKG_FLAGS:=$(shell pkg-config --cflags $(PKGS))
PKG_LIBS:=$(shell pkg-config --libs $(PKGS))
SDL2_LIBS:=-lSDL2_ttf
CFLAGS:=-Wall -Wextra -std=c11 -pedantic -ggdb
SOURCES:=./src/common.c ./src/utf8.c ./src/editor.c ./src/themes.c ./src/render.c ./src/smacs.c
EXEC:=smacs
#TTF_FILE:=fonts/IosevkaFixed-Medium.ttf
TTF_FILE:=fonts/Consolas.ttf

build:
	PWD=$(shell pwd)
	mkdir -p .build
	sed 's+^#define TTF_PATH.*$$+#define TTF_PATH "${PWD}/${TTF_FILE}"+g' ./resources/template > ./.build/main.c
	$(CC) $(CFLAGS) $(PKG_FLAGS) -o $(EXEC) $(SOURCES) ./.build/main.c $(PKG_LIBS) $(SDL2_LIBS)
