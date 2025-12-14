#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "editor.h"
#include "lexer.h"

#define RENDER_NOTIFICATION_LEN 256

typedef enum {
    TEXT             = 0x001,
    REGION           = 0x002,
    CURSOR           = 0x004,
    MODE_LINE        = 0x008,
    MODE_LINE_ACTIVE = 0x010,
    MINI_BUFFER      = 0x020,
    LINE_NUMBER      = 0x040,
    LINE             = 0x080,
    KEYWORD          = 0x100,
    TYPE             = 0x200,
} GlyphItemEnum;


typedef struct {
    char *str;
    size_t len;

    int x;
    int y;
    int w;
    int h;

    GlyphItemEnum kind;
} GlyphItem;

#define fprintf_item(std, it) fprintf(std, "GlyphItem(%s,%ld,%d,%d,%d,%d,%d)\n", (it)->str, (it)->len, (it)->x, (it)->y, (it)->w, (it)->h, (it)->kind);

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
    SDL_Color kvfg;
    SDL_Color tpfg;

    int leading;
    int tab_size;

    enum LineNumberFormat line_number_format;
    char *notification;
    char *home_dir;

    GlyphMatrix glyph;

    int char_h; int char_w;
} Smacs;

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color fg);
void render_draw_smacs(Smacs *smacs);
void render_destroy_smacs(Smacs *smacs);
void render_update_glyph(Smacs *smacs);
void render_glyph_show(Smacs *smacs);

#endif
