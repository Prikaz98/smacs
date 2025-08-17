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

    texture = SDL_CreateTextureFromSurface(smacs->renderer, surface);

    rect.x = x;
    rect.y = y;
    rect.w = surface->w;
    rect.h = surface->h;

    SDL_FreeSurface(surface);
    SDL_RenderCopy(smacs->renderer, texture, NULL, &rect);
    SDL_DestroyTexture(texture);
}

void render_draw_cursor(Smacs *smacs, Pane pane, SDL_Rect cursor_rect, StringBuilder *sb)
{
    char *data;
    size_t cursor, char_len, content_len;
    register size_t i;

    data = pane.buffer->content.data;
    content_len = pane.buffer->content.len;
    cursor = pane.position;

    SDL_SetRenderDrawColor(smacs->renderer, smacs->cfg.r, smacs->cfg.g, smacs->cfg.b, smacs->cfg.a);
    SDL_RenderFillRect(smacs->renderer, &cursor_rect);

    if (cursor >= content_len) return;

    char_len = utf8_size_char(data[cursor]);
    switch (data[cursor]) {
    case '\t':
        sb_append_many(sb, "»");
        break;
    default:
        for (i = 0; i < char_len; ++i) {
            sb_append(sb, data[cursor + i] == '\n' ? ' ' : data[cursor + i]);
        }
        break;
    }

    render_draw_text(smacs, cursor_rect.x, cursor_rect.y, sb->data, sb->len, smacs->bg);
}

void render_draw_mini_buffer(Smacs *smacs)
{
    SDL_Rect mini_buffer_cursor, mini_buffer_area;
    int win_w, win_h, char_w, char_h, mini_buffer_padding, completion_w;
    StringBuilder *mini_buffer_content;
    size_t home_dir_len;
    register size_t i;
    char *completion_delimiter;
    bool need_to_draw;

    need_to_draw = true;
    if ((smacs->editor.state & NONE) && strlen(smacs->notification) == 0) need_to_draw = false;
    if (!need_to_draw) return;

    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);

    mini_buffer_content = &(StringBuilder) {0};
    mini_buffer_cursor = (SDL_Rect) {0, win_h - char_h, 2, char_h};
    mini_buffer_area = (SDL_Rect) {0, win_h - (char_h * 1), win_w, char_h};
    mini_buffer_padding = char_w;
    completion_delimiter = " | ";
    home_dir_len = strlen(smacs->home_dir);

    SDL_SetRenderDrawColor(smacs->renderer, smacs->bg.r, smacs->bg.g, smacs->bg.b, smacs->bg.a);
    SDL_RenderFillRect(smacs->renderer, &mini_buffer_area);

    if (smacs->editor.state & (SEARCH | EXTEND_COMMAND | COMPLETION)) {
        if (smacs->editor.state & COMPLETION) {
            if ((smacs->editor.state & _FILE) && (strcmp(smacs->editor.dir, ".") != 0)) {
                sb_append_many(mini_buffer_content, smacs->editor.dir);
                sb_append(mini_buffer_content, '/');
            }

            if (smacs->editor.user_input.len > 0) {
                sb_append_many(mini_buffer_content, smacs->editor.user_input.data);
            }

            TTF_SizeUTF8(smacs->font, mini_buffer_content->data, &mini_buffer_cursor.x, NULL);

            sb_append(mini_buffer_content, '{');
            for (i = 0; i < smacs->editor.completor.filtered.len; ++i) {
                if (starts_with(smacs->editor.completor.filtered.data[i], smacs->home_dir)) {
                    sb_append(mini_buffer_content, '~');
                    sb_append_many(mini_buffer_content, &smacs->editor.completor.filtered.data[i][home_dir_len]);
                } else {
                    sb_append_many(mini_buffer_content, smacs->editor.completor.filtered.data[i]);
                }

                if (i < (smacs->editor.completor.filtered.len-1)) {
                    sb_append_many(mini_buffer_content, completion_delimiter);
                }

                TTF_SizeUTF8(smacs->font, mini_buffer_content->data, &completion_w, NULL);

                if (completion_w >= win_w) {
                    mini_buffer_content->len = mini_buffer_content->len - strlen(smacs->editor.completor.filtered.data[i]) - strlen(completion_delimiter);
                    memset(&mini_buffer_content->data[mini_buffer_content->len], 0, mini_buffer_content->cap - mini_buffer_content->len);
                    sb_append_many(mini_buffer_content, "...");
                    break;
                }
            }

            sb_append(mini_buffer_content, '}');

            render_draw_text(smacs, mini_buffer_padding, win_h - char_h, mini_buffer_content->data, mini_buffer_content->len, smacs->fg);
        } else {
            if (smacs->editor.user_input.len > 0) {
                sb_append_many(mini_buffer_content, smacs->editor.user_input.data);
            }

            render_draw_text(smacs, mini_buffer_padding, win_h - char_h, mini_buffer_content->data, mini_buffer_content->len, smacs->fg);
            TTF_SizeUTF8(smacs->font, mini_buffer_content->data, &mini_buffer_cursor.x, NULL);
        }

        mini_buffer_cursor.x = mini_buffer_cursor.x + mini_buffer_padding;

        SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
        SDL_RenderFillRect(smacs->renderer, &mini_buffer_cursor);
    } else {
        sb_append_many(mini_buffer_content, smacs->notification);
        render_draw_text(smacs, mini_buffer_padding, win_h - char_h, mini_buffer_content->data, mini_buffer_content->len, smacs->fg);
    }

    sb_free(mini_buffer_content);
}

void render_draw_modeline(Smacs *smacs, Pane pane, bool is_active_pane)
{
    SDL_Rect mode_line;
    int win_w, win_h, char_w, char_h, mode_line_padding;
    StringBuilder *mode_line_info;
    SDL_Color mlbg, mlfg;

    mode_line_info = (&(StringBuilder) {0});
    mlbg = smacs->mlbg;
    mlfg = smacs->mlfg;
    if (!is_active_pane) {
        mlbg = smacs->mlfg;
        mlfg = smacs->mlbg;
    }

    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);

    mode_line = (SDL_Rect) {pane.x, win_h - (char_h * 2), win_w, char_h};
    mode_line_padding = char_w + pane.x;

    SDL_SetRenderDrawColor(smacs->renderer, mlbg.r, mlbg.g, mlbg.b, mlbg.a);
    SDL_RenderFillRect(smacs->renderer, &mode_line);

    SDL_SetRenderDrawColor(smacs->renderer, mlfg.r, mlfg.g, mlfg.b, mlfg.a);
    SDL_RenderDrawLine(smacs->renderer, pane.x, mode_line.y, mode_line.w, mode_line.y);
    SDL_RenderDrawLine(smacs->renderer, pane.x, win_h - char_h-1, mode_line.w, win_h - char_h);


    if(pane.buffer->need_to_save) {
        sb_append(mode_line_info, '*');
    }
    sb_append_many(mode_line_info, pane.buffer->file_path);

    if (is_active_pane) {
        if (smacs->editor.state & SEARCH) {
            sb_append(mode_line_info, ' ');
            if (smacs->editor.state & BACKWARD_SEARCH) {
                sb_append_many(mode_line_info, "Re-");
            }

            sb_append_many(mode_line_info, "Search[:enter next :C-g stop]");
        } else if (smacs->editor.state & EXTEND_COMMAND) {
            sb_append_many(mode_line_info, " C-x");
        }
    }

    render_draw_text(smacs, mode_line_padding, win_h - (char_h * 2), mode_line_info->data, mode_line_info->len, mlfg);
    sb_free(mode_line_info);
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

void render_draw_smacs(Smacs *smacs)
{
    Arena arena;
    Line *lines;
    Line *line;
    size_t arena_end, cursor, content_line_index, region_beg, region_end, max_line_num, current_line, line_number_len;
    register size_t pi, li, ci;
    int win_w, x, y, char_w, char_h, content_hight, char_len, common_indention, text_indention;
    register int i;
    StringBuilder *sb;
    char *data, line_number[100];
    SDL_Rect cursor_rect, region_rect;
    bool is_line_region, is_active_pane, skip_line, show_line_number;
    Pane *pane;

    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);

    cursor_rect = (SDL_Rect) {0, 0, char_w, char_h};
    region_rect = (SDL_Rect) {0, 0, char_w, char_h + smacs->leading};
    sb = (&(StringBuilder) {0});

    show_line_number = true;
    if (smacs->line_number_format == HIDE) show_line_number = false;

    for (pi = 0; pi < smacs->editor.panes_len; ++pi) {
        pane = &smacs->editor.panes[pi];

        is_active_pane = pane == smacs->editor.pane;
        lines = pane->buffer->data;
        arena = pane->arena;
        data = pane->buffer->content.data;
        cursor = pane->position;
        region_beg = MIN(smacs->editor.mark, cursor);
        region_end = MAX(smacs->editor.mark, cursor);
        assert(region_beg <= region_end);

        common_indention = pane->x;
        text_indention = common_indention + char_w;

        current_line = editor_get_current_line_number(pane) + 1;
        if (show_line_number) {
            max_line_num = arena.start + arena.show_lines;
            line_number_len = count_digits(max_line_num);
            render_format_line_number_padding(&(line_number[0]), line_number_len, max_line_num);
            line_number[line_number_len] = '\0';
            text_indention += (char_w * line_number_len);
        }

        cursor_rect.x = text_indention;
        region_rect.x = text_indention;

        x = 0;
        y = char_h;

        arena_end = MIN(arena.start + arena.show_lines, pane->buffer->len);

        if(pane->buffer->content.len > 0) {
            assert(arena.start < arena_end);

            win_w = pane->w - (char_w * 2);
            content_line_index = arena.start;
            content_hight = 0;

            for (li = arena.start; li < arena_end; ++li) {
                line = &lines[content_line_index];

                skip_line = false;
                if (content_line_index == (pane->buffer->len-1) && line->start == line->end) skip_line = true;
                if (content_line_index == (current_line - 1)) skip_line = false;

                if (skip_line) continue;
                ++content_line_index;

                if (show_line_number) {
                    render_format_display_line_number(smacs, line_number, line_number_len, content_line_index, current_line);
                    render_draw_text(smacs, common_indention, content_hight, line_number, line_number_len, current_line == content_line_index ? smacs->fg : smacs->ln);
                }

                is_line_region = is_active_pane &&
                    smacs->editor.state & SELECTION &&
                    ((line->start <= region_beg && region_beg <= line->end) ||
                     (line->start <  region_end && region_end <= line->end) ||
                     (region_beg <  line->start && region_end >  line->end));

                if (is_line_region && region_beg < line->start) {
                    region_rect.x = text_indention;
                }

                for (ci = line->start; ci <= line->end; ++ci) {
                    TTF_SizeUTF8(smacs->font, sb->data, &x, &y);

                    if (is_line_region) {
                        if (region_beg == ci) {
                            region_rect.x = text_indention + x;
                        }

                        if (region_end == ci) {
                            region_rect.w = text_indention + x - region_rect.x;
                        }
                    }

                    if (cursor == ci) {
                        cursor_rect.x = text_indention + x;
                        cursor_rect.y = content_hight;
                    }

                    if (ci < line->end) {
                        switch (data[ci]) {
                        case '\t':
                            if (is_line_region && region_beg <= ci && region_end > ci) {
                                sb_append_many(sb, "»");
                                for (i = 1; i < smacs->tab_size; ++i) sb_append(sb, ' ');
                                break;
                            }

                            for (i = 0; i < smacs->tab_size; ++i) sb_append(sb, ' ');
                            break;
                        case ' ':
                            if (is_line_region && region_beg <= ci && region_end > ci) {
                                sb_append_many(sb, "·");
                            } else {
                                sb_append(sb, data[ci]);
                            }
                            break;
                        default:
                            sb_append(sb, data[ci]);

                            for (char_len = utf8_size_char(data[ci]); char_len > 1; --char_len) {
                                sb_append(sb, data[++ci]);
                            }
                            break;
                        }

                        sb_append(sb, 0);
                        --sb->len;

                        TTF_SizeUTF8(smacs->font, sb->data, &x, &y);
                        if (win_w < (text_indention + x)) {
                            if (is_line_region) {
                                region_rect.y = content_hight;

                                if (region_end == line->end || region_end > ci) {
                                    region_rect.w = pane->w - region_rect.x;
                                }

                                SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
                                SDL_RenderFillRect(smacs->renderer, &region_rect);
                                region_rect.w = 0;
                                region_rect.x = text_indention;
                            }

                            render_draw_text(smacs, text_indention, content_hight, sb->data, sb->len, smacs->fg);

                            sb_clean(sb);
                            content_hight += (y + smacs->leading);
                        }
                    }
                }

                if (is_line_region) {
                    region_rect.y = content_hight;

                    if (region_end > line->end) {
                        region_rect.w = pane->w - region_rect.x;
                    }

                    if (region_end == line->end) {
                        region_rect.w = text_indention + x - region_rect.x;
                    }

                    SDL_SetRenderDrawColor(smacs->renderer, smacs->rg.r, smacs->rg.g, smacs->rg.b, smacs->rg.a);
                    SDL_RenderFillRect(smacs->renderer, &region_rect);
                }

                render_draw_text(smacs, text_indention, content_hight, sb->data, sb->len, smacs->fg);
                sb_clean(sb);
                content_hight += (y + smacs->leading);
            }

            if (is_active_pane) render_draw_cursor(smacs, *pane, cursor_rect, sb);

        } else {
            SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
            SDL_RenderFillRect(smacs->renderer, &cursor_rect);
        }

        render_draw_modeline(smacs, *pane, is_active_pane);

        SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
        SDL_RenderDrawLine(smacs->renderer, pane->w, 0, pane->w, pane->h);
        sb_clean(sb);
    }

    sb_free(sb);
    render_draw_mini_buffer(smacs);
}

void render_destroy_smacs(Smacs *smacs)
{
    editor_destroy(&smacs->editor);
    SDL_DestroyRenderer(smacs->renderer);
    SDL_DestroyWindow(smacs->window);
    free(smacs->notification);
    TTF_CloseFont(smacs->font);
}
