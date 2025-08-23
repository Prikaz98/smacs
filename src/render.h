#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "editor.h"

#define RENDER_NOTIFICATION_LEN 256

enum GlyphItemEnum {
    TEXT,
    REGION,
    CURSOR,
    MODE_LINE,
    MODE_LINE_ACTIVE,
    MINI_BUFFER,
    LINE_NUMBER,
    LINE,
};

typedef struct {
    char *str;
    size_t len;

    int x;
    int y;
    int w;
    int h;

    enum GlyphItemEnum kind;
} GlyphItem;

typedef struct {
    GlyphItem *data;
    size_t len;
    size_t cap;
} GlyphRow;

typedef struct {
    GlyphRow *data;
    size_t len;
    size_t cap;
} GlyphMatrix;

enum LineNumberFormat {
    ABSOLUTE,
    RELATIVE,
    HIDE,
};

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;
    int font_size;

    Editor editor;

    SDL_Color bg;
    SDL_Color fg;
    SDL_Color rg;
    SDL_Color ln;
    SDL_Color mlbg;
    SDL_Color mlfg;
    SDL_Color cfg;

    int leading;
    int tab_size;

    enum LineNumberFormat line_number_format;
    char *notification;
    char *home_dir;

    GlyphMatrix glyph;
} Smacs;

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color fg);
void render_draw_smacs(Smacs *smacs);
void render_destroy_smacs(Smacs *smacs);
void render_update_glyph(Smacs *smacs);
void render_glyph_show(Smacs *smacs);

#endif
