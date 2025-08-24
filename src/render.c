#include <assert.h>
#include <sys/param.h>

#include "common.h"
#include "render.h"
#include "utf8.h"

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color fg)
{
    SDL_Texture *texture = NULL;
    SDL_Rect rect = (SDL_Rect) {0};
    SDL_Surface *surface = NULL;

    if (text == NULL) return;
    if (text_len == 0) return;

    surface = TTF_RenderUTF8_Blended(smacs->font, text, fg);
    if (surface == NULL) {
        fprintf(stderr, "Render text (x %d y %d txt: %s len: %ld) cause: %s\n", x, y, text, text_len, SDL_GetError());
        exit(EXIT_FAILURE);
    }

    texture = SDL_CreateTextureFromSurface(smacs->renderer, surface);

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(smacs->renderer, texture, NULL, &rect);
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
    } break;
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

void render_update_glyph(Smacs *smacs)
{
    Arena arena;
    Line *lines;
    Line *line;
    size_t arena_end, cursor, region_beg, region_end, max_line_num, current_line, line_number_len, data_len, pi, li, ci;
    int win_w, win_h, x, y, char_w, char_h, content_hight, content_limit, common_indention, text_indention, w, h, pane_w;
    StringBuilder *sb;
    char *data, line_number[100];
    bool is_active_pane, show_line_number, selection, is_region;
    Pane *pane;
    GlyphMatrix *glyph;
    GlyphRow *row;

    render_destroy_glyph(&smacs->glyph);
    smacs->glyph = ((GlyphMatrix) {0});
    glyph = &smacs->glyph;
    glyph->len = 0;

    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);
    content_limit = win_h - (char_h * 2.5);

    sb = (&(StringBuilder) {0});

    show_line_number = true;
    if (smacs->line_number_format == HIDE) show_line_number = false;

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
        text_indention = common_indention;
        pane_w = pane->w - (char_w * 2);

        current_line = editor_get_current_line_number(pane) + 1;
        if (show_line_number) {
            max_line_num = arena.start + arena.show_lines;
            line_number_len = count_digits(max_line_num);
            render_format_line_number_padding(&(line_number[0]), line_number_len, max_line_num);
            line_number[line_number_len] = '\0';
            text_indention += (char_w * line_number_len);
        }

        x = 0;
        y = char_h;

        arena_end = MIN(arena.start + arena.show_lines, pane->buffer->len);

        if(data_len > 0) {
            assert(arena.start < arena_end);
            content_hight = 0;

            selection = is_active_pane && smacs->editor.state & SELECTION && region_beg != region_end;
            is_region = false;

            for (li = arena.start; li < arena_end; ++li) {
                gb_append(glyph, ((GlyphRow){0}));
                row = &glyph->data[li - arena.start];

                line = &lines[li];
                x = 0;

                if (content_limit <= content_hight) break;

                if (show_line_number) {
                    render_format_display_line_number(smacs, line_number, line_number_len, li + 1, current_line);
                    TTF_SizeUTF8(smacs->font, line_number, &w, &h);
                    gb_append(row, ((GlyphItem) {strdup(line_number), line_number_len, common_indention, content_hight, w, h, LINE_NUMBER}));
                }

                for (ci = line->start; ci <= line->end; ++ci) {
                    if (selection && (ci > region_beg && ci < region_end)) {
                        is_region = true;
                    }

                    if (selection && (ci == region_beg || ci == region_end)) {
                        if (sb->len > 0) {
                            TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                            gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, text_indention + x, content_hight, ci == region_end ? w : pane_w, h, is_region ? REGION : TEXT}));
                            x += w;
                            sb_clean(sb);
                        }
                        is_region = ci == region_beg;
                    }

                    if (is_active_pane && cursor == ci) {
                        if (sb->len > 0) {
                            TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                            gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, text_indention + x, content_hight, pane_w, h, is_region ? REGION : TEXT}));
                            x += w;
                            sb_clean(sb);
                        }

                        if (ci == data_len) {
                            gb_append(row, ((GlyphItem) {NULL, 0, text_indention + x, content_hight, char_w, char_h, CURSOR}));
                        } else {
                            render_append_char_to_rendering(smacs, sb, data, &ci);
                            TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                            gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, text_indention + x, content_hight, w, h, CURSOR}));
                            x += w;
                            sb_clean(sb);
                        }
                    } else {
                        if (sb->len > 0) {
                            if (pane_w <= (x+char_w*((int)sb->len+text_indention))) {
                                TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                                if (pane_w <= (x+w+text_indention)) {
                                    gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, text_indention + x, content_hight, pane_w, h, is_region ? REGION : TEXT}));
                                    content_hight += (y + smacs->leading);
                                    x = 0;
                                    sb_clean(sb);
                                }
                            }
                        }

                        if (ci < data_len) {
                            render_append_char_to_rendering(smacs, sb, data, &ci);
                        }
                    }
                }

                if (sb->len > 0) {
                    gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, text_indention + x, content_hight, pane_w, char_h, is_region ? REGION : TEXT}));
                }

                content_hight += (y + smacs->leading);
                sb_clean(sb);
            }

            gb_append(row, ((GlyphItem) {NULL, 0, pane->w, 0, pane->w, pane->h-char_h*2, LINE}));
        }

        //MODE LINE
        {
            int padding;

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

            gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, padding, win_h - (char_h*2), pane->w, h, is_active_pane ? MODE_LINE_ACTIVE : MODE_LINE}));
            sb_clean(sb);
        }
    }

    //MINI BUFFER
    {
        size_t home_dir_len;
        int completion_w, padding;
        char *completion_delimiter;
        gb_append(glyph, ((GlyphRow){0}));
        row = &glyph->data[glyph->len-1];

        completion_delimiter = " | ";
        home_dir_len = strlen(smacs->home_dir);
        padding = char_w;

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

                sb_append(sb, '{');
                for (size_t i = 0; i < smacs->editor.completor.filtered.len; ++i) {
                    render_append_file_path(sb, smacs->editor.completor.filtered.data[i], smacs->home_dir, home_dir_len);

                    if (i < (smacs->editor.completor.filtered.len-1)) {
                        sb_append_many(sb, completion_delimiter);
                    }

                    TTF_SizeUTF8(smacs->font, sb->data, &completion_w, NULL);

                    if (completion_w >= win_w) {
                        sb->len = sb->len - strlen(smacs->editor.completor.filtered.data[i]) - strlen(completion_delimiter);
                        memset(&sb->data[sb->len], 0, sb->cap - sb->len);
                        sb_append_many(sb, "...");
                        break;
                    }
                }

                sb_append(sb, '}');
                TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, padding, win_h - char_h, w, h, MINI_BUFFER}));
            } else {
                if (smacs->editor.user_input.len > 0) {
                    sb_append_many(sb, smacs->editor.user_input.data);
                }
                TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
                gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, padding, win_h - char_h, w, h, MINI_BUFFER}));
            }
        } else if (strlen(smacs->notification) > 0) {
            sb_append_many(sb, smacs->notification);
            TTF_SizeUTF8(smacs->font, sb->data, &w, &h);
            gb_append(row, ((GlyphItem) {strdup(sb->data), sb->len, padding, win_h - char_h, w, h, MINI_BUFFER}));
        }

        sb_clean(sb);
    }


    sb_free(sb);
}

void render_glyph_show(Smacs *smacs)
{
    GlyphItem *item;
    GlyphRow *row;
    SDL_Rect rect;
    int char_w, char_h;
    size_t ci;

    rect = (SDL_Rect) {0, 0, 0, 0};
    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);

    for (size_t li = 0; li < smacs->glyph.len; ++li) {
        row = &smacs->glyph.data[li];

        for (ci = 0; ci < row->len; ++ci) {
            item = &row->data[ci];
            rect.x = item->x;
            rect.y = item->y;
            rect.h = item->h;
            rect.w = item->w;

            switch(item->kind) {
            case CURSOR: {
                SDL_SetRenderDrawColor(smacs->renderer, smacs->cfg.r, smacs->cfg.g, smacs->cfg.b, smacs->cfg.a);
                SDL_RenderFillRect(smacs->renderer, &rect);

                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->bg);
            } break;
            case REGION: {
                SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
                SDL_RenderFillRect(smacs->renderer, &rect);

                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->fg);
            } break;
            case MODE_LINE: {
                SDL_SetRenderDrawColor(smacs->renderer, smacs->bg.r, smacs->bg.g, smacs->bg.b, smacs->bg.a);
                SDL_RenderFillRect(smacs->renderer, &rect);

                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->fg);
            } break;
            case MODE_LINE_ACTIVE: {
                SDL_SetRenderDrawColor(smacs->renderer, smacs->mlbg.r, smacs->mlbg.g, smacs->mlbg.b, smacs->mlbg.a);
                SDL_RenderFillRect(smacs->renderer, &rect);

                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->mlfg);
            } break;
            case MINI_BUFFER: {
                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->fg);
            } break;
            case LINE_NUMBER: {
                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->ln);
            } break;
            case TEXT: {
                render_draw_text(smacs, item->x, item->y, item->str, item->len, smacs->fg);
            } break;
            case LINE: {
                SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
                SDL_RenderDrawLine(smacs->renderer, rect.x, rect.y, rect.w, rect.h);
            } break;
            }
        }
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
