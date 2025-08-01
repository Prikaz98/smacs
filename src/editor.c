#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>
#include <assert.h>

#include "editor.h"
#include "utf8.h"
#include "common.h"

void append_char(Content *content, char ch, size_t pos)
{
    if (content == NULL) {
        fprintf(stderr, "Could not append char content is NULL\n");
        return;
    }

    if (content->len >= content->capacity) {
        content->capacity = content->capacity == 0 ? EDITOR_CONTENT_CAP : content->capacity * 2;
        content->data = realloc(content->data, content->capacity * sizeof(*content->data));
        memset(&content->data[content->len], 0, content->capacity - content->len);
    }

    if (pos == content->len) {
        content->data[content->len++] = ch;
    } else if (pos < content->len) {
        memmove(&content->data[pos + 1], &content->data[pos], content->len - pos);
        content->data[pos] = ch;
        content->len++;
    } else {
        fprintf(stderr, "Out of range insert char char_pos: %ld content_len: %ld\n", pos, content->len);
    }
}

void editor_goto_point(Editor *editor, size_t pos)
{
    size_t max_len;

    max_len = editor->pane->buffer->content.len;
    if (pos > max_len) return;

    editor->pane->position = pos;
}

void editor_delete_backward(Editor *editor)
{
    Buffer *buf;
    Content *content;
    size_t reg_beg, reg_end;
    uint8_t delete_len;

    buf = editor->pane->buffer;
    content = &buf->content;

    if (editor->state == SELECTION) {
        reg_beg = MIN(editor->mark, editor->pane->position);
        reg_end = MAX(editor->mark, editor->pane->position);
        delete_len = reg_end - reg_beg;

        memmove(&content->data[reg_beg], &content->data[reg_end], content->len - reg_end);
        content->len -= delete_len;
        editor_goto_point(editor, reg_beg);
    } else {
        if (editor->pane->position == 0) return;

        delete_len = utf8_size_char_backward(content->data, editor->pane->position - 1);
        memmove(&content->data[editor->pane->position - delete_len], &content->data[editor->pane->position], content->len - editor->pane->position + delete_len);
        content->len -= delete_len;
        editor_goto_point(editor, editor->pane->position - delete_len);
    }

    memset(&content->data[content->len], 0, content->capacity - content->len);

    editor_determine_lines(editor);
    editor->state = NONE;
    editor->pane->buffer->need_to_save = true;
}

void editor_delete_forward(Editor *editor)
{
    int8_t char_len;
    Buffer *buf;
    Content *content;

    if (editor->state == SELECTION) {
        editor_delete_backward(editor);
        return;
    }

    buf = editor->pane->buffer;
    content = &buf->content;
    if (editor->pane->position >= content->len) return;

    char_len = utf8_size_char(content->data[editor->pane->position]);
    memmove(&content->data[editor->pane->position], &content->data[editor->pane->position + char_len], content->len - editor->pane->position);
    content->len -= char_len;

    editor_determine_lines(editor);
    editor->state = NONE;
    editor->pane->buffer->need_to_save = true;
}

void editor_insert(Editor *editor, char *str)
{
    Buffer *buf = editor->pane->buffer;
    size_t i;

    if (str == NULL) return;

    Content *content = &buf->content;

    if (content == NULL) {
        fprintf(stderr, "Content is null\n");
        return;
    }

    if (editor->state == SELECTION) {
        editor_delete_backward(editor);
    }

    size_t str_size = strlen(str);
    for (i = 0; i < str_size; ++i) {
        append_char(content, str[i], editor->pane->position + i);
    }

    editor->pane->position += str_size;
    editor->pane->buffer->need_to_save = true;
    editor_determine_lines(editor);
    editor->state = NONE;
}

size_t editor_get_current_line_number(Pane *pane)
{
    size_t i;
    Line line;

    if (pane->buffer->lines_count == 0) return 0;

    for (i = 0; i < pane->buffer->lines_count; ++i) {
        line = pane->buffer->lines[i];
        if (line.start <= pane->position && pane->position <= line.end) {
            return i;
        }
    }

    return 0;
}

void editor_recognize_arena(Editor *editor)
{
    size_t line_num, pane_i;
    Arena *arena;
    Pane *pane;

    for (pane_i = 0; pane_i < editor->panes_len; ++pane_i) {
        pane = &editor->panes[pane_i];
        line_num = editor_get_current_line_number(pane);
        arena = &pane->arena;

        if (line_num >= (arena->start + arena->show_lines - 4)) {
            arena->start = MIN(pane->buffer->lines_count - 1, line_num - arena->show_lines / 2);
        } else if (line_num < arena->start) {
            arena->start = line_num;
        }
    }
}

void editor_next_line(Editor *editor)
{
    size_t next_pos, line_num;
    Line line;

    line_num = editor_get_current_line_number(editor->pane);

    if (editor->pane->buffer->lines_count > 0) {
        line = editor->pane->buffer->lines[line_num];
        editor->pane->buffer->column = MAX(editor->pane->buffer->column, editor->pane->position - line.start);

        line_num++;
        if (editor->pane->buffer->lines_count > line_num) {
            line = editor->pane->buffer->lines[line_num];
            next_pos = line.start + editor->pane->buffer->column;

            editor_goto_point(editor, MIN(next_pos, line.end));

            editor_recognize_arena(editor);
        }
    }
}

void editor_previous_line(Editor *editor)
{
    size_t next_pos, line_num;
    Line line;

    line_num = editor_get_current_line_number(editor->pane);
    line = editor->pane->buffer->lines[line_num];
    editor->pane->buffer->column = MAX(editor->pane->buffer->column, editor->pane->position - line.start);

    if (line_num != 0) {
        line_num--;
        line = editor->pane->buffer->lines[line_num];

        next_pos = line.start + editor->pane->buffer->column;
        editor_goto_point(editor, MIN(next_pos, line.end));

        editor_recognize_arena(editor);
    }
}

void editor_char_forward(Editor *editor)
{
    uint8_t char_len;

    char_len = utf8_size_char(editor->pane->buffer->content.data[editor->pane->position]);
    editor_goto_point(editor, MIN(editor->pane->position + char_len, editor->pane->buffer->content.len));
}

void editor_char_backward(Editor *editor)
{
    uint8_t char_len;

    char_len = utf8_size_char_backward(editor->pane->buffer->content.data, editor->pane->position - 1);
    if (editor->pane->buffer->column > 0) {
        editor->pane->buffer->column--;
    }
    editor_goto_point(editor, (size_t) MAX(((int)editor->pane->position) - char_len, 0));
}

void editor_move_end_of_line(Editor *editor)
{
    size_t i;
    Line line;

    for (i = 0; i < editor->pane->buffer->lines_count; ++i) {
        line = editor->pane->buffer->lines[i];
        if (line.start <= editor->pane->position && editor->pane->position <= line.end) {
            editor_goto_point(editor, line.end);
            return;
        }
    }
}

void editor_move_begginning_of_line(Editor *editor)
{
    size_t i;
    Line line;

    for (i = 0; i < editor->pane->buffer->lines_count; ++i) {
        line = editor->pane->buffer->lines[i];
        if (line.start <= editor->pane->position && editor->pane->position <= line.end) {
            editor_goto_point(editor, line.start);
            editor->pane->buffer->column = 0;
            return;
        }
    }
}

void editor_determine_lines(Editor *editor)
{
    size_t i, beg;
    Buffer *buffer;

    buffer = editor->pane->buffer;

    buf_line_clean(buffer);
    if (buffer->content.len == 0) return;

    beg = 0;
    for (i = 0; i < buffer->content.len; ++i) {
        if (buffer->content.data[i] == '\n') {
            buf_line_append(buffer, ((Line) {beg, i}));
            beg = i + 1;
        }
    }

    buf_line_append(buffer, ((Line) {beg, i}));
}

int editor_save(Editor* editor)
{
    FILE *out;
    Content content;
    Buffer *buf;

    buf = editor->pane->buffer;

    if (buf->file_path == NULL) {
        fprintf(stderr, "No file_path\n");
        return 1;
    }

    if (strlen(buf->file_path) <= 0) {
        fprintf(stderr, "file_path is invalid\n");
        return 1;
    }

    out = fopen(buf->file_path, "w+");
    if (out == NULL) {
        fprintf(stderr, "Could not save file %s\n", buf->file_path);
        return 1;
    }

    content = buf->content;
    if (content.len > 0) {
        if (buf->content.data[buf->content.len - 1] != '\n') {
            append_char(&buf->content, '\n', buf->content.len);
            editor_determine_lines(editor);
        }
        fwrite(content.data, sizeof(char), content.len, out);
    }

    fclose(out);
    editor->pane->buffer->need_to_save = false;

    return 0;
}

Buffer* editor_create_buffer(Editor *editor, char *file_path)
{
    size_t i;

    for (i = 0; i < editor->buffer_list.len; ++i) {
        if (strcmp(editor->buffer_list.buffers[i].file_path, file_path) == 0) {
            return &editor->buffer_list.buffers[i];
        }
    }

    buffer_list_append((&editor->buffer_list), ((Buffer) {0}));
    return &editor->buffer_list.buffers[editor->buffer_list.len - 1];
}

int editor_read_file(Editor *editor, char *file_path)
{
    FILE *in;
    Content content;
    char next;
    size_t len;
    Pane *pane;

    in = fopen(file_path, "r");
    content = (Content) {0};
    len = 0;

    if (in != NULL) {
        while ((next = fgetc(in)) != EOF) {
            append_char(&content, next, len);
            ++len;
        }
        fclose(in);
    }

    pane = editor->pane;

    pane->buffer = editor_create_buffer(editor, file_path);
    pane->buffer->content = content;
    pane->buffer->file_path = (char*) calloc(strlen(file_path) + 1, sizeof(char));
    editor_goto_point(editor, 0);

    strcpy(pane->buffer->file_path, file_path);

    editor_determine_lines(editor);
    editor_recognize_arena(editor);

    return 0;
}

void editor_kill_line(Editor *editor)
{
    size_t i, del_count;
    Line line;

    for (i = 0; i < editor->pane->buffer->lines_count; ++i) {
        line = editor->pane->buffer->lines[i];
        if (line.start <= editor->pane->position && editor->pane->position <= line.end) {
            del_count = (size_t) line.end - editor->pane->position;
            if (del_count > 0) {
                editor_set_mark(editor);
                editor_move_end_of_line(editor);
                editor_cut(editor);
            } else {
                editor_delete_forward(editor);
            }
        }
    }

    editor_determine_lines(editor);
    editor->state = NONE;
}

void editor_destory_buffer(Buffer *buf)
{
    if (buf->content.len > 0) {
        free(buf->file_path);
        buf->file_path = NULL;

        free(buf->content.data);
        buf->content.data = NULL;
    }
}

void editor_destroy(Editor *editor)
{
    size_t i;

    if (editor->pane->buffer->lines_count > 0) {
        free(editor->pane->buffer->lines);
        editor->pane->buffer->lines = NULL;
    }

    for (i = 0; i < editor->buffer_list.len; ++i) {
        editor_destory_buffer(&editor->buffer_list.buffers[i]);
    }

    editor->buffer_list.len = 0;
    free(editor->buffer_list.buffers);
    editor->buffer_list.buffers = NULL;
}

void editor_recenter_top_bottom(Editor *editor)
{
    int line_num, center, half_screen;
    Arena *arena;

    line_num = (int) editor_get_current_line_number(editor->pane);
    arena = &editor->pane->arena;
    half_screen = (int) arena->show_lines / 2;
    center = MIN(editor->pane->buffer->lines_count, arena->start + half_screen);

    //top -> bottom
    if (arena->start == (size_t) line_num) {
        arena->start = (size_t) MAX(line_num - (int) arena->show_lines + 5, 0);
    //center -> top
    } else if (line_num == center) {
        arena->start = line_num;
    //any -> center
    } else {
        arena->start = (size_t) MAX(line_num - ((int) arena->show_lines / 2), 0);
    }
}

void editor_scroll_up(Editor *editor)
{
    size_t i;
    for (i = 0; i < (editor->pane->arena.show_lines / 2); ++i) {
        editor_next_line(editor);
    }
}

void editor_scroll_down(Editor *editor)
{
    size_t i;
    for (i = 0; i < (editor->pane->arena.show_lines / 2); ++i) {
        editor_previous_line(editor);
    }
}

void editor_beginning_of_buffer(Editor *editor)
{
    editor_goto_point(editor, 0);
    editor_recognize_arena(editor);
}

void editor_end_of_buffer(Editor *editor)
{
    editor_goto_point(editor, editor->pane->buffer->content.len - 1);
    editor_recognize_arena(editor);
}

void editor_mwheel_scroll_up(Editor *editor)
{
    size_t line_num;
    size_t line_to_move;
    Line next_line;

    if (editor->pane->arena.start > 0) {
        editor->pane->arena.start--;

        line_num = editor_get_current_line_number(editor->pane);

        //out of arena range
        if (line_num > (editor->pane->arena.start + editor->pane->arena.show_lines)) {
            line_to_move = editor->pane->arena.start + editor->pane->arena.show_lines;
            line_to_move = line_to_move > editor->pane->buffer->lines_count ? (editor->pane->buffer->lines_count - 1) : line_to_move;

            next_line = editor->pane->buffer->lines[line_to_move - 5];
            editor_goto_point(editor, next_line.start);
        }
    }
}

void editor_mwheel_scroll_down(Editor *editor)
{
    size_t line_num;
    size_t line_to_move;
    Line next_line;

    if (editor->pane->arena.start < (editor->pane->buffer->lines_count - 1)) {
        editor->pane->arena.start++;

        line_num = editor_get_current_line_number(editor->pane);

        //out of arena range
        if (line_num < editor->pane->arena.start) {
            line_to_move = editor->pane->arena.start;

            next_line = editor->pane->buffer->lines[line_to_move];
            editor_goto_point(editor, next_line.start);
        }
    }
}

void editor_mwheel_scroll(Editor *editor, Sint32 y)
{
    Sint32 i;

    if (y > 0) {
        for (i = 0; i < y; ++i) {
            editor_mwheel_scroll_up(editor);
        }
    } else {
        for (i = y; i < 0; ++i) {
            editor_mwheel_scroll_down(editor);
        }
    }
}

void editor_set_mark(Editor *editor)
{
    editor->mark = editor->pane->position;
    editor->state = SELECTION;
}

void editor_copy_to_clipboard(Editor *editor)
{
    char *copy;
    size_t len, reg_beg;

    if (editor->state != SELECTION) return;

    reg_beg = MIN(editor->mark, editor->pane->position);
    len = MAX(editor->mark, editor->pane->position) - reg_beg;

    copy = calloc(len + 1, sizeof(char));
    memcpy(copy, &editor->pane->buffer->content.data[reg_beg], len);

    if (SDL_SetClipboardText(copy) < 0) {
        fprintf(stderr, "Could not copy to clipboard: %s\n", SDL_GetError());
    }

    free(copy);
}

void editor_cut(Editor *editor)
{
    if (editor->state != SELECTION) return;

    editor_copy_to_clipboard(editor);
    editor_delete_backward(editor);
}

void editor_paste(Editor *editor)
{
    char* clipboard;

    if (!SDL_HasClipboardText()) return;

    if (editor->state == SELECTION) {
        editor_delete_backward(editor);
    }

    clipboard = SDL_GetClipboardText();
    editor_insert(editor, clipboard);
    editor_recognize_arena(editor);

    free(clipboard);
}

void editor_duplicate_line(Editor *editor)
{
    size_t line_num, line_len, pos;
    Line *line;
    char *copy;

    pos = editor->pane->position;
    line_num = editor_get_current_line_number(editor->pane);
    line = &editor->pane->buffer->lines[line_num];
    line_len = line->end - line->start;
    copy = calloc(line_len + 1, sizeof(char));
    memcpy(copy, &editor->pane->buffer->content.data[line->start], line_len);
    editor_move_begginning_of_line(editor);
    editor_insert(editor, copy);
    editor_insert(editor, "\n");

    editor_goto_point(editor, pos);
    editor_next_line(editor);
    editor_recognize_arena(editor);

    free(copy);
}

void editor_user_input_clear(Editor *editor)
{
    StringBuilder *sb;

    sb = &editor->user_input;

    sb_clean(sb);
    editor->state = NONE;
}

void editor_user_search_forward(Editor *editor)
{
    StringBuilder *sb;

    sb = &editor->user_input;

    editor->state = FORWARD_SEARCH;
    sb_clean(sb);
}

void editor_user_search_backward(Editor *editor)
{
    StringBuilder *sb;

    sb = &editor->user_input;

    editor->state = BACKWARD_SEARCH;
    sb_clean(sb);
}

void editor_user_extend_command(Editor *editor)
{
    StringBuilder *sb;

    sb = &editor->user_input;
    editor->state = EXTEND_COMMAND;

    sb_clean(sb);
}

void editor_user_input_insert(Editor *editor, char *text)
{
    size_t i;
    StringBuilder *sb;

    sb = &editor->user_input;

    for (i = 0; i < strlen(text); ++i) {
        sb_append(sb, text[i]);
    }
}

void editor_user_input_delete_backward(Editor *editor)
{
    StringBuilder *sb;
    uint8_t char_len;

    sb = &editor->user_input;
    char_len = utf8_size_char_backward(sb->data, sb->len - 1);
    sb->len -= char_len;
    memset(&sb->data[sb->len], 0, char_len);
}

bool editor_user_search_next(Editor *editor, char *notification)
{
    size_t to_find_len;
    char *to_find;
    long i, threshold;
    bool forward;

    to_find = editor->user_input.data;

    if (to_find == NULL) return false;
    if (strlen(to_find) == 0) return false;

    to_find_len = strlen(to_find);
    forward = true;

    switch (editor->state) {
    case BACKWARD_SEARCH:
        forward = false;
        i = (long) editor->pane->position - 1;
        threshold = 0;
        break;
    case FORWARD_SEARCH:
        i = (long) editor->pane->position + 1;
        threshold = editor->pane->buffer->content.len;
        break;
    default:
        return false;
    }

    while (i != threshold) {
        if (strncasecmp(&editor->pane->buffer->content.data[i], to_find, to_find_len) == 0) {
            editor_goto_point(editor, i);
            editor_recognize_arena(editor);
            return true;
        }
        i += forward ? 1 : -1;
    }

    sprintf(notification, "Not found: %s", to_find);
    editor_user_input_clear(editor);

    return false;
}

void editor_goto_line(Editor *editor, size_t line)
{
    size_t goto_line;
    goto_line = MIN(editor->pane->buffer->lines_count - 1, line - 1);

    editor_goto_point(editor, editor->pane->buffer->lines[goto_line].start);
    editor_recognize_arena(editor);
}

void editor_goto_line_forward(Editor *editor, size_t line)
{
    for (size_t i = 0; i < line; ++i) editor_next_line(editor);
}

void editor_goto_line_backward(Editor *editor, size_t line)
{
    for (size_t i = 0; i < line; ++i) editor_previous_line(editor);
}

bool editor_is_editing_text(Editor *editor)
{
    return editor->state == NONE;
}

void editor_move_line_down(Editor *editor)
{
    size_t curr_line;

    curr_line = editor_get_current_line_number(editor->pane);

    if ((editor->pane->buffer->lines_count - 1) <= curr_line) return;
    if (!editor_is_editing_text(editor)) return;

    editor_move_begginning_of_line(editor);
    editor_set_mark(editor);
    editor_move_end_of_line(editor);
    editor_cut(editor);
    editor_next_line(editor);
    editor_move_begginning_of_line(editor);
    editor_paste(editor);
    editor_set_mark(editor);
    editor_move_end_of_line(editor);
    editor_cut(editor);
    editor_previous_line(editor);
    editor_paste(editor);
    editor_next_line(editor);
    editor_move_begginning_of_line(editor);
}

void editor_move_line_up(Editor *editor)
{
    size_t curr_line;

    curr_line = editor_get_current_line_number(editor->pane);

    if (curr_line == 0) return;
    if (!editor_is_editing_text(editor)) return;

    editor_move_begginning_of_line(editor);
    editor_set_mark(editor);
    editor_move_end_of_line(editor);
    editor_cut(editor);
    editor_previous_line(editor);
    editor_move_begginning_of_line(editor);
    editor_paste(editor);
    editor_set_mark(editor);
    editor_move_end_of_line(editor);
    editor_cut(editor);
    editor_next_line(editor);
    editor_paste(editor);
    editor_previous_line(editor);
    editor_move_begginning_of_line(editor);
}

void editor_print_buffers_names(Editor *editor, char *notification)
{
    size_t i;
    StringBuilder *str;
    char buffer_index[100];

    str = &((StringBuilder) {0});

    sb_append(str, '{');
    for (i = 0; i < editor->buffer_list.len; ++i) {
        sprintf(buffer_index, "%ld", i+1);
        sb_append_many(str, buffer_index);

        sb_append(str, ':');
        sb_append_many(str, editor->buffer_list.buffers[i].file_path);

        if (i < (editor->buffer_list.len - 1)) {
            sb_append(str, ' ');
            sb_append(str, '|');
            sb_append(str, ' ');
        }
    }
    sb_append(str, '}');

    strcpy(notification, str->data);
    sb_free(str);
}

void editor_switch_buffer(Editor *editor, size_t buf_index)
{
    --buf_index;
    if (buf_index >= editor->buffer_list.len) return;

    editor->pane->buffer = &editor->buffer_list.buffers[buf_index];
    editor_recognize_arena(editor);
}

void editor_kill_buffer(Editor *editor, size_t buf_index, char *notification)
{
    size_t i;
    --buf_index;

    if (buf_index >= editor->buffer_list.len) return;
    if (&editor->buffer_list.buffers[buf_index] == editor->pane->buffer) return;

    editor_destory_buffer(&editor->buffer_list.buffers[buf_index]);

    --editor->buffer_list.len;
    for (i = buf_index; i < editor->buffer_list.len; ++i) {
        editor->buffer_list.buffers[i] = editor->buffer_list.buffers[i+1];
    }

    sprintf(notification, "Buffer killed");
}

void editor_mark_forward_word(Editor *editor)
{
    size_t pos;

    pos = editor->pane->position;

    if (editor->state == SELECTION) {
        editor_goto_point(editor, MAX(editor->pane->position, editor->mark));
    }

    editor_word_forward(editor);

    editor->mark = editor->pane->position;
    editor_goto_point(editor, pos);
    editor->state = SELECTION;
}

void editor_delete_word_forward(Editor *editor)
{
    editor_set_mark(editor);
    editor_word_forward(editor);
    editor_delete_backward(editor);
}


int editor_find_word(char *ch, bool *word_beginning, bool *found_word_ending)
{
    uint8_t char_len;
    uint32_t utf8_char_int;

    char_len = utf8_size_char(ch[0]);
    utf8_char_int = utf8_chars_to_int(ch, char_len);

    if (!*word_beginning) {
        if (0x40 < utf8_char_int && utf8_char_int < 0x5B) *word_beginning = true;
        else if (0x60 < utf8_char_int && utf8_char_int < 0x7B) *word_beginning = true;
        else if (0xCFBE < utf8_char_int && utf8_char_int < 0xD4B0) *word_beginning = true;
    }

    if (*word_beginning) {
        if (0x0 < utf8_char_int && utf8_char_int <= 0x40) *found_word_ending = true;
        else if (0x5B <= utf8_char_int && utf8_char_int <= 0x60) *found_word_ending = true;
        else if (0x7B <= utf8_char_int && utf8_char_int <= 0xCFBE) *found_word_ending = true;
        else if (0xD4B0 <= utf8_char_int) *found_word_ending = true;
    }

    return char_len;
}

void editor_word_forward(Editor *editor)
{
    size_t i, move_len;
    bool word_beginning, found_word_ending;

    word_beginning = false;
    found_word_ending = false;

    i = editor->pane->position;
    while (i < editor->pane->buffer->content.len) {
        move_len = editor_find_word(&editor->pane->buffer->content.data[i], &word_beginning, &found_word_ending);

        if (found_word_ending) {
            editor_goto_point(editor, i);
            editor_recognize_arena(editor);
            return;
        }

        i += move_len;
    }

    editor_goto_point(editor, editor->pane->buffer->content.len);
    editor_recognize_arena(editor);
}

void editor_word_backward(Editor *editor)
{
    size_t i, starting_point;
    uint8_t utf8_char_len;
    bool word_beginning, found_word_ending;

    word_beginning = false;
    found_word_ending = false;
    starting_point = editor->pane->position == 0 ? 0 : editor->pane->position;
    i = starting_point;

    while (i > 0) {
        utf8_char_len = utf8_size_char_backward(editor->pane->buffer->content.data, i-1);
        editor_find_word(&editor->pane->buffer->content.data[i - utf8_char_len], &word_beginning, &found_word_ending);

        if (found_word_ending) {
            editor_goto_point(editor, i);
            editor_recognize_arena(editor);
            return;
        }

        i -= utf8_char_len;
    }

    editor_goto_point(editor, 0);
    editor_recognize_arena(editor);
}

void editor_user_input_insert_from_clipboard(Editor *editor)
{
    char *clipboard;

    clipboard = SDL_GetClipboardText();

    if (strlen(clipboard) > EDITOR_MINI_BUFFER_CONTENT_LIMIT) {
        fprintf(stderr, "Could not insert clipboard content to mini buffer because limit exceeded\n");
        return;
    }

    sb_append_many((&editor->user_input), clipboard);
}

void editor_split_pane(Editor *editor)
{
    Pane pane;

    assert(editor->panes_len < PANES_MAX_SIZE);

    pane = (Pane) {0};

    pane.buffer = editor->pane->buffer;
    pane.arena = editor->pane->arena;
    pane.position = 0;

    editor->panes[editor->panes_len++] = pane;
    editor_recognize_arena(editor);
}

void editor_wrap_region_in_parens(Editor *editor)
{
    size_t reg_beg, reg_end;

    if (editor->state != SELECTION) return;

    reg_beg = MIN(editor->mark, editor->pane->position);
    reg_end = MAX(editor->mark, editor->pane->position);

    editor->state = NONE;
    editor_goto_point(editor, reg_end);
    editor_insert(editor, ")");

    editor_goto_point(editor, reg_beg);
    editor_insert(editor, "(");
}
