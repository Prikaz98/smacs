#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "editor.h"
#include "render.h"
#include "themes.h"
#include "common.h"

#define FONT_SIZE       17
#define MESSAGE_TIMEOUT 5
#define TAB_SIZE        4
#define LEADING         2    /* space between raws */

#define NEWLINE         "\n"
#define SPACE           " "
#define TAB             "\t"

const enum LineNumberFormat DISPLAY_LINE_FROMAT = RELATIVE; //[ABSOLUTE, RELATIVE]

static Smacs smacs = {0};

//TODO: Clean up whitespaces before saving
//TODO: M-& emacs command
//TODO: Multicursor
//TODO: undo/redo

bool ctrl_leader_mapping(SDL_Event event);
bool alt_leader_mapping(SDL_Event event);
bool search_mapping(SDL_Event event, int *message_timeout);
bool extend_command_mapping(SDL_Event event, int *message_timeout);

int smacs_launch(char *ttf_path, char *file_path)
{
    int win_w, win_h, message_timeout, font_y, i, win_w_per_pane;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL TTF: %s\n", SDL_GetError());
        return 1;
    }

    smacs.font_size = FONT_SIZE;
    smacs.font = TTF_OpenFont(ttf_path, smacs.font_size);
    if (smacs.font == NULL) {
        fprintf(stderr, "Could not open ttf: %s\n", SDL_GetError());
        return 1;
    }

    smacs.window = SDL_CreateWindow("smacs", 0, 0, 0, 0, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);
    if (smacs.window == NULL) {
        fprintf(stderr, "Could not open SDL window: %s\n", SDL_GetError());
        return 1;
    }

    smacs.renderer = SDL_CreateRenderer(smacs.window, 0, 0);
    if (smacs.renderer == NULL) {
        fprintf(stderr, "Could not initilize render  of SDL window: %s\n", SDL_GetError());
        return 1;
    }

    bool quit = false;
    message_timeout = 0;

    themes_naysayer(&smacs); // alternatives: [themes_naysayer, themes_mindre]

    smacs.line_number_format = DISPLAY_LINE_FROMAT;
    smacs.editor = (Editor) {0};

    smacs.editor.panes_len = 0;
    smacs.editor.panes[smacs.editor.panes_len] = (Pane) {0};
    smacs.editor.pane = &smacs.editor.panes[smacs.editor.panes_len];
    ++smacs.editor.panes_len;

    smacs.editor.buffer_list = (Buffer_List) {0};
    editor_read_file(&smacs.editor, file_path);
    SDL_GetWindowSize(smacs.window, &win_w, &win_h);
    smacs.editor.pane->arena = (Arena) {0, win_h / smacs.font_size};
    smacs.notification = calloc(RENDER_NOTIFICATION_LEN, sizeof(char));
    smacs.leading = LEADING;
    smacs.tab_size = TAB_SIZE;

    SDL_Event event = {0};

    while (!quit) {
        SDL_WaitEvent(&event);

        if (event.type == SDL_MOUSEMOTION) continue;

        switch (event.type) {
        case SDL_QUIT:
            quit = true;
            break;
        case SDL_MOUSEWHEEL:
            editor_mwheel_scroll(&smacs.editor, event.wheel.y);
            break;
        case SDL_KEYDOWN:
            if (search_mapping(event, &message_timeout)) break;
            if (extend_command_mapping(event, &message_timeout)) break;
            if (ctrl_leader_mapping(event)) break;
            if (alt_leader_mapping(event)) break;

            switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE:
                if (smacs.editor.state & SEARCH) {
                    editor_user_input_delete_backward(&smacs.editor);
                } else {
                    editor_delete_backward(&smacs.editor);
                }
                break;
            case SDLK_RETURN:
                editor_insert(&smacs.editor, NEWLINE);
                break;
            case SDLK_TAB:
                editor_insert(&smacs.editor, TAB);
                break;
            }
            break;
        case SDL_TEXTINPUT:
            if (event.key.keysym.mod & (KMOD_CTRL | KMOD_ALT)) break;

            if (smacs.editor.state & (SEARCH | EXTEND_COMMAND)) {
                editor_user_input_insert(&smacs.editor, event.text.text);
            } else {
                editor_insert(&smacs.editor, event.text.text);
            }
            break;
        }

        SDL_SetRenderDrawColor(smacs.renderer, smacs.bg.r, smacs.bg.g, smacs.bg.b, smacs.bg.a);
        SDL_RenderClear(smacs.renderer);

        SDL_GetWindowSize(smacs.window, &win_w, &win_h);
        TTF_SizeUTF8(smacs.font, "|", NULL, &font_y);

        win_w_per_pane = win_w / smacs.editor.panes_len;

        for (i = 0; i < (int) smacs.editor.panes_len; ++i) {
            smacs.editor.panes[i].x = win_w_per_pane * i;
            smacs.editor.panes[i].w = win_w_per_pane * i + win_w_per_pane;
            smacs.editor.panes[i].h = win_h;
            smacs.editor.panes[i].arena.show_lines = (win_h / font_y);
        }

        render_draw_smacs(&smacs);
        SDL_RenderPresent(smacs.renderer);

        if (message_timeout > 0) {
            message_timeout--;
        } else {
            memset(&smacs.notification[0], 0, RENDER_NOTIFICATION_LEN);
        }
    }

    render_destroy_smacs(&smacs);

    TTF_Quit();
    SDL_Quit();

    return 0;
}

bool ctrl_leader_mapping(SDL_Event event)
{
    if ((event.key.keysym.mod & KMOD_CTRL) == 0) return false;

    switch (event.key.keysym.sym) {
    case SDLK_b:
        editor_char_backward(&smacs.editor);
        break;
    case SDLK_f:
        editor_char_forward(&smacs.editor);
        break;
    case SDLK_p:
        editor_previous_line(&smacs.editor);
        break;
    case SDLK_n:
        editor_next_line(&smacs.editor);
        break;
    case SDLK_a:
        editor_move_begginning_of_line(&smacs.editor);
        break;
    case SDLK_e:
        editor_move_end_of_line(&smacs.editor);
        break;
    case SDLK_k:
        editor_kill_line(&smacs.editor);
        break;
    case SDLK_d:
        editor_delete_forward(&smacs.editor);
        break;
    case SDLK_l:
        editor_recenter_top_bottom(&smacs.editor);
        break;
    case SDLK_v:
        editor_scroll_up(&smacs.editor);
        break;
    case SDLK_SPACE:
        editor_set_mark(&smacs.editor);
        break;
    case SDLK_g:
        smacs.editor.state = NONE;
        editor_user_input_clear(&smacs.editor);
        break;
    case SDLK_y:
        editor_paste(&smacs.editor);
        break;
    case SDLK_COMMA:
        editor_duplicate_line(&smacs.editor);
        break;
    case SDLK_s:
        editor_user_search_forward(&smacs.editor);
        break;
    case SDLK_r:
        editor_user_search_backward(&smacs.editor);
        break;
    case SDLK_w:
        editor_cut(&smacs.editor);
        break;
    case SDLK_EQUALS:
        TTF_SetFontSize(smacs.font, ++smacs.font_size);
        break;
    case SDLK_MINUS:
        TTF_SetFontSize(smacs.font, --smacs.font_size);
        break;
    case SDLK_x:
        editor_user_extend_command(&smacs.editor);
        break;
    }

    return true;
}

bool alt_leader_mapping(SDL_Event event)
{
    if ((event.key.keysym.mod & KMOD_ALT) == 0) return false;

    if (event.key.keysym.mod & KMOD_SHIFT) {
        switch (event.key.keysym.sym) {
        case SDLK_2: // @
            editor_mark_forward_word(&smacs.editor);
            break;
        case SDLK_9: // (
            editor_wrap_region_in_parens(&smacs.editor);
            break;
        case SDLK_PERIOD:
            editor_end_of_buffer(&smacs.editor);
            break;
        case SDLK_COMMA:
            editor_beginning_of_buffer(&smacs.editor);
            break;
        }
    } else {
        switch (event.key.keysym.sym) {
        case SDLK_v:
            editor_scroll_down(&smacs.editor);
            break;
        case SDLK_w:
            editor_copy_to_clipboard(&smacs.editor);
            smacs.editor.state = NONE;
            break;
        case SDLK_n:
            editor_move_line_down(&smacs.editor);
            break;
        case SDLK_p:
            editor_move_line_up(&smacs.editor);
            break;
        case SDLK_f:
            editor_word_forward(&smacs.editor);
            break;
        case SDLK_b:
            editor_word_backward(&smacs.editor);
            break;
        case SDLK_d:
            editor_delete_word_forward(&smacs.editor);
            break;
        }
    }

    return true;
}

bool extend_command_mapping(SDL_Event event, int *message_timeout)
{
    size_t i, data_len;
    char *data;

    if ((smacs.editor.state & EXTEND_COMMAND) == 0) return false;

    if (event.key.keysym.mod & KMOD_CTRL) {
        switch (event.key.keysym.sym) {
        case SDLK_g:
            editor_user_input_clear(&smacs.editor);
            break;
        case SDLK_y:
            editor_user_input_insert_from_clipboard(&smacs.editor);
            break;
        }
    }

    switch (event.key.keysym.sym) {
    case SDLK_BACKSPACE:
        editor_user_input_delete_backward(&smacs.editor);
        break;
    case SDLK_RETURN:
        //TODO: need to simplify this shit

        data = smacs.editor.user_input.data;
        data_len = strlen(data);

        if (starts_with(data, "ff") && data_len > 4) {
            editor_read_file(&smacs.editor, &data[3]);
        } else if (starts_with(data, "bl")) {
            editor_print_buffers_names(&smacs.editor, smacs.notification);
            *message_timeout = MESSAGE_TIMEOUT;
        } else if (starts_with(data, "bk") && data_len > 2) {
            editor_kill_buffer(&smacs.editor, (size_t) atoi(&data[2]), smacs.notification);
            *message_timeout = MESSAGE_TIMEOUT;
        } else if (starts_with(data, "pn")) {
            for (i = 0; i < smacs.editor.panes_len; ++i) {
                if (&smacs.editor.panes[i] == smacs.editor.pane) {
                    smacs.editor.pane = &smacs.editor.panes[(i+1) >= smacs.editor.panes_len ? 0 : (i+1)];
                    break;
                }
            }
        } else if (starts_with(data, "pk")) {
            if (smacs.editor.panes_len == 1) {
                sprintf(smacs.notification, "Can not kill last pane");
                *message_timeout = MESSAGE_TIMEOUT;
            } else {
                --smacs.editor.panes_len;
                smacs.editor.pane = &smacs.editor.panes[0];
            }

        } else if (starts_with(data, "sp")) {
            if (smacs.editor.panes_len < PANES_MAX_SIZE) {
                editor_split_pane(&smacs.editor);
            } else {
                sprintf(smacs.notification, "Can not create more than %d panes", PANES_MAX_SIZE);
                *message_timeout = MESSAGE_TIMEOUT;
            }
        } else if (starts_with(data, "b") && data_len > 1) {
            editor_switch_buffer(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, "n") && data_len > 1) {
            editor_goto_line_forward(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, "p") && data_len > 1) {
            editor_goto_line_backward(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, ":") && data_len > 1) {
            editor_goto_line(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (strcmp(data, "s") == 0) {
            if (editor_save(&smacs.editor) == 0) {
                sprintf(smacs.notification, "Saved");
                *message_timeout = MESSAGE_TIMEOUT;
            }
        } else {
            fprintf(stderr, "Unknown cmd %s\n", smacs.editor.user_input.data);
        }

        editor_user_input_clear(&smacs.editor);
        break;
    }

    return true;
}

bool search_mapping(SDL_Event event, int *message_timeout)
{
    if ((smacs.editor.state & SEARCH) == 0) return false;

    if (event.key.keysym.mod & KMOD_CTRL) {
        switch (event.key.keysym.sym) {
        case SDLK_g:
            editor_user_input_clear(&smacs.editor);
            break;
        case SDLK_y:
            editor_user_input_insert_from_clipboard(&smacs.editor);
            break;
        }
    }

    switch (event.key.keysym.sym) {
    case SDLK_BACKSPACE:
        editor_user_input_delete_backward(&smacs.editor);
        break;
    case SDLK_RETURN:
        if (!editor_user_search_next(&smacs.editor, smacs.notification)) {
            *message_timeout = MESSAGE_TIMEOUT;
        }
        break;
    }

    return true;
}
