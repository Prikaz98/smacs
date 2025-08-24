#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>
#include <assert.h>
#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

#include "editor.h"
#include "utf8.h"
#include "common.h"

void append_char(Content *content, char ch, size_t pos)
{
    Content cont;

    if (content == NULL) {
        fprintf(stderr, "Could not append char content is NULL\n");
        return;
    }

    cont = *content;

    if (cont.len >= cont.capacity) {
        cont.capacity = cont.capacity == 0 ? EDITOR_CONTENT_CAP : cont.capacity * 2;
        cont.data = realloc(cont.data, cont.capacity * sizeof(cont.data));
        memset(&cont.data[cont.len], 0, cont.capacity - cont.len);
    }

    if (pos == cont.len) {
        cont.data[cont.len++] = ch;
    } else if (pos < cont.len) {
        memmove(&cont.data[pos + 1], &cont.data[pos], cont.len - pos);
        cont.data[pos] = ch;
        ++cont.len;
    } else {
        fprintf(stderr, "Out of range insert char char_pos: %ld content_len: %ld\n", pos, cont.len);
    }

    *content = cont;
}

void editor_goto_point(Editor *editor, size_t pos)
{
    size_t max_len;

    max_len = editor->pane->buffer->content.len;
    if (pos > max_len) return;

    editor->pane->position = pos;
    editor_recognize_arena(editor);
}

size_t editor_reg_beg(Editor *editor)
{
    return MIN(editor->mark, editor->pane->position);
}

size_t editor_reg_end(Editor *editor)
{
    return MIN(editor->pane->buffer->content.len, MAX(editor->mark, editor->pane->position));
}

void editor_delete_backward(Editor *editor)
{
    Buffer *buf;
    Content *content;
    size_t reg_beg, reg_end, delete_len;

    buf = editor->pane->buffer;
    content = &buf->content;

    if (editor->state == SELECTION) {
        reg_beg = editor_reg_beg(editor);
        reg_end = editor_reg_end(editor);
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
    register size_t i;

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
    register size_t i;
    Line *line;

    if (pane->buffer->len == 0) return 0;

    for (i = 0; i < pane->buffer->len; ++i) {
        line = &pane->buffer->data[i];
        if (line->start <= pane->position && pane->position <= line->end) {
            return i;
        }
    }

    return 0;
}

void editor_recognize_arena(Editor *editor)
{
    size_t line_num;
    register size_t pane_i;
    Arena *arena;
    Pane *pane;

    for (pane_i = 0; pane_i < editor->panes_len; ++pane_i) {
        pane = &editor->panes[pane_i];
        line_num = editor_get_current_line_number(pane);
        arena = &pane->arena;

        if (line_num >= (arena->start + arena->show_lines - 2)) {
            arena->start = MIN(pane->buffer->len - 1, line_num - arena->show_lines / 2);
        } else if (line_num < arena->start) {
            arena->start = line_num;
        }
    }
}

void editor_next_line(Editor *editor)
{
    size_t next_pos, line_num;
    Line *line;

    line_num = editor_get_current_line_number(editor->pane);

    if (editor->pane->buffer->len > 0) {
        line = &editor->pane->buffer->data[line_num];
        editor->pane->buffer->column = MAX(editor->pane->buffer->column, editor->pane->position - line->start);

        ++line_num;
        if (editor->pane->buffer->len > line_num) {
            line = &editor->pane->buffer->data[line_num];
            next_pos = line->start + editor->pane->buffer->column;

            editor_goto_point(editor, MIN(next_pos, line->end));

            editor_recognize_arena(editor);
        }
    }
}

void editor_previous_line(Editor *editor)
{
    size_t next_pos, line_num;
    Line *line;

    line_num = editor_get_current_line_number(editor->pane);
    line = &editor->pane->buffer->data[line_num];
    editor->pane->buffer->column = MAX(editor->pane->buffer->column, editor->pane->position - line->start);

    if (line_num != 0) {
        line_num--;
        line = &editor->pane->buffer->data[line_num];

        next_pos = line->start + editor->pane->buffer->column;
        editor_goto_point(editor, MIN(next_pos, line->end));

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
    register size_t i;
    Line *line;

    for (i = 0; i < editor->pane->buffer->len; ++i) {
        line = &editor->pane->buffer->data[i];
        if (line->start <= editor->pane->position && editor->pane->position <= line->end) {
            editor_goto_point(editor, line->end);
            return;
        }
    }
}

void editor_move_begginning_of_line(Editor *editor)
{
    register size_t i;
    Line *line;

    for (i = 0; i < editor->pane->buffer->len; ++i) {
        line = &editor->pane->buffer->data[i];
        if (line->start <= editor->pane->position && editor->pane->position <= line->end) {
            editor_goto_point(editor, line->start);
            editor->pane->buffer->column = 0;
            return;
        }
    }
}

void editor_determine_lines(Editor *editor)
{
    register size_t i;
    size_t beg;
    Buffer *buffer;

    buffer = editor->pane->buffer;
    buffer->len = 0;

    if (buffer->content.len == 0) return;

    beg = 0;
    for (i = 0; i < buffer->content.len; ++i) {
        if (buffer->content.data[i] == '\n') {
            gb_append(buffer, ((Line) {beg, i}));
            beg = i + 1;
        }
    }

    gb_append(buffer, ((Line) {beg, i}));
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
    register size_t i;

    for (i = 0; i < editor->buffer_list.len; ++i) {
        if (strcmp(editor->buffer_list.data[i].file_path, file_path) == 0) {
            return &editor->buffer_list.data[i];
        }
    }

    gb_append((&editor->buffer_list), ((Buffer) {0}));
    return &editor->buffer_list.data[editor->buffer_list.len - 1];
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
    register size_t i;
    size_t del_count;
    Line *line;

    for (i = 0; i < editor->pane->buffer->len; ++i) {
        line = &editor->pane->buffer->data[i];
        if (line->start <= editor->pane->position && editor->pane->position <= line->end) {
            del_count = (size_t) line->end - editor->pane->position;
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
        buf->content.len = 0;
        buf->content.capacity = 0;

        gb_free(buf);
    }
}

void editor_destroy(Editor *editor)
{
    register size_t i;

    if (editor->pane->buffer->len > 0) {
        free(editor->pane->buffer->data);
        editor->pane->buffer->data = NULL;
    }

    for (i = 0; i < editor->buffer_list.len; ++i) {
        editor_destory_buffer(&editor->buffer_list.data[i]);
    }

    editor->buffer_list.len = 0;
    free(editor->buffer_list.data);
    editor->buffer_list.data = NULL;

    editor_completor_clean(editor);
    gb_free(&(editor->completor.filtered));
    gb_free(&(editor->completor));
}

void editor_recenter_top_bottom(Editor *editor)
{
    int line_num, center, half_screen;
    Arena *arena;

    line_num = (int) editor_get_current_line_number(editor->pane);
    arena = &editor->pane->arena;
    half_screen = (int) arena->show_lines / 2;
    center = MIN(editor->pane->buffer->len, arena->start + half_screen);

    //top -> bottom
    if (arena->start == (size_t) line_num) {
        arena->start = (size_t) MAX(line_num - (int) arena->show_lines + 2, 0);
    //center -> top
    } else if (line_num == center) {
        arena->start = line_num;
    //any -> center
    } else {
        arena->start = (size_t) MAX(line_num - half_screen, 0);
    }
}

void editor_scroll_up(Editor *editor)
{
    register size_t i;
    for (i = 0; i < (editor->pane->arena.show_lines / 2); ++i) {
        editor_next_line(editor);
    }
}

void editor_scroll_down(Editor *editor)
{
    register size_t i;
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
    Line *next_line;

    if (editor->pane->arena.start > 0) {
        editor->pane->arena.start--;

        line_num = editor_get_current_line_number(editor->pane);

        //out of arena range
        if (line_num > (editor->pane->arena.start + editor->pane->arena.show_lines)) {
            line_to_move = editor->pane->arena.start + editor->pane->arena.show_lines;
            line_to_move = line_to_move > editor->pane->buffer->len ? (editor->pane->buffer->len - 1) : line_to_move;

            next_line = &editor->pane->buffer->data[line_to_move - 5];
            editor_goto_point(editor, next_line->start);
        }
    }
}

void editor_mwheel_scroll_down(Editor *editor)
{
    size_t line_num;
    size_t line_to_move;
    Line *next_line;

    if (editor->pane->arena.start < (editor->pane->buffer->len - 1)) {
        ++editor->pane->arena.start;

        line_num = editor_get_current_line_number(editor->pane);

        //out of arena range
        if (line_num < editor->pane->arena.start) {
            line_to_move = editor->pane->arena.start;

            next_line = &editor->pane->buffer->data[line_to_move];
            editor_goto_point(editor, next_line->start);
        }
    }
}

void editor_mwheel_scroll(Editor *editor, Sint32 y)
{
    register Sint32 i;

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
    size_t len, reg_beg, reg_end;

    if (editor->state != SELECTION) return;

    reg_beg = editor_reg_beg(editor);
    reg_end = editor_reg_end(editor);
    len = reg_end - reg_beg;

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
    line = &editor->pane->buffer->data[line_num];
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
    sb_clean((&editor->user_input));
    editor->state = NONE;
}

void editor_user_search_forward(Editor *editor)
{
    editor->state = FORWARD_SEARCH;
    sb_clean((&editor->user_input));
}

void editor_user_search_backward(Editor *editor)
{
    editor->state = BACKWARD_SEARCH;
    sb_clean((&editor->user_input));
}

void editor_user_extend_command(Editor *editor)
{
    editor->state = EXTEND_COMMAND;
    sb_clean((&editor->user_input));
}

void editor_user_input_insert(Editor *editor, char *text)
{
    sb_append_many(&editor->user_input, text);
}

void editor_user_input_delete_backward(Editor *editor)
{
    StringBuilder *sb;
    uint8_t char_len;

    if (editor->user_input.len == 0) return;

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
    goto_line = MIN(editor->pane->buffer->len - 1, line - 1);

    editor_goto_point(editor, editor->pane->buffer->data[goto_line].start);
    editor_recognize_arena(editor);
}

void editor_goto_line_forward(Editor *editor, size_t line)
{
    for (register size_t i = 0; i < line; ++i) editor_next_line(editor);
}

void editor_goto_line_backward(Editor *editor, size_t line)
{
    for (register size_t i = 0; i < line; ++i) editor_previous_line(editor);
}

bool editor_is_editing_text(Editor *editor)
{
    return editor->state == NONE;
}

void editor_move_line_down(Editor *editor)
{
    size_t curr_line;

    curr_line = editor_get_current_line_number(editor->pane);

    if ((editor->pane->buffer->len - 1) <= curr_line) return;
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

void editor_switch_buffer(Editor *editor, size_t buf_index)
{
    if (buf_index >= editor->buffer_list.len) return;

    editor->pane->buffer->last_position = editor->pane->position;
    editor->pane->buffer = &editor->buffer_list.data[buf_index];
    editor->pane->position = editor->pane->buffer->last_position;

    editor_recognize_arena(editor);
}

void editor_kill_buffer(Editor *editor, size_t buf_index, char *notification)
{
    register size_t i;

    if (buf_index >= editor->buffer_list.len) return;
    if (&editor->buffer_list.data[buf_index] == editor->pane->buffer) return;

    sprintf(notification, "Buffer killed %s", editor->buffer_list.data[buf_index].file_path);
    editor_destory_buffer(&editor->buffer_list.data[buf_index]);

    --editor->buffer_list.len;
    for (i = buf_index; i < editor->buffer_list.len; ++i) {
        editor->buffer_list.data[i] = editor->buffer_list.data[i+1];
    }
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
    bool found_beg, found_end;

    found_beg = *word_beginning;
    found_end = *found_word_ending;

    char_len = utf8_size_char(ch[0]);
    utf8_char_int = utf8_chars_to_int(ch, char_len);

    if (!found_beg) {
        if (0x40 < utf8_char_int && utf8_char_int < 0x5B) found_beg = true;
        else if (0x60 < utf8_char_int && utf8_char_int < 0x7B) found_beg = true;
        else if (0xCFBE < utf8_char_int && utf8_char_int < 0xD4B0) found_beg = true;
    }

    if (found_beg) {
        if (0x0 < utf8_char_int && utf8_char_int <= 0x40) found_end = true;
        else if (0x5B <= utf8_char_int && utf8_char_int <= 0x60) found_end = true;
        else if (0x7B <= utf8_char_int && utf8_char_int <= 0xCFBE) found_end = true;
        else if (0xD4B0 <= utf8_char_int) found_end = true;
    }

    *word_beginning = found_beg;
    *found_word_ending = found_end;

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

    editor->panes[editor->panes_len++] = pane;
    editor_recognize_arena(editor);
}

void editor_wrap_region_in_parens(Editor *editor)
{
    size_t reg_beg, reg_end;

    if (editor->state != SELECTION) return;

    reg_beg = editor_reg_beg(editor);
    reg_end = editor_reg_end(editor);

    editor->state = NONE;
    editor_goto_point(editor, reg_end);
    editor_insert(editor, ")");

    editor_goto_point(editor, reg_beg);
    editor_insert(editor, "(");
}

void editor_next_pane(Editor *editor)
{
    register size_t i;

    editor->state = NONE;
    for (i = 0; i < editor->panes_len; ++i) {
        if (&editor->panes[i] == editor->pane) {
            editor->pane = &editor->panes[(i+1) >= editor->panes_len ? 0 : (i+1)];
            break;
        }
    }
}

void editor_close_pane(Editor *editor)
{
    if (editor->panes_len > 1) {
        --editor->panes_len;
        editor->pane = &editor->panes[0];
    }
}

void editor_upper_region(Editor *editor)
{
    Content *content;
    size_t reg_beg, reg_end;
    register size_t i;

    if (editor->state != SELECTION) return;

    reg_beg = editor_reg_beg(editor);
    reg_end = editor_reg_end(editor);
    content = &editor->pane->buffer->content;

    for (i = reg_beg; i < reg_end; ++i) {
        content->data[i] = (char) toupper((int)content->data[i]);
    }

    editor->state = NONE;
}

void editor_upper(Editor *editor)
{
    if (editor->state == SELECTION) {
        editor_upper_region(editor);
    }
}


void editor_lower_region(Editor *editor)
{
    Content *content;
    register size_t i;
    size_t reg_beg, reg_end;

    if (editor->state != SELECTION) return;

    reg_beg = editor_reg_beg(editor);
    reg_end = editor_reg_end(editor);
    content = &editor->pane->buffer->content;

    i = reg_beg;
    for (i = reg_beg; i < reg_end; ++i) {
        content->data[i] = (char) tolower((int)content->data[i]);
    }

    editor->state = NONE;
}

void editor_completor_clean(Editor *editor)
{
    for (register size_t i = 0; i < editor->completor.len; ++i) {
        if (editor->completor.data[i] != NULL) {
            free(editor->completor.data[i]);
            editor->completor.data[i] = NULL;
        }
    }

    editor->completor.len = 0;
}

void editor_lower(Editor *editor)
{
    if (editor->state == SELECTION) {
        editor_lower_region(editor);
    }
}

void editor_buffer_switch(Editor *editor)
{
    register size_t i;

    editor_user_input_clear(editor);
    editor->state = COMPLETION;
    editor_completor_clean(editor);

    for (i = 0; i < editor->buffer_list.len; ++i) {
        gb_append(&(editor->completor), strdup(editor->buffer_list.data[i].file_path));
    }

    editor_completion_actualize(editor);
}

void editor_completion_actualize(Editor *editor)
{
    register size_t i;
    char *elem;
    bool need_append;

    if ((editor->state & COMPLETION) == 0) return;

    editor->completor.filtered.len = 0;

    for (i = 0; i < editor->completor.len; ++i) {
        need_append = false;
        elem = editor->completor.data[i];

        if (editor->user_input.len == 0) need_append = true;
        if (contains_ignore_case(elem, strlen(elem), editor->user_input.data, editor->user_input.len)) need_append = true;

        if (need_append) {
            gb_append(&(editor->completor.filtered), elem);
        }
    }
}

bool editor_buffer_switch_complete(Editor *editor)
{
    register size_t i;
    char *buffer_target, *buffer_name;
    if (editor->completor.filtered.len == 0) return false;

    buffer_target = editor->completor.filtered.data[0];

    for (i = 0; i < editor->buffer_list.len; ++i) {
        buffer_name = editor->buffer_list.data[i].file_path;
        if (strcmp(buffer_target, buffer_name) == 0) {
            editor_switch_buffer(editor, i);
            editor->state = NONE;
            return true;
        }
    }
    return false;
}

void editor_set_dir_by_current_file(Editor *editor)
{
    register long i;
    char *fp;
    size_t fp_len;

    if (strlen(editor->dir) == 0) {
        getcwd(editor->dir, 1024);
        return;
    }

    fp = editor->pane->buffer->file_path;
    fp_len = strlen(fp);

    for (i = (long)fp_len-1; i >= 0; --i) {
        if (fp[i] == '/') {
            memcpy(editor->dir, fp, i);
            editor->dir[i] = '\0';
            break;
        }
    }

}

void editor_find_file(Editor *editor, bool refresh_dir)
{
    DIR *dp;
    struct dirent *ep;
    bool should_add;

    if (refresh_dir) editor_set_dir_by_current_file(editor);

    dp = opendir(editor->dir);
    if (dp == NULL) {
        fprintf(stderr, "Could not open directory by name %s\n", editor->dir);
        return;
    }

    editor_user_input_clear(editor);
    editor->state = FILE_SEARCH;
    editor_completor_clean(editor);

    gb_append(&(editor->completor), strdup(EDITOR_DIR_PREV));

    while ((ep = readdir(dp)) != NULL) {
        should_add = true;
        if (strcmp(ep->d_name, EDITOR_DIR_CUR) == 0) should_add = false;
        if (strcmp(ep->d_name, EDITOR_DIR_PREV) == 0) should_add = false;

        if (should_add) {
            gb_append(&(editor->completor), strdup(ep->d_name));
        }
    }

    closedir(dp);
    editor_completion_actualize(editor);
}

int editor_is_directory(const char *path)
{
    struct stat path_stat;
    stat(path, &path_stat);
    return S_ISDIR(path_stat.st_mode);
}

bool editor_find_file_complete(Editor *editor)
{
    char *file_path;
    size_t dir_len;
    long dir_i;
    StringBuilder *full_path;
    bool file_is_dir;

    file_path = editor->completor.filtered.len == 0 ? editor->user_input.data : editor->completor.filtered.data[0];

    if (strcmp(file_path, EDITOR_DIR_PREV) == 0) {
        dir_len =  strlen(editor->dir);
        dir_i = (long) dir_len;

        while(dir_i > 0) {
            if (editor->dir[dir_i] == EDITOR_DIR_SLASH) break;
            --dir_i;
        }
        memset(&editor->dir[dir_i], 0, dir_len - dir_i);
        if (strlen(editor->dir) == 0) {
            editor->dir[0] = '.';
            editor->dir[1] = '\0';
        }

        editor_find_file(editor, false);
        return true;
    }

    full_path = (&(StringBuilder){0});
    sb_append_many(full_path, editor->dir);
    sb_append(full_path, EDITOR_DIR_SLASH);
    sb_append_many(full_path, file_path);

    file_is_dir = false;
    if (editor_is_directory(full_path->data)) file_is_dir = true;
    if (editor->completor.filtered.len == 0) file_is_dir = false;

    if (file_is_dir) {
        memcpy(&editor->dir[0], full_path->data, full_path->len);
        editor->dir[full_path->len] = '\0';
        editor_find_file(editor, false);
    } else {
        editor_read_file(editor, full_path->data);
        editor->state = NONE;
    }

    sb_free(full_path);
    return true;
}

void editor_completion_next_match(Editor *editor)
{
    char *tmp;
    size_t len;

    if ((editor->state & COMPLETION) == 0) return;
    if (editor->completor.filtered.len == 0) return;
    if (editor->completor.filtered.len == 1) return;

    len = editor->completor.filtered.len;
    tmp = editor->completor.filtered.data[0];

    memmove(&editor->completor.filtered.data[0], &editor->completor.filtered.data[1], (len - 1) * sizeof(void*));
    editor->completor.filtered.data[len - 1] = tmp;
}

void editor_completion_prev_match(Editor *editor)
{
    char *tmp;
    size_t len;

    if ((editor->state & COMPLETION) == 0) return;
    if (editor->completor.filtered.len == 0) return;
    if (editor->completor.filtered.len == 1) return;

    len = editor->completor.filtered.len;
    tmp = editor->completor.filtered.data[len - 1];

    memmove(&editor->completor.filtered.data[1], &editor->completor.filtered.data[0], (len - 1) * sizeof(void*));
    editor->completor.filtered.data[0] = tmp;
}

void editor_new_line(Editor *editor)
{
    size_t pos, i;
    char ch, *buffer;

    buffer = SDL_GetClipboardText();

    pos = editor->pane->position;
    editor_move_begginning_of_line(editor);
    i = editor->pane->position;
    editor_set_mark(editor);
    while (i < pos && ((ch = editor->pane->buffer->content.data[i]) == ' ' || ch == '\t')) {
        ++i;
    }
    editor_goto_point(editor, i);
    editor_copy_to_clipboard(editor);
    editor->state = NONE;
    editor_goto_point(editor, pos);
    editor_insert(editor, "\n");
    editor_paste(editor);
    SDL_SetClipboardText(buffer);
}
