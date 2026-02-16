#ifndef RENDER_H
#define RENDER_H

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include "editor.h"
#include "lexer.h"
#include "tokenize.h"

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
    STRING           = 0x400,

    COMMENT          = 0x1000,
} GlyphItemEnum;

typedef struct {
    size_t beg;
    size_t len;

    float x;
    float y;
    float w;
    float h;

    GlyphItemEnum kind;

    long position;
} GlyphItem;

#define fprintf_item(std, it) fprintf(std, "GlyphItem(%s,%ld,%f,%f,%f,%f,%d)\n", (it)->str, (it)->len, (it)->x, (it)->y, (it)->w, (it)->h, (it)->kind);

typedef struct {
    StringBuilder string_data;

    GlyphItem *data;
    size_t len;
    size_t cap;
} GlyphList;

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

    SDL_Color bg;   //background color
    SDL_Color fg;   //foreground color
    SDL_Color rg;   //region
    SDL_Color ln;   //line number
    SDL_Color mlbg; //modeline bg
    SDL_Color mlfg; //modeline fg
    SDL_Color cfg;  //todo(ivan): forget what is it lol???
    SDL_Color kvfg; //keyword fg
    SDL_Color tpfg; //type fg
    SDL_Color sfg;  //string fg
    SDL_Color cmfg;  //comment fg

    int leading;
    int tab_size;

    enum LineNumberFormat line_number_format;
    char *notification;
    char *home_dir;

    GlyphList glyph;
    Tokens tokenize;

    int char_h; int char_w;
} Smacs;

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color fg);
void render_draw_smacs(Smacs *smacs);
void render_destroy_smacs(Smacs *smacs);
void render_update_glyph(Smacs *smacs);
void render_glyph_show(Smacs *smacs);
long render_find_position_by_xy(Smacs *smacs, int x, int y);

#endif
