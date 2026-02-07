#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stdlib.h>

#include "editor.h"
#include "render.h"
#include "themes.h"
#include "common.h"

#define FONT_SIZE       12
#define MESSAGE_TIMEOUT 5
#define TAB_SIZE        4
#define LEADING         0    /* space between raws */

#define NEWLINE         "\n"
#define SPACE           " "
#define TAB             "\t"

const enum LineNumberFormat DISPLAY_LINE_FROMAT = HIDE; //[ABSOLUTE, RELATIVE, HIDE]

static Smacs smacs = {0};

//TODO: Clean up whitespaces before saving
//TODO: M-& Emacs command
//TODO: Multicursor
//TODO: undo (revert deletion and changes)
//TODO: replace

void initial_hook(void);
bool ctrl_leader_mapping(SDL_Event *event, int *message_timeout);
bool alt_leader_mapping(SDL_Event *event);
bool search_mapping(SDL_Event *event, int *message_timeout);
bool extend_command_mapping(SDL_Event *event, int *message_timeout);
bool completion_command_mapping(SDL_Event *event);

bool mini_buffer_event_handle(SDL_Event *event);
bool completion_event_handle(SDL_Event *event);

int
smacs_launch(char *home_dir, char *ttf_path, char *file_path)
{
    int win_w, win_h, message_timeout, win_w_per_pane;
    register int i;

    if (!SDL_Init(SDL_INIT_VIDEO)) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (!TTF_Init()) {
        fprintf(stderr, "Could not initialize SDL TTF: %s\n", SDL_GetError());
        return 1;
    }

    smacs.font_size = FONT_SIZE;
    smacs.font = TTF_OpenFont(ttf_path, smacs.font_size);
    if (smacs.font == NULL) {
        fprintf(stderr, "Could not open ttf: %s\n", SDL_GetError());
        return 1;
    }

    if (!SDL_CreateWindowAndRenderer("smacs", 1000, 1200, SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED, &smacs.window, &smacs.renderer)) {
        fprintf(stderr, "Could not open SDL window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_StartTextInput(smacs.window);

    bool quit = false;
    message_timeout = 0;
    
	// alternatives: [themes_naysayer, themes_mindre, themes_acme, themes_jblow_nastalgia]
    themes_jblow_nastalgia(&smacs);

    smacs.line_number_format = DISPLAY_LINE_FROMAT;
    smacs.home_dir = home_dir;
    smacs.editor = (Editor) {0};

    smacs.editor.panes_len = 0;
    smacs.editor.panes[smacs.editor.panes_len] = (Pane) {0};
    smacs.editor.pane = &smacs.editor.panes[smacs.editor.panes_len];
    ++smacs.editor.panes_len;

    smacs.editor.buffer_list = (BufferList) {0};
    smacs.editor.completor = (Completor) {0};
    editor_read_file(&smacs.editor, file_path);

    SDL_GetWindowSize(smacs.window, &win_w, &win_h);
    smacs.editor.pane->arena = (Arena) {0, win_h / smacs.font_size};
    smacs.editor.pane->buffer->events_len = 0;
    smacs.notification = calloc(RENDER_NOTIFICATION_LEN, sizeof(char));
    smacs.leading = LEADING;
    smacs.tab_size = TAB_SIZE;

    initial_hook();

    SDL_Event event = {0};

    while (!quit) {
        SDL_WaitEvent(&event);

        if (event.type == SDL_EVENT_MOUSE_MOTION) continue;

        switch (event.type) {
        case SDL_EVENT_TEXT_INPUT: {
#ifdef OS_LINUX
            if ((event.key.mod & (SDL_KMOD_CTRL | SDL_KMOD_ALT))) continue;
#endif
            if (mini_buffer_event_handle(&event)) break;
            if (completion_event_handle(&event)) break;

            editor_insert(&smacs.editor, (char*)event.text.text);
        } break;
        case SDL_EVENT_KEY_DOWN: {
            if (search_mapping(&event, &message_timeout)) break;
            if (extend_command_mapping(&event, &message_timeout)) break;
            if (completion_command_mapping(&event)) break;
            if (ctrl_leader_mapping(&event, &message_timeout)) break;
            if (alt_leader_mapping(&event)) break;

            switch (event.key.key) {
            case SDLK_BACKSPACE:{
                editor_delete_backward(&smacs.editor);
            }break;
            case SDLK_RETURN:{
                editor_new_line(&smacs.editor);
            }break;
            case SDLK_TAB:{
                editor_insert(&smacs.editor, TAB);
            }break;
            case SDLK_F11:{
                if (SDL_GetWindowFlags(smacs.window) & SDL_WINDOW_FULLSCREEN) {
                    SDL_SetWindowFullscreen(smacs.window, 0);
                } else {
                    SDL_SetWindowFullscreen(smacs.window, SDL_WINDOW_FULLSCREEN);
                }
            }break;
            }
        } break;
        case SDL_EVENT_QUIT: {
            quit = true;
        } break;
        case SDL_EVENT_MOUSE_WHEEL: {
            editor_mwheel_scroll(&smacs.editor, event.wheel.y);
        } break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN: {
            //TODO(ivan): is not working well with utf8 chars and right pane
            long point = render_find_position_by_xy(&smacs, (int)event.button.x, (int)event.button.y);
            if (point > 0) {
                editor_goto_point(&smacs.editor, point);
                editor_char_backward(&smacs.editor);
            }
        }break;
        }

        SDL_GetWindowSize(smacs.window, &win_w, &win_h);
        TTF_GetStringSize(smacs.font, "|", 1, &smacs.char_w, &smacs.char_h);

        win_w_per_pane = win_w / smacs.editor.panes_len;

        for (i = 0; i < (int) smacs.editor.panes_len; ++i) {
            smacs.editor.panes[i].x = win_w_per_pane * i;
            smacs.editor.panes[i].w = win_w_per_pane;
            smacs.editor.panes[i].h = win_h;
            smacs.editor.panes[i].arena.show_lines = (win_h / (smacs.char_h + smacs.leading));
        }

        SDL_SetRenderDrawColor(smacs.renderer, smacs.bg.r, smacs.bg.g, smacs.bg.b, smacs.bg.a);
        SDL_RenderClear(smacs.renderer);

        render_update_glyph(&smacs);
        render_glyph_show(&smacs);

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

void
initial_hook(void)
{
    /*
    editor_split_pane(&smacs.editor);
    editor_next_pane(&smacs.editor);
    editor_read_file(&smacs.editor, "*scratch*");
    editor_insert(&smacs.editor, ";; Buffer for your notes\n");
    editor_next_pane(&smacs.editor);
    */
}

bool
ctrl_leader_mapping(SDL_Event *event, int *message_timeout)
{
    if ((event->key.mod & SDL_KMOD_CTRL) == 0) return false;

    switch (event->key.key) {
    case SDLK_B:
        editor_char_backward(&smacs.editor);
        break;
    case SDLK_F:
        editor_char_forward(&smacs.editor);
        break;
    case SDLK_P:
        editor_previous_line(&smacs.editor);
        break;
    case SDLK_N:
        editor_next_line(&smacs.editor);
        break;
    case SDLK_A:
        editor_move_begginning_of_line(&smacs.editor);
        break;
    case SDLK_E:
        editor_move_end_of_line(&smacs.editor);
        break;
    case SDLK_K:
        editor_kill_line(&smacs.editor);
        break;
    case SDLK_D:
        editor_delete_forward(&smacs.editor);
        break;
    case SDLK_L:
        editor_recenter_top_bottom(&smacs.editor);
        break;
    case SDLK_V:
        editor_scroll_up(&smacs.editor);
        break;
    case SDLK_SPACE:
        editor_set_mark(&smacs.editor);
        break;
    case SDLK_G:
        smacs.editor.state = NONE;
        editor_user_input_clear(&smacs.editor);
        break;
    case SDLK_Y:
        editor_paste(&smacs.editor);
        break;
    case SDLK_COMMA:
        editor_duplicate_line(&smacs.editor);
        break;
    case SDLK_S:
        editor_user_search_forward(&smacs.editor);
        break;
    case SDLK_R:
        editor_user_search_backward(&smacs.editor);
        break;
    case SDLK_W:
        editor_cut(&smacs.editor);
        break;
    case SDLK_EQUALS:
        TTF_SetFontSize(smacs.font, ++smacs.font_size);
        break;
    case SDLK_MINUS:
        TTF_SetFontSize(smacs.font, --smacs.font_size);
        break;
    case SDLK_X:
        editor_user_extend_command(&smacs.editor);
        break;
    case SDLK_Q:
        editor_next_pane(&smacs.editor);
        break;
    case SDLK_I:
        editor_buffer_switch(&smacs.editor);
        break;
    case SDLK_O:
        editor_find_file(&smacs.editor, true);
        break;
    case SDLK_C:
        if (editor_save(&smacs.editor) == 0) {
            sprintf(smacs.notification, "Saved");
            *message_timeout = MESSAGE_TIMEOUT;
        }
        break;
    case SDLK_SLASH:
        editor_undo(&smacs.editor);
        break;
    }

    return true;
}

bool
alt_leader_mapping(SDL_Event *event)
{
    if ((event->key.mod & SDL_KMOD_ALT) == 0) return false;

    if (event->key.mod & SDL_KMOD_SHIFT) {
        switch (event->key.key) {
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
        switch (event->key.key) {
        case SDLK_V:
            editor_scroll_down(&smacs.editor);
            break;
        case SDLK_W:
            editor_copy_to_clipboard(&smacs.editor);
            smacs.editor.state = NONE;
            break;
        case SDLK_N:
            editor_move_line_down(&smacs.editor);
            break;
        case SDLK_P:
            editor_move_line_up(&smacs.editor);
            break;
        case SDLK_F:
            editor_word_forward(&smacs.editor);
            break;
        case SDLK_B:
            editor_word_backward(&smacs.editor);
            break;
        case SDLK_D:
            editor_delete_word_forward(&smacs.editor);
            break;
        case SDLK_K:
            editor_close_pane(&smacs.editor);
            break;
        case SDLK_U:
            editor_upper(&smacs.editor);
            break;
        case SDLK_L:
            editor_lower(&smacs.editor);
            break;
        case SDLK_BACKSPACE:
            editor_delete_word_backward(&smacs.editor);
            break;
        }
    }

    return true;
}

bool
extend_command_mapping(SDL_Event *event, int *message_timeout)
{
    size_t data_len;
    char *data;

    if ((smacs.editor.state & EXTEND_COMMAND) == 0) return false;

    if (event->key.mod & SDL_KMOD_CTRL) {
        switch (event->key.key) {
        case SDLK_G:
            editor_user_input_clear(&smacs.editor);
            break;
        case SDLK_Y:
            editor_user_input_insert_from_clipboard(&smacs.editor);
            break;
        }
    }

    switch (event->key.key) {
    case SDLK_BACKSPACE:
        editor_user_input_delete_backward(&smacs.editor);
        break;
    case SDLK_RETURN:
        //TODO: need to simplify this shit

        data = smacs.editor.user_input.data;
        data_len = strlen(data);

        if (starts_with(data, "bk") && data_len > 2) {
            editor_kill_buffer(&smacs.editor, (size_t) atoi(&data[2]), smacs.notification);
            *message_timeout = MESSAGE_TIMEOUT;
        } else if (starts_with(data, "sp")) {
            if (smacs.editor.panes_len < PANES_MAX_SIZE) {
                editor_split_pane(&smacs.editor);
            } else {
                sprintf(smacs.notification, "Can not create more than %d panes", PANES_MAX_SIZE);
                *message_timeout = MESSAGE_TIMEOUT;
            }
        } else if (starts_with(data, "n") && data_len > 1) {
            editor_goto_line_forward(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, "p") && data_len > 1) {
            editor_goto_line_backward(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, ":") && data_len > 1) {
            editor_goto_line(&smacs.editor, (size_t) atoi(&data[1]));
        } else if (starts_with(data, "dlrel")) {
            smacs.line_number_format = RELATIVE;
        } else if (starts_with(data, "dlabs")) {
            smacs.line_number_format = ABSOLUTE;
        } else if (starts_with(data, "dlnon")) {
            smacs.line_number_format = HIDE;
        } else {
            //fprintf(stderr, "Unknown cmd %s\n", smacs.editor.user_input.data);
        }

        editor_user_input_clear(&smacs.editor);
        break;
    }

    return true;
}

bool
search_mapping(SDL_Event *event, int *message_timeout)
{
    if ((smacs.editor.state & SEARCH) == 0) return false;

    if (event->key.mod & SDL_KMOD_CTRL) {
        switch (event->key.key) {
        case SDLK_G:
            editor_user_input_clear(&smacs.editor);
            break;
        case SDLK_Y:
            editor_user_input_insert_from_clipboard(&smacs.editor);
            break;
        default:
            editor_user_input_clear(&smacs.editor);
            break;
        }
    }

    switch (event->key.key) {
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

bool
completion_command_mapping(SDL_Event *event)
{
    if ((smacs.editor.state & COMPLETION) == 0) return false;

    if (event->key.mod & SDL_KMOD_CTRL) {
        switch (event->key.key) {
        case SDLK_G:
            editor_user_input_clear(&smacs.editor);
            break;
        case SDLK_Y:
            editor_user_input_insert_from_clipboard(&smacs.editor);
            editor_completion_actualize(&smacs.editor);
            break;
        case SDLK_S:
            editor_completion_next_match(&smacs.editor);
            break;
        case SDLK_R:
            editor_completion_prev_match(&smacs.editor);
            break;
        }
    }

    switch (event->key.key) {
    case SDLK_BACKSPACE:
        editor_user_input_delete_backward(&smacs.editor);
        editor_completion_actualize(&smacs.editor);
        break;
    case SDLK_RETURN:
        if (editor_buffer_switch_complete(&smacs.editor)) break;
        if (editor_find_file_complete(&smacs.editor)) break;
        break;
    }

    return true;
}

bool
mini_buffer_event_handle(SDL_Event *event) {
    if (!(smacs.editor.state & (SEARCH | EXTEND_COMMAND))) return false;

    editor_user_input_insert(&smacs.editor, (char*)event->text.text);
    return true;
}

bool
completion_event_handle(SDL_Event *event) {
    if (!(smacs.editor.state & COMPLETION)) return false;

    editor_user_input_insert(&smacs.editor, (char*)event->text.text);
    editor_completion_actualize(&smacs.editor);
    return true;
}
