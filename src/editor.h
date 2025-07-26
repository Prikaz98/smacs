#ifndef EDITOR_H
#define EDITOR_H

#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdbool.h>
#include "common.h"

typedef struct {
    size_t start;
    size_t end;
} Line;

typedef struct {
    char *data;
    size_t len;
    size_t capacity;
} Content;

typedef struct {
    size_t start;
    size_t show_lines;
} Arena;

typedef struct {
    Content content;

    Line *lines;
    size_t lines_count;
    size_t lines_cap;

    Arena arena;
    char *file_path;
} Buffer;

#define buf_line_append(buf, ln)                                                            \
    do {                                                                                    \
        if (buf->lines_count >= buf->lines_cap) {                                           \
            buf->lines_cap = buf->lines_cap == 0 ? 10 : buf->lines_cap * 2; \
            buf->lines = realloc(buf->lines, buf->lines_cap * sizeof(*buf->lines));         \
            memset(&buf->lines[buf->lines_count], 0, buf->lines_cap - buf->lines_count);    \
        }                                                                                   \
        buf->lines[buf->lines_count++] = ln;                                                \
    } while(0)

#define buf_line_clean(buf)                                       \
    do {                                                          \
        buf->lines_count = 0;                                     \
        memset(&buf->lines[buf->lines_count], 0, buf->lines_cap); \
    } while(0)

typedef struct {
    Buffer *buffers;
    size_t len;
    size_t cap;
} Buffer_List;

#define buffer_list_append(bl, buf)                                             \
    do {                                                                        \
        if (bl->len >= bl->cap) {                                               \
            bl->cap = bl->cap == 0 ? 5 : bl->cap * 2;                           \
            bl->buffers = realloc(bl->buffers, bl->cap * sizeof(*bl->buffers)); \
            memset(&bl->buffers[bl->len], 0, bl->cap - bl->len);                \
        }                                                                       \
        bl->buffers[bl->len++] = buf;                                           \
    } while(0)

#define buffer_list_clean(bl)                      \
    do {                                           \
        bl->len = 0;                               \
        memset(&bl->buffers[bl->len], 0, bl->cap); \
    } while(0)

typedef struct {
	//TODO: move it to buffer
    size_t position;
    Buffer *buffer;

    size_t mark;

    bool selection;
    bool searching;
    bool reverse_searching;
    bool extend_command;
    StringBuilder user_input;
    Buffer_List buffer_list;
} Editor;

#define EDITOR_CONTENT_CAP 256

void editor_insert(Editor *editor, char *str);
void editor_delete_backward(Editor *editor);
void editor_delete_forward(Editor *editor);
int  editor_save(Editor* editor);
int  editor_read_file(Editor *editor, char *file_path);
void editor_determine_lines(Editor *editor);
size_t editor_get_current_line_number(Editor *editor);

void editor_next_line(Editor *editor);
void editor_previous_line(Editor *editor);
void editor_char_backward(Editor *editor);
void editor_char_forward(Editor *editor);
void editor_word_forward(Editor *editor);
void editor_word_backward(Editor *editor);
void editor_move_end_of_line(Editor *editor);
void editor_move_begginning_of_line(Editor *editor);
void editor_kill_line(Editor *editor);

void editor_destroy(Editor *editor);
void editor_recenter_top_bottom(Editor *editor);
void editor_scroll_up(Editor *editor);
void editor_scroll_down(Editor *editor);
void editor_beginning_of_buffer(Editor *editor);
void editor_end_of_buffer(Editor *editor);
void editor_mwheel_scroll(Editor *editor, Sint32 y);
void editor_set_mark(Editor *editor);
void editor_copy_to_clipboard(Editor *editor);
void editor_paste(Editor *editor);
void editor_cut(Editor *editor);
void editor_duplicate_line(Editor *editor);
void editor_move_line_up(Editor *editor);
void editor_move_line_down(Editor *editor);

//TODO: ignore case
void editor_user_search_forward(Editor *editor);
void editor_user_search_backward(Editor *editor);

void editor_user_extend_command(Editor *editor);

void editor_user_input_clear(Editor *editor);
void editor_user_input_insert(Editor *editor, char *text);
void editor_user_input_delete_backward(Editor *editor);
bool editor_user_search_next(Editor *editor, char *notification);

void editor_goto_line(Editor *editor, size_t line);
void editor_goto_line_forward(Editor *editor, size_t line);
void editor_goto_line_backward(Editor *editor, size_t line);

bool editor_is_editing_text(Editor *editor);
void editor_print_buffers_names(Editor *editor, char *notification);
void editor_swtich_buffer(Editor *editor, size_t buf_index);

#endif
