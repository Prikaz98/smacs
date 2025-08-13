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
    size_t column;

    Content content;

    Line *lines;
    size_t lines_count;
    size_t lines_cap;

    char *file_path;

    bool need_to_save;

    size_t last_position;
} Buffer;

typedef struct {
    Buffer *data;
    size_t len;
    size_t cap;
} Buffer_List;

typedef struct {
    Buffer *buffer;

    uint32_t x;
    uint32_t w;
    uint32_t h;

    size_t position;
    Arena arena;
} Pane;

#define PANES_MAX_SIZE 3

typedef enum {
    NONE            = 0x000,
    SELECTION       = 0x001,
    FORWARD_SEARCH  = 0x040,
    BACKWARD_SEARCH = 0x080,
    EXTEND_COMMAND  = 0x100,
    SEARCH          = FORWARD_SEARCH | BACKWARD_SEARCH,
} Editor_State;

typedef struct {
    Pane panes[PANES_MAX_SIZE];
    size_t panes_len;

    Pane *pane;

    size_t mark;

    Editor_State state;
    StringBuilder user_input;
    Buffer_List buffer_list;
} Editor;

#define EDITOR_CONTENT_CAP 256
#define EDITOR_MINI_BUFFER_CONTENT_LIMIT 1000

void editor_goto_point(Editor *editor, size_t pos);

void editor_insert(Editor *editor, char *str);
void editor_delete_backward(Editor *editor);
void editor_delete_forward(Editor *editor);
int  editor_save(Editor* editor);
int  editor_read_file(Editor *editor, char *file_path);
void editor_determine_lines(Editor *editor);
void editor_recognize_arena(Editor *editor);

size_t editor_get_current_line_number(Pane *pane);

void editor_next_line(Editor *editor);
void editor_previous_line(Editor *editor);
void editor_char_backward(Editor *editor);
void editor_char_forward(Editor *editor);
void editor_word_forward(Editor *editor);
void editor_word_backward(Editor *editor);
void editor_delete_word_forward(Editor *editor);
void editor_mark_forward_word(Editor *editor);

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

void editor_user_search_forward(Editor *editor);
void editor_user_search_backward(Editor *editor);
void editor_user_extend_command(Editor *editor);
void editor_user_input_clear(Editor *editor);
void editor_user_input_insert(Editor *editor, char *text);
void editor_user_input_delete_backward(Editor *editor);
void editor_user_input_insert_from_clipboard(Editor *editor);

bool editor_user_search_next(Editor *editor, char *notification);

void editor_goto_line(Editor *editor, size_t line);
void editor_goto_line_forward(Editor *editor, size_t line);
void editor_goto_line_backward(Editor *editor, size_t line);

bool editor_is_editing_text(Editor *editor);
void editor_print_buffers_names(Editor *editor, char *notification);
void editor_kill_buffer(Editor *editor, size_t buf_index, char *notification);
void editor_switch_buffer(Editor *editor, size_t buf_index);

void editor_split_pane(Editor *editor);

void editor_wrap_region_in_parens(Editor *editor);

void editor_next_pane(Editor *editor);
void editor_close_pane(Editor *editor);
#endif
