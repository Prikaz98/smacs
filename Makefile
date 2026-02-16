PKGS=sdl3 sdl3-ttf
PKG_FLAGS:=$(shell pkg-config --cflags $(PKGS))
PKG_LIBS:=$(shell pkg-config --libs $(PKGS))
CFLAGS:=-Wall -Wextra -std=c11
DEV_CFLAGS:=-pedantic -ggdb -D_DEFAULT_SOURCE -fsanitize=address,undefined
SOURCES:=$(shell find ./src/ -type f -name "*.c")
EXEC:=smacs
TTF:=fonts/unifont-16.0.04.ttf
UNAME_S := $(shell uname -s)

ifeq ($(UNAME_S),Linux)
	CFLAGS += -D OS_LINUX
endif

default:
	PWD=$(shell pwd)
	mkdir -p .build
	cat ./resources/template | sed 's+^#define HOME .*$$+#define HOME "${HOME}"+g' | sed 's+^#define APP_DIR.*$$+#define APP_DIR "${PWD}"+g' | sed 's+^#define TTF.*$$+#define TTF "${TTF}"+g' > ./.build/main.c

dev: default
	$(CC) $(CFLAGS) $(DEV_CFLAGS) $(PKG_FLAGS) -o $(EXEC) $(SOURCES) ./.build/main.c $(PKG_LIBS)

prod: default
	$(CC) $(CFLAGS) $(PKG_FLAGS) -o $(EXEC) $(SOURCES) ./.build/main.c $(PKG_LIBS)

test:
	$(CC) $(CFLAGS) -o tokenize_test ./src/common.c ./src/tokenize.c ./test/tokenize_test.c
	./tokenize_test
	rm tokenize_test
