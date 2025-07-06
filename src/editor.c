#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdio.h>
#include <sys/param.h>

#include "editor.h"

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

void editor_delete_backward(Editor *editor)
{
    Buffer *buf;
    Content *content;
    size_t delete_len, reg_beg, reg_end;

    if (editor->position == 0) return;

    buf = &editor->buffer;
    content = &buf->content;

    if (editor->selection) {
        reg_beg = MIN(editor->mark, editor->position);
        reg_end = MAX(editor->mark, editor->position);
        delete_len = reg_end - reg_beg;

        memmove(&content->data[reg_beg], &content->data[reg_end], content->len - reg_end);
        content->len -= delete_len;
        editor->position = reg_beg;
    } else {
        memmove(&content->data[editor->position - 1], &content->data[editor->position], content->len - editor->position + 1);
        content->len--;
        editor->position--;
    }

    memset(&content->data[content->len], 0, content->capacity - content->len);

    editor_determine_lines(editor);
    editor->selection = false;
}

void editor_delete_forward(Editor *editor)
{
    Buffer *buf = &editor->buffer;
    Content *content = &buf->content;
    if (editor->position >= content->len) return;

    memmove(&content->data[editor->position], &content->data[editor->position + 1], content->len - editor->position);
    content->len--;

    editor_determine_lines(editor);
    editor->selection = false;
}

void editor_insert(Editor *editor, char *str)
{
    Buffer *buf = &editor->buffer;
    size_t i;

    if (str == NULL) return;

    Content *content = &buf->content;

    if (content == NULL) {
        fprintf(stderr, "Content is null\n");
        return;
    }

    size_t str_size = strlen(str);
    for (i = 0; i < str_size; i++) {
        append_char(content, str[i], editor->position + i);
    }

    editor->position += str_size;
    editor_determine_lines(editor);
    editor->selection = false;
}

size_t editor_get_current_line_number(Editor *editor)
{
    size_t i;
    Line line;

    if (editor->buffer.lines_count == 0) return 0;

    for (i = 0; i < editor->buffer.lines_count; i++) {
        line = editor->buffer.lines[i];
        if (line.start <= editor->position && editor->position <= line.end) {
            return i;
        }
    }

    return 0;
}

void editor_recognize_arena(Editor *editor)
{
    size_t line_num;
    Arena *arena;

    line_num = editor_get_current_line_number(editor);
    arena = &editor->buffer.arena;

    if (line_num >= (arena->start + arena->show_lines)) {
        arena->start = line_num - (arena->show_lines / 2);
    } else if (line_num < arena->start) {
        arena->start = line_num;
    }
}

void editor_next_line(Editor *editor)
{
    size_t col, next_pos, line_num;
    Line line;

    line_num = editor_get_current_line_number(editor);

    if (editor->buffer.lines_count > 0) {
        line = editor->buffer.lines[line_num];
        col = editor->position - line.start;

        line_num++;
        if (editor->buffer.lines_count > line_num) {
            line = editor->buffer.lines[line_num];
            next_pos = line.start + col;
            editor->position = next_pos > line.end ? line.end : next_pos;

            editor_recognize_arena(editor);
        }
    }
}

void editor_previous_line(Editor *editor)
{
    size_t col, next_pos, line_num;
    Line line;

    line_num = editor_get_current_line_number(editor);
    line = editor->buffer.lines[line_num];
    col = editor->position - line.start;

    if (line_num != 0) {
        line_num--;
        line = editor->buffer.lines[line_num];

        next_pos = line.start + col;
        editor->position = next_pos > line.end ? line.end : next_pos;

        editor_recognize_arena(editor);
    }
}

void editor_char_forward(Editor *editor)
{
    editor->position = MIN(editor->position + 1, editor->buffer.content.len);
}

void editor_char_backward(Editor *editor)
{
    editor->position = (size_t) MAX(((int)editor->position) - 1, 0);
}

void editor_move_end_of_line(Editor *editor)
{
    size_t i;
    Line line;

    for (i = 0; i < editor->buffer.lines_count; i++) {
        line = editor->buffer.lines[i];
        if (line.start <= editor->position && editor->position <= line.end) {
            editor->position = line.end;
            return;
        }
    }
}

void editor_move_begginning_of_line(Editor *editor)
{
    size_t i;
    Line line;

    for (i = 0; i < editor->buffer.lines_count; i++) {
        line = editor->buffer.lines[i];
        if (line.start <= editor->position && editor->position <= line.end) {
            editor->position = line.start;
            return;
        }
    }
}

void editor_determine_lines(Editor *editor)
{
    size_t n, i, beg;
    Buffer *buffer;

    buffer = &editor->buffer;

    if (buffer->lines_count > 0) {
        free(buffer->lines);
        buffer->lines = NULL;
        buffer->lines_count = 0;
    }

    if (buffer->content.len == 0) return;

    n = 1;
    for (i = 0; i < buffer->content.len; i++) {
        if (buffer->content.data[i] == '\n') {
            n++;
        }
    }


    buffer->lines = calloc(n + 1, sizeof(Line));

    beg = 0;
    for (i = 0; i < buffer->content.len; i++) {
        if (buffer->content.data[i] == '\n') {
            buffer->lines[buffer->lines_count++] = (Line) {beg, i};
            beg = i + 1;
        }
    }

    buffer->lines[buffer->lines_count++] = (Line) {beg, i};
}

int editor_save(Buffer *buf)
{
    FILE *out;
    Content content;

    if (buf->file_path == NULL) {
        fprintf(stderr, "No file_path\n");
        return 1;
    }

    if (strlen(buf->file_path) <= 0) {
        fprintf(stderr, "file_path is invalid\n");
        return 1;
    }

    out = fopen(buf->file_path, "w");
    if (out == NULL) {
        fprintf(stderr, "Could not save file %s\n", buf->file_path);
        return 1;
    }

    content = buf->content;
    if (content.len > 0) {
        fwrite(content.data, sizeof(char), content.len, out);
    }

    fclose(out);
    fprintf(stderr, "Wrote file %s\n", buf->file_path);
    return 0;
}

int editor_read_file(Editor *editor, char *file_path)
{
    FILE *in;
    Content content;
    char next;
    size_t len;

    in = fopen(file_path, "r");
    if (in == NULL) {
        fprintf(stderr, "Could not read file %s\n", file_path);
        return 1;
    }

    content = (Content) {0};
    len = 0;
    while ((next = fgetc(in)) != EOF) {
        append_char(&content, next, len++);
    }

    fclose(in);

    editor->buffer = (Buffer) {content, NULL, 0, {0, 0}, 0};
    editor->buffer.file_path = file_path;
    editor->position = 0;

    editor_determine_lines(editor);

    fprintf(stderr, "Read file %s\n", file_path);
    return 0;
}

void editor_kill_line(Editor *editor)
{
    size_t i, del_count;
    Line line;

    for (i = 0; i < editor->buffer.lines_count; i++) {
        line = editor->buffer.lines[i];
        if (line.start <= editor->position && editor->position <= line.end) {
            del_count = line.end - editor->position;
            if (del_count > 0) {
                memmove(&editor->buffer.content.data[editor->position], &editor->buffer.content.data[line.end], editor->buffer.content.len - editor->position);
                editor->buffer.content.len -= del_count;
            } else {
                editor_delete_forward(editor);
            }
        }
    }

    editor_determine_lines(editor);
    editor->selection = false;
}

void editor_destroy(Editor *editor)
{
    if (editor->buffer.lines_count > 0) {
        free(editor->buffer.lines);
        editor->buffer.lines = NULL;
    }

    if (editor->buffer.content.len > 0) {
        free(editor->buffer.content.data);
        editor->buffer.content.data = NULL;
    }
}

void editor_recenter(Editor *editor)
{
    int line_num;
    Arena *arena;

    line_num = (int) editor_get_current_line_number(editor);
    arena = &editor->buffer.arena;

    arena->start = (size_t) MAX(line_num - ((int) arena->show_lines / 2), 0);
}

void editor_scroll_up(Editor *editor)
{
    size_t i;
    for (i = 0; i < (editor->buffer.arena.show_lines / 2); i++) {
        editor_next_line(editor);
    }
}

void editor_scroll_down(Editor *editor)
{
    size_t i;
    for (i = 0; i < (editor->buffer.arena.show_lines / 2); i++) {
        editor_previous_line(editor);
    }
}

void editor_beginning_of_buffer(Editor *editor)
{
    editor->position = 0;
    editor_recognize_arena(editor);
}

void editor_end_of_buffer(Editor *editor)
{
    editor->position = editor->buffer.content.len;
    editor_recognize_arena(editor);
}

void editor_mwheel_scroll_up(Editor *editor)
{
    size_t line_num;
    size_t line_to_move;
    Line next_line;

    if (editor->buffer.arena.start > 0) {
        editor->buffer.arena.start--;

        line_num = editor_get_current_line_number(editor);

        //out of arena range
        if (line_num > (editor->buffer.arena.start + editor->buffer.arena.show_lines)) {
            line_to_move = editor->buffer.arena.start + editor->buffer.arena.show_lines;
            line_to_move = line_to_move > editor->buffer.lines_count ? (editor->buffer.lines_count - 1) : line_to_move;

            next_line = editor->buffer.lines[line_to_move - 5];
            editor->position = next_line.start;
        }
    }
}

void editor_mwheel_scroll_down(Editor *editor)
{
    size_t line_num;
    size_t line_to_move;
    Line next_line;

    if (editor->buffer.arena.start < editor->buffer.lines_count) {
        editor->buffer.arena.start++;

        line_num = editor_get_current_line_number(editor);

        //out of arena range
        if (line_num < editor->buffer.arena.start) {
            line_to_move = editor->buffer.arena.start;

            next_line = editor->buffer.lines[line_to_move];
            editor->position = next_line.start;
        }
    }
}

void editor_mwheel_scroll(Editor *editor, Sint32 y)
{
    Sint32 i;

    if (y > 0) {
        for (i = 0; i < y; i++) {
            editor_mwheel_scroll_up(editor);
        }
    } else {
        for (i = y; i < 0; i++) {
            editor_mwheel_scroll_down(editor);
        }
    }
}

void editor_set_mark(Editor *editor)
{
    editor->mark = editor->position;
    editor->selection = true;
}

void editor_copy_to_clipboard(Editor *editor)
{
    char *copy;
    size_t len, reg_beg;

    if (!editor->selection) return;

    reg_beg = MIN(editor->mark, editor->position);
    len = MAX(editor->mark, editor->position) - reg_beg;

    copy = calloc(len, sizeof(char));
    memcpy(copy, &editor->buffer.content.data[reg_beg], len);

    if (SDL_SetClipboardText(copy) < 0) {
        fprintf(stderr, "Could not copy to clipboard: %s\n", SDL_GetError());
    }

    free(copy);
}

void editor_paste(Editor *editor)
{
    char* clipboard;

    if (!SDL_HasClipboardText()) return;

    clipboard = SDL_GetClipboardText();
    editor_insert(editor, clipboard);

    free(clipboard);
}
