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
    size_t cursor, char_len, i, content_len;

    data = pane.buffer->content.data;
    content_len = pane.buffer->content.len;
    cursor = pane.position;

    assert(content_len >= cursor);
    SDL_SetRenderDrawColor(smacs->renderer, smacs->cfg.r, smacs->cfg.g, smacs->cfg.b, smacs->cfg.a);
    SDL_RenderFillRect(smacs->renderer, &cursor_rect);

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

void render_draw_modeline(Smacs *smacs, Pane pane, bool is_active_pane)
{
    SDL_Rect mode_line, user_input_rect, mini_buffer_cursor;
    int win_w, win_h, char_w, char_h, mode_line_padding, mini_buffer_padding;
    StringBuilder *mini_buffer_content;
	size_t i;

    char mode_line_info[1000] = {0};
    SDL_Color mlbg, mlfg;

    mlbg = smacs->mlbg;
    mlfg = smacs->mlfg;
	mini_buffer_content = &(StringBuilder) {0};
    if (!is_active_pane) {
        mlbg = smacs->mlfg;
        mlfg = smacs->mlbg;
    }

    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);
    SDL_GetWindowSize(smacs->window, &win_w, &win_h);

    mode_line = (SDL_Rect) {pane.x, win_h - (char_h * 2), win_w, char_h};
    user_input_rect = (SDL_Rect) {pane.x, win_h - (char_h * 1), win_w, char_h};
    mini_buffer_cursor = (SDL_Rect) {pane.x, win_h - char_h, 2, char_h};

    mode_line_padding = char_w + pane.x;
    mini_buffer_padding = char_w;

    SDL_SetRenderDrawColor(smacs->renderer, mlbg.r, mlbg.g, mlbg.b, mlbg.a);
    SDL_RenderFillRect(smacs->renderer, &mode_line);

    SDL_SetRenderDrawColor(smacs->renderer, smacs->bg.r, smacs->bg.g, smacs->bg.b, smacs->bg.a);
    SDL_RenderFillRect(smacs->renderer, &user_input_rect);

    SDL_SetRenderDrawColor(smacs->renderer, mlfg.r, mlfg.g, mlfg.b, mlfg.a);
    SDL_RenderDrawLine(smacs->renderer, pane.x, mode_line.y, mode_line.w, mode_line.y);
    SDL_RenderDrawLine(smacs->renderer, pane.x, win_h - char_h, mode_line.w, win_h - char_h);

    if (is_active_pane) {

        if (smacs->editor.state & (SEARCH | EXTEND_COMMAND | COMPLETION)) {
			if (smacs->editor.state & COMPLETION) {
				if (smacs->editor.user_input.len > 0) {
					sb_append_many(mini_buffer_content, smacs->editor.user_input.data);
				}

				sb_append_many(mini_buffer_content, " {");
				for (i = 0; i < smacs->editor.completor.len; ++i) {
					sb_append_many(mini_buffer_content, smacs->editor.completor.data[i]);
					if (i < (smacs->editor.completor.len-1)) {
						sb_append_many(mini_buffer_content, " | ");
					}
				}
				sb_append(mini_buffer_content, '}');

	            render_draw_text(smacs, mini_buffer_padding, win_h - char_h, mini_buffer_content->data, mini_buffer_content->len, smacs->fg);
	            TTF_SizeUTF8(smacs->font, smacs->editor.user_input.len == 0 ? "" : smacs->editor.user_input.data, &mini_buffer_cursor.x, NULL);
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
    }

    sprintf(mode_line_info,
            "%s%s %s%s",
            pane.buffer->need_to_save ? "*" : "",
            pane.buffer->file_path,
            !is_active_pane ? "" : smacs->editor.state & BACKWARD_SEARCH ? "Re-" : "",
            !is_active_pane ? "" : smacs->editor.state & SEARCH ? "Search[:enter next :C-g stop]" : smacs->editor.state & EXTEND_COMMAND ? "C-x" : "");

    render_draw_text(smacs, mode_line_padding, win_h - (char_h * 2), mode_line_info, strlen(mode_line_info), mlfg);
	sb_free(mini_buffer_content);
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
        count++;
    }

    return count;
}

void render_format_line_number_padding(char **buffer, size_t buffer_len, size_t num)
{
    int to_fill;

    memset(*buffer, ' ', buffer_len);
    to_fill = ((int) buffer_len) - count_digits(num);
    sprintf(&(*buffer)[to_fill], "%ld", num);
}

void render_format_display_line_number(Smacs *smacs, char **buffer, size_t buffer_len, size_t num, size_t cursor_line_num)
{
    switch (smacs->line_number_format) {
    case ABSOLUTE: {
        render_format_line_number_padding(buffer, buffer_len, num);
        break;
    }
    case RELATIVE: {
        if (cursor_line_num < num) {
            render_format_line_number_padding(buffer, buffer_len, num - cursor_line_num);
        } else if (cursor_line_num > num) {
            render_format_line_number_padding(buffer, buffer_len, cursor_line_num - num);
        } else {
            render_format_line_number_padding(buffer, buffer_len, cursor_line_num);
        }
        break;
    }
    }
}

void render_draw_smacs(Smacs *smacs)
{
    Arena arena;
    Line *lines;
    Line line;
    size_t arena_end, li, ci, cursor, content_line_index, region_beg, region_end, max_line_num, current_line, line_number_len, pi;
    int win_w, x, y, char_w, char_h, content_hight, char_len, common_indention, text_indention, i;
    StringBuilder *sb;
    char *data, *line_number;
    SDL_Rect cursor_rect, region_rect;
    bool is_line_region, is_active_pane, skip_line;
    Pane *pane;
    TTF_SizeUTF8(smacs->font, "|", &char_w, &char_h);

    for (pi = 0; pi < smacs->editor.panes_len; ++pi) {
        pane = &smacs->editor.panes[pi];

        is_active_pane = pane == smacs->editor.pane;

        lines = pane->buffer->lines;
        arena = pane->arena;
        data = pane->buffer->content.data;
        cursor = pane->position;
        region_beg = smacs->editor.mark < cursor ? smacs->editor.mark : cursor;
        region_end = smacs->editor.mark > cursor ? smacs->editor.mark : cursor;
        assert(region_beg <= region_end);

        current_line = editor_get_current_line_number(pane) + 1;
        max_line_num = arena.start + arena.show_lines;
        line_number_len = count_digits(max_line_num);
        line_number = (char*) calloc(line_number_len + 1, sizeof(char));
        render_format_line_number_padding(&line_number, line_number_len, max_line_num);

        common_indention = pane->x + char_w * 2;
        text_indention = common_indention + char_w * strlen(line_number) + (char_w * 1);
        cursor_rect = (SDL_Rect) {text_indention, 0, char_w, char_h};
        region_rect = (SDL_Rect) {text_indention, 0, char_w, char_h + smacs->leading};

        x = 0;
        y = char_h;

        arena_end = MIN(arena.start + arena.show_lines, pane->buffer->lines_count);

        if(pane->buffer->content.len > 0) {
            assert(arena.start < arena_end);

            sb = &(StringBuilder) {0};

            win_w = pane->w;

            win_w -= (char_w * 2); //PADDING

            content_line_index = arena.start;
            content_hight = 0;

            for (li = 0; (li + arena.start) < arena_end; ++li) {
                line = lines[content_line_index];

                skip_line = false;
                if (content_line_index == (pane->buffer->lines_count-1) && line.start == line.end) skip_line = true;
                if (content_line_index == (current_line - 1)) skip_line = false;

                if (skip_line) break;
                ++content_line_index;

                render_format_display_line_number(smacs, &line_number, line_number_len, content_line_index, current_line);
                if (current_line == content_line_index) {
                    render_draw_text(smacs, common_indention, content_hight, line_number, line_number_len, smacs->fg);
                } else {
                    render_draw_text(smacs, common_indention, content_hight, line_number, line_number_len, smacs->ln);
                }

                is_line_region = is_active_pane &&
                    smacs->editor.state & SELECTION &&
                    ((line.start <= region_beg && region_beg <= line.end) ||
                     (line.start <  region_end && region_end <= line.end) ||
                     (region_beg <  line.start && region_end >  line.end));

                if (is_line_region && region_beg < line.start) {
                    region_rect.x = text_indention;
                }

                for (ci = line.start; ci <= line.end; ++ci) {
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

                    if (ci < line.end) {
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

                                if (region_end == line.end || region_end > ci) {
                                    TTF_SizeUTF8(smacs->font, sb->data, &x, NULL);
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

                    if (region_end > line.end) {
                        region_rect.w = pane->w - region_rect.x;
                    }

                    if (region_end == line.end) {
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

            sb_free(sb);
        } else {
            SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
            SDL_RenderFillRect(smacs->renderer, &cursor_rect);
        }

        render_draw_modeline(smacs, *pane, is_active_pane);

        SDL_SetRenderDrawColor(smacs->renderer, smacs->fg.r, smacs->fg.g, smacs->fg.b, smacs->fg.a);
        SDL_RenderDrawLine(smacs->renderer, pane->w, 0, pane->w, pane->h);
    }

    free(line_number);
}

void render_destroy_smacs(Smacs *smacs)
{
    editor_destroy(&smacs->editor);
    SDL_DestroyRenderer(smacs->renderer);
    SDL_DestroyWindow(smacs->window);
    free(smacs->notification);
    TTF_CloseFont(smacs->font);
}
