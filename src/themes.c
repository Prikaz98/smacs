#include <sys/param.h>

#include "themes.h"

SDL_Color
themes_as_color(unsigned int rgb)
{
    return (SDL_Color) {rgb >> 16, rgb >> 8 & 0xFF, (Uint8) rgb & 0xFFFF, 0};
}

SDL_Color
themes_sdl_color_brighter(SDL_Color color, float multipier)
{
    return (SDL_Color) {
        MIN(255, color.r * multipier),
        MIN(255, color.g * multipier),
        MIN(255, color.b * multipier),
        0,
    };
}

void
themes_mindre(Smacs *smacs)
{
    smacs->bg = themes_as_color(0xF5F5F5);
    smacs->fg = themes_as_color(0x2E3331);
    smacs->rg = themes_as_color(0xCFD8DC);
    smacs->ln = smacs->rg;
    smacs->mlbg = themes_as_color(0xECEFF1);
    smacs->mlfg = themes_as_color(0x2E3331);
    smacs->cfg = themes_as_color(0x2E3331);
    smacs->kvfg = smacs->fg;
    smacs->tpfg = smacs->fg;
    smacs->sfg = smacs->fg;
    smacs->cmfg = smacs->fg;
}

void
themes_naysayer(Smacs *smacs)
{
    smacs->bg = themes_as_color(0x062329);
    smacs->fg = themes_as_color(0xD1B897);
    smacs->rg = themes_as_color(0x0000FF);
    smacs->ln = themes_sdl_color_brighter(smacs->bg, 2);
    smacs->mlbg = themes_as_color(0xD1B897);
    smacs->mlfg = themes_as_color(0x062329);
    smacs->cfg = themes_as_color(0xFFFFFF);
    smacs->kvfg = themes_as_color(0xFFFFFF);
    smacs->tpfg = themes_as_color(0x8cde94);
    smacs->sfg = smacs->fg;
    smacs->cmfg = smacs->fg;
}

void
themes_acme(Smacs *smacs)
{
    smacs->bg = themes_as_color(0xFFFFE8);
    smacs->fg = themes_as_color(0x444444);
    smacs->rg = themes_as_color(0xE8EB98);
    smacs->ln = themes_sdl_color_brighter(smacs->fg, 2);
    smacs->mlbg = themes_as_color(0xE1FAFF);
    smacs->mlfg = smacs->fg;
    smacs->cfg = smacs->fg;
    smacs->kvfg = smacs->fg;
    smacs->tpfg = smacs->fg;
    smacs->sfg = smacs->fg;
    smacs->cmfg = smacs->fg;
}

void
themes_jblow_nastalgia(Smacs *smacs)
{
    smacs->bg = themes_as_color(0x292929);
    smacs->fg = themes_as_color(0xD3B58D);
    smacs->rg = themes_as_color(0x0000FF);
    smacs->ln = themes_sdl_color_brighter(smacs->fg, 2);
    smacs->mlbg = smacs->fg;
    smacs->mlfg = smacs->bg;
    smacs->cfg = smacs->fg;
    smacs->kvfg = smacs->fg;
    smacs->tpfg = smacs->fg;
    smacs->sfg = themes_as_color(0xBEBEBE);
    smacs->cmfg = themes_as_color(0xFFFF00);
}
