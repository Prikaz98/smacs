#ifndef RENDER_H
#define RENDER_H

#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>
#include "editor.h"

typedef struct {
    SDL_Window *window;
    SDL_Renderer *renderer;
    TTF_Font *font;

    Editor editor;

    SDL_Color bg;
    SDL_Color fg;
    SDL_Color rg;
} Smacs;

void render_draw_text(Smacs *smacs, int x, int y, char *text);
void render_draw_smacs(Smacs *smacs);

#endif
