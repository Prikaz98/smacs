#include <assert.h>
#include <sys/param.h>
#include <stdlib.h>

#include "common.h"
#include "tokenize.h"
#include "render.h"
#include "utf8.h"

#define LINE_BUFFER_LEN 100

void render_draw_text(Smacs *smacs, int x, int y, char *text, size_t text_len, SDL_Color foreground_color)
{
	SDL_Texture *texture = NULL;
	SDL_FRect rect = (SDL_FRect) {0.0, 0.0, 0.0, 0.0};
	SDL_Surface *surface = NULL;

	if (text == NULL) return;
	if (text_len == 0) return;

	surface = TTF_RenderText_Blended(smacs->font, text, text_len, foreground_color);
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
	if (starts_withl(path, home_dir, home_dir_len)) {
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
	snprintf(buffer, LINE_BUFFER_LEN, "%*ld", (int)buffer_len, num);
}

void render_format_display_line_number(Smacs *smacs, char *buffer, size_t buffer_len, size_t num, size_t cursor_line_num)
{
	switch (smacs->line_number_format) {
	case HIDE:
		break;
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

void render_glyph_clean(GlyphList *glyph)
{
	gb_clean(&glyph->string_data);
	gb_clean(glyph);
}

void render_destroy_glyph(GlyphList *glyph)
{
	render_glyph_clean(glyph);
	gb_free(&glyph->string_data);
	gb_free(glyph);
}

void render_append_char_to_rendering(Smacs *smacs, StringBuilder *sb, char *data, size_t *i)
{
	size_t ci;
	int char_len;

	ci = *i;

	switch (data[ci]) {
	case '\t':
		for (int tabs = 0; tabs < smacs->tab_size; ++tabs) sb_append(sb, ' ');
		break;
	case '\n':
		break;
	default: {
		char_len = utf8_size_char(data[ci]);
		for (int i = 0; i < char_len; ++i) sb_append(sb, data[ci+i]);
		ci += (char_len-1);
		break;
	}
	}

	*i = ci;
}

GlyphItem *render_flush_item_sb_and_move_x(Smacs *smacs, GlyphList *glyph, StringBuilder *sb, int *x_, int y, GlyphItemEnum kind, long position)
{
	int x = *x_;
	size_t string_pointer = glyph->string_data.len;
	GlyphItem *new_item = NULL;

	if (sb->len > 0) {
		int w, h;

		TTF_GetStringSize(smacs->font, sb->data, sb->len, &w, &h);

		sb_append_manyl(&glyph->string_data, sb->data, sb->len);
		gb_append(glyph, ((GlyphItem) {
					.beg = string_pointer,
					.len = sb->len,
					.x = x,
					.y = y,
					.w = w,
					.h = h,
					.kind = kind,
					position = position}));

		sb_clean(sb);
		x += w;
		new_item = &glyph->data[glyph->len-1];
	} else if (kind & CURSOR) {
		gb_append(glyph, ((GlyphItem) {
					.beg = string_pointer,
					.len = 0,
					.x = x,
					.y = y,
					.w = smacs->char_w,
					.h = smacs->char_h,
					.kind = kind,
					.position = position}));
		x += smacs->char_w;
		new_item = &glyph->data[glyph->len-1];
	}

	*x_ = x;
	return new_item;
}

typedef struct {
	char *data;
	size_t data_len;
	int x;

	bool selection, is_active_pane, c_like_file;

	size_t region_beg, region_end;
	size_t cursor;
	size_t arena_start_point;

	int content_hight, text_indention, pane_width_threashold;
} PaneDrawingInfo;

void render_line_processing(Smacs *smacs, PaneDrawingInfo *info, Line *line, StringBuilder *sb, GlyphList *glyph)
{
	size_t data_index;
	GlyphItemEnum kind = TEXT;

	for (data_index = line->start; data_index <= line->end; ++data_index) {
		if (info->selection && (data_index >= info->region_beg)) {
			kind = kind | REGION;
		}

		if (kind & REGION && (data_index >= info->region_end)) {
			kind = kind ^ REGION;
		}

		if (info->is_active_pane && data_index == info->cursor) {
			kind = kind | CURSOR;

			if (info->cursor == info->data_len) {
				gb_append(glyph, ((GlyphItem) {0, 0, info->x, info->content_hight, smacs->char_w, smacs->char_h, kind, data_index}));
				kind = kind ^ CURSOR;
			}
		} else if (kind & CURSOR) {
			kind = kind ^ CURSOR;
		}

		if (info->c_like_file) {
			switch (smacs->tokenize.data[data_index - info->arena_start_point]) {
			case TOKEN_STRING:
				kind = kind | STRING;
				break;
			case TOKEN_COMMENT:
				kind = kind | COMMENT;
				break;
			case TOKEN_NUMBER:
				kind = kind | NUMBER;
				break;
			case TOKEN_BOOLEAN:
				kind = kind | NUMBER;
				break;
			default:
				break;
			}
		}


		if (info->x >= info->pane_width_threashold) {
			info->content_hight += (smacs->char_h + smacs->leading);
			info->x = info->text_indention;
		}

		if (data_index < info->data_len) {
			render_append_char_to_rendering(smacs, sb, info->data, &data_index);
			render_flush_item_sb_and_move_x(smacs, glyph, sb, &info->x, info->content_hight, kind, data_index);
		}

		if (info->c_like_file) {
			switch (smacs->tokenize.data[data_index - info->arena_start_point]) {
			case TOKEN_STRING:
				kind = kind ^ STRING;
				break;
			case TOKEN_COMMENT:
				kind = kind ^ COMMENT;
				break;
			case TOKEN_NUMBER:
				kind = kind ^ NUMBER;
				break;
			case TOKEN_BOOLEAN:
				kind = kind ^ NUMBER;
				break;
			default:
				break;
			}
		}
	}
}

static StringBuilder RenderStringBuilder = {0};

void render_update_glyph(Smacs *smacs)
{
	Arena arena;
	Line *lines;
	Line *line;
	size_t arena_end, cursor, region_beg, region_end, max_line_num, current_line, line_number_len, data_len, pane_index;
	int win_w, win_h, content_limit, common_indention, text_indention, pane_width_threashold;
	StringBuilder *sb;
	char *data, line_number[LINE_BUFFER_LEN];
	bool is_active_pane, show_line_number, mini_buffer_is_active;
	Pane *pane;
	GlyphList *glyph;

	glyph = &smacs->glyph;
	render_glyph_clean(glyph);

	SDL_GetWindowSize(smacs->window, &win_w, &win_h);
	content_limit = win_h - (smacs->char_h * 2.5);

	sb = &RenderStringBuilder;
	sb_clean(sb);

	show_line_number = true;
	if (smacs->line_number_format == HIDE) show_line_number = false;

	mini_buffer_is_active = editor_is_mini_buffer_active(&smacs->editor);

	for (pane_index = 0; pane_index < smacs->editor.panes_len; ++pane_index) {
		pane = &smacs->editor.panes[pane_index];

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
		pane_width_threashold = pane->x + pane->w - (smacs->char_w * 2);

		current_line = editor_get_current_line_number(pane) + 1;
		if (show_line_number) {
			max_line_num = arena.start + arena.show_lines;
			line_number_len = count_digits(max_line_num);
			render_format_line_number_padding(line_number, line_number_len, max_line_num);

			text_indention += (smacs->char_w * line_number_len);
		}

		arena_end = MIN(arena.start + arena.show_lines, pane->buffer->len);

		//MAIN BUFFERS RENDERING
		{
			PaneDrawingInfo *info = &((PaneDrawingInfo) {0});
			info->x = 0;
			info->content_hight = 0;

			if(data_len == 0) {
				gb_append(glyph, ((GlyphItem) {0, 0, info->x, info->content_hight, smacs->char_w, smacs->char_h, TEXT | CURSOR, 0}));
			} else {

				assert(arena.start < arena_end);
				size_t line_index, string_pointer;
				int w, h;

				info->selection = is_active_pane && smacs->editor.state & SELECTION && region_beg != region_end;
				info->arena_start_point = lines[arena.start].start;
				info->data = data;
				info->data_len = data_len;
				info->is_active_pane = is_active_pane;
				info->region_beg = region_beg;
				info->region_end = region_end;
				info->cursor = cursor;
				info->text_indention = text_indention;
				info->pane_width_threashold = pane_width_threashold;
				if (pane->buffer->file_path_len > 2) {
					if (strncmp(&pane->buffer->file_path[pane->buffer->file_path_len-2], ".c", 2) == 0 ||
						strncmp(&pane->buffer->file_path[pane->buffer->file_path_len-2], ".h", 2) == 0) {
						info->c_like_file = true;
						tokenize(&smacs->tokenize, &data[info->arena_start_point], lines[arena_end-1].end + 1);
					}
				}

				for (line_index = arena.start; line_index < arena_end; ++line_index) {
					line = &lines[line_index];
					info->x = text_indention;
					if (content_limit <= info->content_hight) break;

					if (show_line_number) {
						render_format_display_line_number(smacs, line_number, line_number_len, line_index + 1, current_line);
						TTF_GetStringSize(smacs->font, line_number, line_number_len, &w, &h);
						string_pointer = glyph->string_data.len;
						sb_append_manyl(&glyph->string_data, line_number, line_number_len);

						gb_append(glyph,
								  ((GlyphItem) {
									  string_pointer,
									  line_number_len,
									  common_indention,
									  info->content_hight,
									  w,
									  h,
									  LINE_NUMBER,
									  -1}));

					}

					render_line_processing(smacs, info, line, sb, glyph);
					info->content_hight += (smacs->char_h + smacs->leading);
				}
			}

			gb_append(glyph, ((GlyphItem) {0, 0, pane->x+pane->w, 0, pane->x+pane->w, pane->h-smacs->char_h * (mini_buffer_is_active ? 2 : 1), LINE, -1}));
		}

		//MODE LINE
		{
			int padding;
			GlyphItem *item;
			int mode_line_h;

			padding = common_indention;

			if(pane->buffer->need_to_save) {
				sb_append(sb, '*');
			}

			render_append_file_path(sb, pane->buffer->file_path, smacs->home_dir, strlen(smacs->home_dir));
			sb_append_many(sb, " (");
			snprintf(line_number, LINE_BUFFER_LEN, "%ld", current_line);
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

			mode_line_h = win_h - (mini_buffer_is_active ? smacs->char_h*2 : smacs->char_h);

			item = render_flush_item_sb_and_move_x(smacs, glyph, sb, &padding, mode_line_h, is_active_pane ? MODE_LINE_ACTIVE : MODE_LINE, -1);
			if (item != NULL) {
				item->w = pane->w;
			}
		}
	}

	//MINI BUFFER

	if (mini_buffer_is_active)
	{
		size_t home_dir_len, completion_delimiter_len;
		int completion_w, padding, complition_width_limit;
		char *completion_delimiter, *parens;

		completion_delimiter = " | ";
		completion_delimiter_len = strlen(completion_delimiter);

		home_dir_len = strlen(smacs->home_dir);
		padding = smacs->char_w;
		complition_width_limit = win_w - smacs->char_w*10;

		if (smacs->editor.state & (SEARCH | EXTEND_COMMAND | COMPLETION)) {
			if (smacs->editor.state & COMPLETION) {
				if ((smacs->editor.state & _FILE) && (strncmp(smacs->editor.dir, ".", 1) != 0)) {
					sb_append_many(sb, "Find file: ");
					render_append_file_path(sb, smacs->editor.dir, smacs->home_dir, home_dir_len);
					sb_append(sb, '/');
				}

				if (smacs->editor.user_input.len > 0) {
					sb_append_manyl(sb, smacs->editor.user_input.data, smacs->editor.user_input.len);
				}

				parens = "{}";
				if (smacs->editor.completor.filtered.len == 1) {
					parens = "[]";
				}

				sb_append(sb, parens[0]);
				for (size_t i = 0; i < smacs->editor.completor.filtered.len; ++i) {
					render_append_file_path(sb, smacs->editor.completor.filtered.data[i], smacs->home_dir, home_dir_len);

					if (i < (smacs->editor.completor.filtered.len-1)) {
						sb_append_manyl(sb, completion_delimiter, completion_delimiter_len);
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
				render_flush_item_sb_and_move_x(smacs, glyph, sb, &padding, win_h - smacs->char_h, MINI_BUFFER, -1);
			} else {
				if (smacs->editor.user_input.len > 0) {
					sb_append_manyl(sb, smacs->editor.user_input.data, smacs->editor.user_input.len);
					render_flush_item_sb_and_move_x(smacs, glyph, sb, &padding, win_h - smacs->char_h, MINI_BUFFER, -1);
				}
			}
		} else if (strlen(smacs->notification) > 0) {
			sb_append_many(sb, smacs->notification);
			render_flush_item_sb_and_move_x(smacs, glyph, sb, &padding, win_h - smacs->char_h, MINI_BUFFER, -1);
		}
	}
}

void render_draw_batch(Smacs *smacs, GlyphItemEnum kind, char *string, size_t string_len, SDL_FRect *rect)
{
	SDL_Color foreground_color;

	if (kind & TEXT) {
		if (kind & REGION) {
			SDL_SetRenderDrawColor(smacs->renderer, smacs->region_color.r, smacs->region_color.g, smacs->region_color.b, smacs->region_color.a);
			//small hack to fill space between the lines
			SDL_FRect selection_rectangle = {.x = rect->x, .y = rect->y, .w = rect->w, .h = (rect->h * 1.2)};
			SDL_RenderFillRect(smacs->renderer, &selection_rectangle);
			foreground_color = smacs->foreground_color;
		} else if (kind & COMMENT) {
			foreground_color = smacs->comment_foreground_color;
		} else if (kind & NUMBER) {
			foreground_color = smacs->number_foreground_color;
		} else if (kind & STRING) {
			foreground_color = smacs->string_foreground_color;
		} else {
			foreground_color = smacs->foreground_color;
		}

		if (kind & CURSOR) {
			SDL_SetRenderDrawColor(smacs->renderer, smacs->cursor_foreground_color.r, smacs->cursor_foreground_color.g, smacs->cursor_foreground_color.b, smacs->cursor_foreground_color.a);
			SDL_RenderFillRect(smacs->renderer, rect);
			foreground_color = smacs->background_color;
		}

		render_draw_text(smacs, rect->x, rect->y, string, string_len, foreground_color);
	} else if (kind & MODE_LINE) {
		SDL_SetRenderDrawColor(smacs->renderer, smacs->background_color.r, smacs->background_color.g, smacs->background_color.b, smacs->background_color.a);
		SDL_RenderFillRect(smacs->renderer, rect);

		render_draw_text(smacs, rect->x, rect->y, string, string_len, smacs->foreground_color);
	} else if (kind & MODE_LINE_ACTIVE) {
		SDL_SetRenderDrawColor(smacs->renderer, smacs->mode_line_background_color.r, smacs->mode_line_background_color.g, smacs->mode_line_background_color.b, smacs->mode_line_background_color.a);
		SDL_RenderFillRect(smacs->renderer, rect);

		render_draw_text(smacs, rect->x, rect->y, string, string_len, smacs->mode_line_foreground_color);
	} else if (kind & MINI_BUFFER) {
		render_draw_text(smacs, rect->x, rect->y, string, string_len, smacs->foreground_color);
	} else if (kind & LINE_NUMBER) {
		render_draw_text(smacs, rect->x, rect->y, string, string_len, smacs->line_number_color);
	} else if (kind & LINE) {
		SDL_SetRenderDrawColor(smacs->renderer, smacs->foreground_color.r, smacs->foreground_color.g, smacs->foreground_color.b, smacs->foreground_color.a);
		SDL_RenderLine(smacs->renderer, rect->x, rect->y, rect->w, rect->h);
	}
}

void render_glyph_show(Smacs *smacs)
{
	GlyphItem *item, *prev_item;
	SDL_FRect rect;
	char *string_beginning;
	size_t string_len_to_show;

	rect = (SDL_FRect) {0.0, 0.0, 0.0, 0.0};
	string_beginning = NULL;
	string_len_to_show = 0;


	for (size_t i = 0; i < smacs->glyph.len; ++i) {
		item = &smacs->glyph.data[i];

		if (i == 0) {
			string_beginning = &smacs->glyph.string_data.data[item->beg];
			string_len_to_show = item->len;
		}

		if (i > 0) {
			prev_item = &smacs->glyph.data[i-1];

			if (prev_item->y == item->y && prev_item->kind == item->kind) {
				if (item->len > 0) string_len_to_show += item->len;

				rect.w += item->w;
				continue;
			}

			render_draw_batch(smacs, prev_item->kind, string_beginning, string_len_to_show, &rect);
		}

		rect.x = item->x;
		rect.y = item->y;
		rect.h = item->h;
		rect.w = item->w;

		string_beginning = &smacs->glyph.string_data.data[item->beg];
		string_len_to_show = item->len;
		if (i == (smacs->glyph.len-1)) {
			if (string_len_to_show > 0 || item->kind & CURSOR) {
				render_draw_batch(smacs, item->kind, string_beginning, string_len_to_show, &rect);
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
	TTF_CloseFont(smacs->fallback_font);
	sb_free(&RenderStringBuilder);
}

long render_find_position_by_xy(Smacs *smacs, int x, int y)
{
	GlyphItem item;
	long point = -1;

	/*
	fixme(ivan):
		Now it works fine but if you click to empty space it should found a line (x) first
		and if in the current line no fitted y char put it in the end of the line
	*/

	for (size_t i = 0; i < smacs->glyph.len; ++i) {
		item = smacs->glyph.data[i];
		if (item.y >= y && item.x >= x) {
			point = item.position;
			break;
		}
	}

	return point;
}
