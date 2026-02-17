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
    smacs->background_color = themes_as_color(0xF5F5F5);
    smacs->foreground_color = themes_as_color(0x2E3331);
    smacs->region_color = themes_as_color(0xCFD8DC);
    smacs->line_number_color = smacs->region_color;
    smacs->mode_line_background_color = themes_as_color(0xECEFF1);
    smacs->mode_line_foreground_color = themes_as_color(0x2E3331);
    smacs->cursor_foreground_color = themes_as_color(0x2E3331);
    smacs->number_foreground_color = smacs->foreground_color;
    smacs->type_foreground_color = smacs->foreground_color;
    smacs->string_foreground_color = smacs->foreground_color;
    smacs->comment_foreground_color = smacs->foreground_color;
}

void
themes_naysayer(Smacs *smacs)
{
    smacs->background_color = themes_as_color(0x062329);
    smacs->foreground_color = themes_as_color(0xD1B897);
    smacs->region_color = themes_as_color(0x0000FF);
    smacs->line_number_color = themes_sdl_color_brighter(smacs->background_color, 2);
    smacs->mode_line_background_color = themes_as_color(0xD1B897);
    smacs->mode_line_foreground_color = themes_as_color(0x062329);
    smacs->cursor_foreground_color = themes_as_color(0xFFFFFF);
    smacs->number_foreground_color = themes_as_color(0xFFFFFF);
    smacs->type_foreground_color = themes_as_color(0x8cde94);
    smacs->string_foreground_color = smacs->foreground_color;
    smacs->comment_foreground_color = smacs->foreground_color;
}

void
themes_acme(Smacs *smacs)
{
    smacs->background_color = themes_as_color(0xFFFFE8);
    smacs->foreground_color = themes_as_color(0x444444);
    smacs->region_color = themes_as_color(0xE8EB98);
    smacs->line_number_color = themes_sdl_color_brighter(smacs->foreground_color, 2);
    smacs->mode_line_background_color = themes_as_color(0xE1FAFF);
    smacs->mode_line_foreground_color = smacs->foreground_color;
    smacs->cursor_foreground_color = smacs->foreground_color;
    smacs->number_foreground_color = smacs->foreground_color;
    smacs->type_foreground_color = smacs->foreground_color;
    smacs->string_foreground_color = smacs->foreground_color;
    smacs->comment_foreground_color = smacs->foreground_color;
}

void
themes_jblow_nastalgia(Smacs *smacs)
{
    smacs->background_color = themes_as_color(0x292929);
    smacs->foreground_color = themes_as_color(0xD3B58D);
    smacs->region_color = themes_as_color(0x0000FF);
    smacs->line_number_color = themes_sdl_color_brighter(smacs->foreground_color, 2);
    smacs->mode_line_background_color = smacs->foreground_color;
    smacs->mode_line_foreground_color = smacs->background_color;
    smacs->cursor_foreground_color = smacs->foreground_color;
    smacs->number_foreground_color = themes_as_color(0xD699B5);
    smacs->type_foreground_color = smacs->foreground_color;
    smacs->string_foreground_color = themes_as_color(0xBEBEBE);
    smacs->comment_foreground_color = themes_as_color(0xFFFF00);
}
