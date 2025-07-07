#include <assert.h>
#include <sys/param.h>
#include "render.h"
#include "common.h"

// TODO: Render only visible part of the file
// TODO: Add scroll option

void render_draw_text(Smacs *smacs, int x, int y, char *text)
{
    SDL_Texture *texture = NULL;
    SDL_Rect rect = (SDL_Rect) {0};

    if (text == NULL) return;

    if (strlen(text) > 0) {
        int text_width, text_height;

        SDL_Surface *surface = TTF_RenderText_Blended(smacs->font, text, smacs->fg);
        texture = SDL_CreateTextureFromSurface(smacs->renderer, surface);

        text_width = surface->w;
        text_height = surface->h;

        SDL_FreeSurface(surface);
        rect.x = x;
        rect.y = y;
        rect.w = text_width;
        rect.h = text_height;
    } else {
        rect.x = 0;
        rect.y = 0;
        rect.w = 0;
        rect.h = 0;
    }

    SDL_RenderCopy(smacs->renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}

void render_draw_cursor(Smacs *smacs, SDL_Rect cursor_rect, StringBuilder *sb)
{
    char *data;
    size_t cursor;

    data = smacs->editor.buffer.content.data;
    cursor = smacs->editor.position;

    SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
    SDL_RenderFillRect(smacs->renderer, &cursor_rect);

    SDL_Color tmp;
    tmp = smacs->fg;
    smacs->fg = smacs->bg;
    sb_append(sb, data[cursor] == '\n' ? ' ' : data[cursor]);
    render_draw_text(smacs, cursor_rect.x, cursor_rect.y, sb->data);
    smacs->fg = tmp;
}

void render_draw_modeline(Smacs *smacs)
{
    SDL_Rect mode_line;
    int win_w, win_h, char_w, char_h, padding_left;
    size_t current_line;
    char mode_line_info[1000] = {0};

    TTF_SizeText(smacs->font, "|", &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);

    mode_line = (SDL_Rect) {0, win_h - (char_h * 2), win_w, win_h};
    padding_left = char_w;
    current_line = editor_get_current_line_number(&smacs->editor);

    SDL_SetRenderDrawColor(smacs->renderer, smacs->bg.r, smacs->bg.g, smacs->bg.b, smacs->bg.a);
    SDL_RenderFillRect(smacs->renderer, &mode_line);

    SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
    SDL_RenderDrawLine(smacs->renderer, 0, mode_line.y, mode_line.w, mode_line.y);

    SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
    SDL_RenderDrawLine(smacs->renderer, 0, win_h - char_h, mode_line.w, win_h - char_h);

    //TODO: Move it in minibuffer rendering
    if (smacs->editor.search.searching) {
        render_draw_text(smacs, padding_left, win_h - char_h, smacs->editor.search.target.data);
    } else {
        render_draw_text(smacs, padding_left, win_h - char_h, smacs->notification);
    }

    sprintf(mode_line_info, "%s  (%ld)  %s%s", smacs->editor.buffer.file_path, current_line + 1, smacs->editor.search.reverse ? "Re-" : "", smacs->editor.search.searching ? "Search[:enter next :C-g stop]" : "");
    render_draw_text(smacs, padding_left, win_h - (char_h * 2), mode_line_info);
}

void render_draw_smacs(Smacs *smacs)
{
    Arena arena;
    Line *lines;
    Line line;
    size_t arena_end, li, ci, cursor, content_line_index, region_beg, region_end;
    int win_w, win_h, x, y, char_w, char_h;
    StringBuilder *sb;
    char *data;
    SDL_Rect cursor_rect, region_rect;
    bool is_line_region;

    lines = smacs->editor.buffer.lines;
    arena = smacs->editor.buffer.arena;
    data = smacs->editor.buffer.content.data;
    cursor = smacs->editor.position;
    region_beg = smacs->editor.mark < cursor ? smacs->editor.mark : cursor;
    region_end = smacs->editor.mark > cursor ? smacs->editor.mark : cursor;
    assert(region_beg <= region_end);

    TTF_SizeText(smacs->font, "|", &char_w, &char_h);
    cursor_rect = (SDL_Rect) {0, 0, char_w, char_h};
    region_rect = (SDL_Rect) {0, 0, char_w, char_h};

    x = 0;
    y = 0;
    arena_end = MIN(arena.start + arena.show_lines, smacs->editor.buffer.lines_count);
    if(smacs->editor.buffer.content.len > 0) {
        assert(arena.start < arena_end);

        li = 0;
        sb = &(StringBuilder) {0};

        SDL_GetWindowSize(smacs->window, &win_w, &win_h);
        win_w -= y; //PADDING

        content_line_index = arena.start;
        while ((li + arena.start) < arena_end) {
            line = lines[content_line_index++];

            is_line_region = smacs->editor.selection &&
                ((line.start <= region_beg && region_beg < line.end) ||
                 (line.start < region_end && region_end <= line.end) ||
                 (region_beg < line.start && region_end > line.end));

            if (is_line_region) {
                if (region_beg < line.start) {
                    region_rect.x = 0;
                }
            }

            for (ci = line.start; ci <= line.end; ci++) {
                if (is_line_region) {
                    if (region_beg == ci) {
                        TTF_SizeText(smacs->font, sb->data, &x, NULL);
                        region_rect.x = x;
                    }

                    if (region_end == ci) {
                        TTF_SizeText(smacs->font, sb->data, &x, NULL);
                        region_rect.w = x - region_rect.x;
                    }
                }

                if (cursor == ci) {
                    TTF_SizeText(smacs->font, sb->data, &x, &y);
                    if (y > 0) {
                        cursor_rect.x = x;
                        cursor_rect.y = li * y;
                    }
                }

                if (ci < line.end) {
                    sb_append(sb, data[ci]);
                    TTF_SizeText(smacs->font, sb->data, &x, &y);

                    if (win_w < x) {
                        if (is_line_region) {
                            region_rect.y = li * y;

                            if (region_end >= ci) {
                                TTF_SizeText(smacs->font, sb->data, &x, NULL);
                                region_rect.w = x - region_rect.x;
                            }

                            SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
                            SDL_RenderFillRect(smacs->renderer, &region_rect);
                            region_rect.w = 0;
                        }

                        render_draw_text(smacs, 0, li * y, sb->data);
                        sb_clean(sb);
                        li++;
                    }
                }
            }

            if (is_line_region) {
                region_rect.y = li * y;

                if (region_end >= line.end) {
                    TTF_SizeText(smacs->font, sb->data, &x, NULL);
                    region_rect.w = x - region_rect.x;
                }

                SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
                SDL_RenderFillRect(smacs->renderer, &region_rect);
            }

            render_draw_text(smacs, 0, li * y, sb->data);
            sb_clean(sb);
            li++;
        }

        render_draw_cursor(smacs, cursor_rect, sb);
        sb_free(sb);
    } else {
        SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
        SDL_RenderFillRect(smacs->renderer, &cursor_rect);
    }

    render_draw_modeline(smacs);
}

void render_destroy_smacs(Smacs *smacs)
{
    editor_destroy(&smacs->editor);
    SDL_DestroyRenderer(smacs->renderer);
    SDL_DestroyWindow(smacs->window);
    free(smacs->notification);
    TTF_CloseFont(smacs->font);
}
