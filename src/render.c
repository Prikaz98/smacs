#include <assert.h>
#include <sys/param.h>
#include <stdlib.h>

#include "common.h"
#include "render.h"
#include "utf8.h"

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color fg)
{
    SDL_Texture *texture = NULL;
    SDL_FRect rect = (SDL_FRect) {0.0, 0.0, 0.0, 0.0};
    SDL_Surface *surface = NULL;

    if (text == NULL) return;
    if (text_len == 0) return;

    surface = TTF_RenderText_Blended(smacs->font, text, text_len, fg);
    if (surface == NULL) {
        fprintf(stderr, "Render text (x %d y %d txt: %s len: %ld) cause: %s\n", x, y, text, text_len, SDL_GetError());
        exit(EXIT_FAILURE);
    }

    texture = SDL_CreateTextureFromSurface(smacs->renderer, surface);

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_DestroySurface(surface);
    SDL_RenderTexture(smacs->renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}

void render_append_file_path(StringBuilder *sb, char *path, char *home_dir, size_t home_dir_len)
{
    if (starts_with(path, home_dir)) {
        sb_append(sb, '~');
        sb_append_many(sb, &path[home_dir_len]);
    } else {
        sb_append_many(sb, path);
    }
}

int count_digits(size_t num)
{
    long tmp;
    int count;

    if (num == 0) {
        return 1;
    }

    count = 0;
    tmp = (long) num;

    while (tmp > 0) {
        tmp = tmp / 10;
        ++count;
    }

    return count;
}

void render_format_line_number_padding(char *buffer, size_t buffer_len, size_t num)
{
    int to_fill;

    memset(buffer, ' ', buffer_len);
    to_fill = ((int) buffer_len) - count_digits(num);
    sprintf(&buffer[to_fill], "%ld", num);
}

void render_format_display_line_number(Smacs *smacs, char *buffer, size_t buffer_len, size_t num, size_t cursor_line_num)
{
    switch (smacs->line_number_format) {
    case HIDE: {
    }break;
    case ABSOLUTE: {
        render_format_line_number_padding(buffer, buffer_len, num);
    }break;
    case RELATIVE: {
        if (cursor_line_num < num) {
            render_format_line_number_padding(buffer, buffer_len, num - cursor_line_num);
        } else if (cursor_line_num > num) {
            render_format_line_number_padding(buffer, buffer_len, cursor_line_num - num);
        } else {
            render_format_line_number_padding(buffer, buffer_len, cursor_line_num);
        }
    }break;
    }
}

void render_destroy_glyph(GlyphMatrix *glyph)
{
    GlyphRow *row;

    for (size_t li = 0; li < glyph->len; ++li) {
        row = &glyph->data[li];
        for (size_t ci = 0; ci < row->len; ++ci) {
            free(row->data[ci].str);
            row->data[ci].str = NULL;
        }

        gb_free(row);
        row = NULL;
    }

    gb_free(glyph);
}

void render_append_char_to_rendering(Smacs *smacs, StringBuilder *sb, char *data, size_t *i)
{
    size_t ci;
    int char_len;

    ci = *i;

    switch (data[ci]) {
    case '\t': {
        for (int tabs = 0; tabs < smacs->tab_size; ++tabs) sb_append(sb, ' ');
    } break;
    case '\n': {
        sb_append(sb, ' ');
    } break;
    default: {
        char_len = utf8_size_char(data[ci]);
        for (int i = 0; i < char_len; ++i) sb_append(sb, data[ci+i]);
        ci += (char_len-1);
    } break;
    }

    *i = ci;
}

GlyphItem *render_flush_item_sb_and_move_x(Smacs *smacs, GlyphRow *row, StringBuilder *sb, int *x, int y, GlyphItemEnum kind)
{
    int w, h, x_;

    x_ = *x;

    if (sb->len > 0) {
        TTF_GetStringSize(smacs->font, sb->data, sb->len, &w, &h);
        gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, x_, y, w, h, kind}));
        sb_clean(sb);
        x_ += w;
        *x = x_;
        return &row->data[row->len-1];
    }

    return NULL;
}

void render_update_glyph(Smacs *smacs)
{
    Arena arena;
    Line *lines;
    Line *line;
    size_t arena_end, cursor, region_beg, region_end, max_line_num, current_line, line_number_len, data_len, pi;
    int win_w, win_h, char_w, char_h, content_limit, common_indention, text_indention, pane_width_threashold;
    StringBuilder *sb;
    char *data, line_number[100];
    bool is_active_pane, show_line_number, mini_buffer_is_active;
    Pane *pane;
    GlyphMatrix *glyph;
    GlyphRow *row;

    render_destroy_glyph(&smacs->glyph);
    smacs->glyph = ((GlyphMatrix) {0});
    glyph = &smacs->glyph;
    glyph->len = 0;

    TTF_GetStringSize(smacs->font, ".", 1, &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);
    content_limit = win_h - (char_h * 2.5);

    sb = (&(StringBuilder) {0});

    show_line_number = true;
    if (smacs->line_number_format == HIDE) show_line_number = false;

    mini_buffer_is_active = false;
    if (smacs->editor.state & SEARCH) mini_buffer_is_active = true;
    if (smacs->editor.state & EXTEND_COMMAND) mini_buffer_is_active = true;
    if (smacs->editor.state & COMPLETION) mini_buffer_is_active = true;

    for (pi = 0; pi < smacs->editor.panes_len; ++pi) {
        pane = &smacs->editor.panes[pi];

        lines = pane->buffer->data;
        arena = pane->arena;
        data = pane->buffer->content.data;
        data_len = pane->buffer->content.len;
        is_active_pane = pane == smacs->editor.pane;

        cursor = pane->position;
        region_beg = MIN(smacs->editor.mark, cursor);
        region_end = MAX(smacs->editor.mark, cursor);
        assert(region_beg <= region_end);

        common_indention = pane->x;
        //extra pixel is needed to not cover the seporator line
        text_indention = common_indention + 1;
        pane_width_threashold = pane->x + pane->w - (char_w * 2);

        current_line = editor_get_current_line_number(pane) + 1;
        if (show_line_number) {
            max_line_num = arena.start + arena.show_lines;
            line_number_len = count_digits(max_line_num);
            render_format_line_number_padding(&(line_number[0]), line_number_len, max_line_num);
            line_number[line_number_len] = '\0';
            text_indention += (char_w * line_number_len);
        }

        arena_end = MIN(arena.start + arena.show_lines, pane->buffer->len);

        {
            size_t li, ci;
            int content_hight, x, w, h;
            bool selection;
            GlyphItemEnum kind;

            content_hight = 0;

            if(data_len == 0) {
                gb_append(row, ((GlyphItem) {NULL, 0, x, content_hight, char_w, char_h, CURSOR}));
            } else {
                assert(arena.start < arena_end);

                selection = is_active_pane && smacs->editor.state & SELECTION && region_beg != region_end;
                kind = TEXT;

                for (li = arena.start; li < arena_end; ++li) {
                    gb_append(glyph, ((GlyphRow){0}));
                    row = &glyph->data[glyph->len-1];

                    line = &lines[li];
                    x = text_indention;
                    if (content_limit <= content_hight) break;

                    if (show_line_number) {
                        render_format_display_line_number(smacs, line_number, line_number_len, li + 1, current_line);
                        TTF_GetStringSize(smacs->font, line_number, line_number_len, &w, &h);
                        gb_append(row, ((GlyphItem) {strdup(line_number), line_number_len, common_indention, content_hight, w, h, LINE_NUMBER}));
                    }

                    for (ci = line->start; ci <= line->end; ++ci) {
                        if (selection && (ci >= region_beg)) {
                            kind = kind | REGION;
                        }

                        if (kind & REGION && (ci >= region_end)) {
                            kind = kind ^ REGION;
                        }

                        if (is_active_pane && ci == cursor) {
                            kind = kind | CURSOR;

                            if (cursor == data_len) {
                                gb_append(row, ((GlyphItem) {NULL, 0, x, content_hight, char_w, char_h, kind}));
                                kind = kind ^ CURSOR;
                            }
                        } else if (kind & CURSOR) {
                            kind = kind ^ CURSOR;
                        }

                        if (x >= pane_width_threashold) {
                            gb_append(glyph, ((GlyphRow){0}));
                            row = &glyph->data[glyph->len-1];
                            content_hight += (char_h + smacs->leading);
                            x = text_indention;
                        }

                        if (ci < data_len) {
                            render_append_char_to_rendering(smacs, sb, data, &ci);
                            render_flush_item_sb_and_move_x(smacs, row, sb, &x, content_hight, kind);
                        }
                    }

                    //TODO: this code is needed for rendering widht region selection by according by width of window
                    //   but it does not work correcty because it access to any last element
                    //   and this last element might be a cursor
                    //   that brokes it showing

                    // if (selection && region_beg <= line->end && line->end < region_end && row->len > 0) {
                    //     row->data[row->len-1].w = pane_width_threashold - row->data[row->len-1].x;
                    // }

                    content_hight += (char_h + smacs->leading);
                    sb_clean(sb);
                }
            }

            gb_append(row, ((GlyphItem) {NULL, 0, pane->x+pane->w, 0, pane->x+pane->w, pane->h-char_h*2, LINE}));
        }

        //MODE LINE
        {
            int padding;
            GlyphItem *item;
            int mode_line_h;

            padding = common_indention;
            gb_append(glyph, ((GlyphRow){0}));
            row = &glyph->data[glyph->len-1];

            if(pane->buffer->need_to_save) {
                sb_append(sb, '*');
            }

            render_append_file_path(sb, pane->buffer->file_path, smacs->home_dir, strlen(smacs->home_dir));
            sb_append_many(sb, " (");
            sprintf(line_number, "%ld", current_line);
            sb_append_many(sb, line_number);
            sb_append(sb, ')');

            if (is_active_pane) {
                if (smacs->editor.state & SEARCH) {
                    sb_append(sb, ' ');
                    if (smacs->editor.state & BACKWARD_SEARCH) {
                        sb_append_many(sb, "Re-");
                    }

                    sb_append_many(sb, "Search[:enter next :C-g stop]");
                } else if (smacs->editor.state & EXTEND_COMMAND) {
                    sb_append_many(sb, " C-x");
                }
            }

            mode_line_h = win_h - (mini_buffer_is_active ? char_h*2 : char_h);

            item = render_flush_item_sb_and_move_x(smacs, row, sb, &padding, mode_line_h, is_active_pane ? MODE_LINE_ACTIVE : MODE_LINE);
            item->w = pane->w;
        }
    }

    //MINI BUFFER

    if (mini_buffer_is_active)
    {
        size_t home_dir_len;
        int completion_w, padding, complition_width_limit;
        char *completion_delimiter, *parens;
        GlyphRow *row;

        gb_append(glyph, ((GlyphRow){0}));
        row = &glyph->data[glyph->len-1];

        completion_delimiter = " | ";
        home_dir_len = strlen(smacs->home_dir);
        padding = char_w;
        complition_width_limit = win_w - char_w*10;

        if (smacs->editor.state & (SEARCH | EXTEND_COMMAND | COMPLETION)) {
            if (smacs->editor.state & COMPLETION) {
                if ((smacs->editor.state & _FILE) && (strcmp(smacs->editor.dir, ".") != 0)) {
                    sb_append_many(sb, "Find file: ");
                    render_append_file_path(sb, smacs->editor.dir, smacs->home_dir, home_dir_len);
                    sb_append(sb, '/');
                }

                if (smacs->editor.user_input.len > 0) {
                    sb_append_many(sb, smacs->editor.user_input.data);
                }

                parens = "{}";
                if (smacs->editor.completor.filtered.len == 1) {
                    parens = "[]";
                }

                sb_append(sb, parens[0]);
                for (size_t i = 0; i < smacs->editor.completor.filtered.len; ++i) {
                    render_append_file_path(sb, smacs->editor.completor.filtered.data[i], smacs->home_dir, home_dir_len);

                    if (i < (smacs->editor.completor.filtered.len-1)) {
                        sb_append_many(sb, completion_delimiter);
                    }

                    TTF_GetStringSize(smacs->font, sb->data, sb->len, &completion_w, NULL);

                    if (completion_w >= complition_width_limit) {
                        sb->len = sb->len - strlen(smacs->editor.completor.filtered.data[i]) - strlen(completion_delimiter);
                        memset(&sb->data[sb->len], 0, sb->cap - sb->len);
                        sb_append_many(sb, "...");
                        break;
                    }
                }

                sb_append(sb, parens[1]);
                render_flush_item_sb_and_move_x(smacs, row, sb, &padding, win_h - char_h, MINI_BUFFER);
            } else {
                if (smacs->editor.user_input.len > 0) {
                    sb_append_many(sb, smacs->editor.user_input.data);
                    render_flush_item_sb_and_move_x(smacs, row, sb, &padding, win_h - char_h, MINI_BUFFER);
                }
            }
        } else if (strlen(smacs->notification) > 0) {
            sb_append_many(sb, smacs->notification);
            render_flush_item_sb_and_move_x(smacs, row, sb, &padding, win_h - char_h, MINI_BUFFER);
        }
    }

    sb_free(sb);
}

void render_draw_batch(Smacs *smacs, GlyphItemEnum kind, StringBuilder *sb, SDL_FRect *rect)
{
    SDL_Color fg;

    if (kind & TEXT) {
        if (kind & REGION) {
            SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
            SDL_RenderFillRect(smacs->renderer, rect);
            fg = smacs->fg;
        } else if (kind & KEYWORD) {
            fg = smacs->kvfg;
        } else if (kind & TYPE) {
            fg = smacs->tpfg;
        } else {
            fg = smacs->fg;
        }

        if (kind & CURSOR) {
            SDL_SetRenderDrawColor(smacs->renderer, smacs->cfg.r, smacs->cfg.g, smacs->cfg.b, smacs->cfg.a);
            SDL_RenderFillRect(smacs->renderer, rect);
            fg = smacs->bg;
        }

        render_draw_text(smacs, rect->x, rect->y, sb->data, sb->len, fg);
    } else if (kind & MODE_LINE) {
        SDL_SetRenderDrawColor(smacs->renderer, smacs->bg.r, smacs->bg.g, smacs->bg.b, smacs->bg.a);
        SDL_RenderFillRect(smacs->renderer, rect);

        render_draw_text(smacs, rect->x, rect->y, sb->data, sb->len, smacs->fg);
    } else if (kind & MODE_LINE_ACTIVE) {
        SDL_SetRenderDrawColor(smacs->renderer, smacs->mlbg.r, smacs->mlbg.g, smacs->mlbg.b, smacs->mlbg.a);
        SDL_RenderFillRect(smacs->renderer, rect);

        render_draw_text(smacs, rect->x, rect->y, sb->data, sb->len, smacs->mlfg);
    } else if (kind & MINI_BUFFER) {
        render_draw_text(smacs, rect->x, rect->y, sb->data, sb->len, smacs->fg);
    } else if (kind & LINE_NUMBER) {
        render_draw_text(smacs, rect->x, rect->y, sb->data, sb->len, smacs->ln);
    } else if (kind & LINE) {
        SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
        SDL_RenderLine(smacs->renderer, rect->x, rect->y, rect->w, rect->h);
    }
}

void render_glyph_show(Smacs *smacs)
{
    GlyphItem *item;
    GlyphRow *row;
    SDL_FRect rect;
    int char_w, char_h;
    size_t ci;
    GlyphItemEnum prev_kind;
    StringBuilder *sb;

    rect = (SDL_FRect) {0.0, 0.0, 0.0, 0.0};
    sb = &(StringBuilder) {0};
    TTF_GetStringSize(smacs->font, "|", 1, &char_w, &char_h);

    for (size_t li = 0; li < smacs->glyph.len; ++li) {
        row = &smacs->glyph.data[li];
        if (row->len == 0) continue;

        for (ci = 0; ci < row->len; ++ci) {
            item = &row->data[ci];

            if (ci != 0 && item->kind == prev_kind) {
                if (item->len > 0) sb_append_many(sb, item->str);
                rect.w += item->w;
            } else {
                if (ci != 0) {
                    render_draw_batch(smacs, prev_kind, sb, &rect);
                    sb_clean(sb);
                }

                if (item->len > 0) sb_append_many(sb, item->str);
                prev_kind = item->kind;
                rect.x = item->x;
                rect.y = item->y;
                rect.h = item->h;
                rect.w = item->w;
            }
        }

        render_draw_batch(smacs, prev_kind, sb, &rect);
        sb_clean(sb);
    }
}

void render_destroy_smacs(Smacs *smacs)
{
    editor_destroy(&smacs->editor);
    render_destroy_glyph(&smacs->glyph);
    SDL_DestroyRenderer(smacs->renderer);
    SDL_DestroyWindow(smacs->window);
    free(smacs->notification);
    TTF_CloseFont(smacs->font);
}
