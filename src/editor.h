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
    Arena arena;
    char *file_path;
} Buffer;

typedef struct {
    size_t position;
    Buffer buffer;
    size_t mark;

    bool selection;
    bool searching;
    bool reverse_searching;
    bool extend_command;
    StringBuilder user_input;
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

void editor_user_search_forward(Editor *editor);
void editor_user_search_backward(Editor *editor);

void editor_user_extend_command(Editor *editor);

void editor_user_input_clear(Editor *editor);
void editor_user_input_insert(Editor *editor, char *text);
void editor_user_input_delete_backward(Editor *editor);
bool editor_user_search_next(Editor *editor, char *notification);

void editor_goto_line(Editor *editor, size_t line);

#endif
