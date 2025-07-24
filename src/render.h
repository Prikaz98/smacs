#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "editor.h"

#define RENDER_NOTIFICATION_LEN 256

enum LineNumberFormat {
    ABSOLUTE,
    RELATIVE,
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

    int leading;

    enum LineNumberFormat line_number_format;
    char *notification;
} Smacs;

void render_draw_text(Smacs *smacs, int x, int y, char *text);
void render_draw_smacs(Smacs *smacs);
void render_destroy_smacs(Smacs *smacs);

#endif
