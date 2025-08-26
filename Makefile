PKGS=sdl2
PKG_FLAGS:=$(shell pkg-config --cflags $(PKGS))
PKG_LIBS:=$(shell pkg-config --libs $(PKGS))
SDL2_LIBS:=-lSDL2_ttf
CFLAGS:=-Wall -Wextra -std=c11 -pedantic -ggdb -D_DEFAULT_SOURCE #-fsanitize=address,undefined
SOURCES:=./src/common.c ./src/utf8.c ./src/editor.c ./src/themes.c ./src/render.c ./src/lexer.c ./src/smacs.c
EXEC:=smacs
TTF:=fonts/IosevkaFixed-Medium.ttf
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	CFLAGS += -D OS_LINUX
endif

build:
	PWD=$(shell pwd)
	mkdir -p .build
	cat ./resources/template | sed 's+^#define HOME .*$$+#define HOME "${HOME}"+g' | sed 's+^#define APP_DIR.*$$+#define APP_DIR "${PWD}"+g' | sed 's+^#define TTF.*$$+#define TTF "${TTF}"+g' > ./.build/main.c
	$(CC) $(CFLAGS) $(PKG_FLAGS) -o $(EXEC) $(SOURCES) ./.build/main.c $(PKG_LIBS) $(SDL2_LIBS)
