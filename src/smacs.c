#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "editor.h"
#include "render.h"

#define SCREEN_WIDTH    1000
#define SCREEN_HEIGHT   1000
#define FONT_SIZE       17
#define MESSAGE_TIMEOUT 100
#define TAB_SIZE        4
#define NEWLINE         "\n"
#define SPACE           " "

/* mindre-theme */
//const unsigned int BG_COLOR    = 0xF5F5F5;
//const unsigned int FG_COLOR    = 0x2E3331;
//const unsigned int RG_COLOR    = 0xCFD8DC;

/* naysayer-theme */
const unsigned int BG_COLOR    = 0x062329;
const unsigned int FG_COLOR    = 0xD1B897;
const unsigned int RG_COLOR    = 0x0000FF;

const enum LineNumberFormat DISPLAY_LINE_FROMAT = RELATIVE;
//const enum LineNumberFormat DISPLAY_LINE_FROMAT = ABSOLUTE;

static Smacs smacs = {0};

//TODO: write function cut
//TODO: forward-word/backward-word
//TODO: show line numbers
//TODO: undo/redo

void ctrl_leader_mapping(SDL_Event event);
void alt_leader_mapping(SDL_Event event);
bool search_mapping(SDL_Event event, int *message_timeout);
bool extend_command_mapping(SDL_Event event);

int smacs_launch(char *ttf_path, char *file_path)
{
    size_t i;
    int win_h, message_timeout, font_y;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL TTF: %s\n", SDL_GetError());
        return 1;
    }

    smacs.font = TTF_OpenFont(ttf_path, FONT_SIZE);
    if (smacs.font == NULL) {
        fprintf(stderr, "Could not open ttf: %s\n", SDL_GetError());
        return 1;
    }

    smacs.window = SDL_CreateWindow("smacs", 300, 100, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
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

    smacs.bg = (SDL_Color) {BG_COLOR >> 16, BG_COLOR >> 8 & 0xFF, (Uint8) BG_COLOR & 0xFFFF, 0};
    smacs.fg = (SDL_Color) {FG_COLOR >> 16, FG_COLOR >> 8 & 0xFF, (Uint8) FG_COLOR & 0xFFFF, 0};
    smacs.rg = (SDL_Color) {RG_COLOR >> 16, RG_COLOR >> 8 & 0xFF, (Uint8) RG_COLOR & 0xFFFF, 0};
    smacs.line_number_format = DISPLAY_LINE_FROMAT;
    smacs.editor = (Editor) {0};

    editor_read_file(&smacs.editor, file_path);
    smacs.editor.buffer.arena = (Arena) {0, SCREEN_HEIGHT / FONT_SIZE};
    smacs.notification = calloc(RENDER_NOTIFICATION_LEN, sizeof(char));

    SDL_Event event = {0};

    while (!quit) {
        SDL_WaitEvent(&event);

        if (event.type == SDL_MOUSEMOTION) {
            continue;
        }

        switch (event.type) {
        case SDL_QUIT: {
            quit = true;
            break;
        }
        case SDL_TEXTINPUT: {
            if (!(event.key.keysym.mod & (KMOD_CTRL | KMOD_ALT))) {
                if (smacs.editor.searching || smacs.editor.extend_command) {
                    editor_user_input_insert(&smacs.editor, event.text.text);
                } else {
                    editor_insert(&smacs.editor, event.text.text);
                }
            }
            break;
        }
        case SDL_MOUSEWHEEL: {
            editor_mwheel_scroll(&smacs.editor, event.wheel.y);
            break;
        }
        case SDL_KEYDOWN: {
            if (search_mapping(event, &message_timeout)) break;
            if (extend_command_mapping(event)) break;

            ctrl_leader_mapping(event);
            alt_leader_mapping(event);

            switch (event.key.keysym.sym) {
            case SDLK_BACKSPACE: {
                if (smacs.editor.searching) {
                    editor_user_input_delete_backward(&smacs.editor);
                } else {
                    editor_delete_backward(&smacs.editor);
                }
                break;
            }
            case SDLK_RETURN: {
                editor_insert(&smacs.editor, NEWLINE);
                break;
            }
            case SDLK_TAB: {
                for (i = 0; i < TAB_SIZE; i++) editor_insert(&smacs.editor, SPACE);
                break;
            }
            case SDLK_F2: {
                if (editor_save(&smacs.editor.buffer) == 0) {
                    sprintf(smacs.notification, "Saved");
                    message_timeout = MESSAGE_TIMEOUT;
                }
                break;
            }
            }
            break;
        }
        }

        SDL_SetRenderDrawColor(smacs.renderer, smacs.bg.r, smacs.bg.g, smacs.bg.b, smacs.bg.a);
        SDL_RenderClear(smacs.renderer);

        SDL_GetWindowSize(smacs.window, NULL, &win_h);
        TTF_SizeUTF8(smacs.font, "|", NULL, &font_y);

        smacs.editor.buffer.arena.show_lines = (win_h / font_y) + 1;
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

void ctrl_leader_mapping(SDL_Event event)
{
    if (event.key.keysym.mod & KMOD_CTRL) {
        switch (event.key.keysym.sym) {
        case SDLK_b: {
            editor_char_backward(&smacs.editor);
            break;
        }
        case SDLK_f: {
            editor_char_forward(&smacs.editor);
            break;
        }
        case SDLK_p: {
            editor_previous_line(&smacs.editor);
            break;
        }
        case SDLK_n: {
            editor_next_line(&smacs.editor);
            break;
        }
        case SDLK_a: {
            editor_move_begginning_of_line(&smacs.editor);
            break;
        }
        case SDLK_e: {
            editor_move_end_of_line(&smacs.editor);
            break;
        }
        case SDLK_k: {
            editor_kill_line(&smacs.editor);
            break;
        }
        case SDLK_d: {
            editor_delete_forward(&smacs.editor);
            break;
        }
        case SDLK_l: {
            editor_recenter_top_bottom(&smacs.editor);
            break;
        }
        case SDLK_v: {
            editor_scroll_up(&smacs.editor);
            break;
        }
        case SDLK_SPACE: {
            editor_set_mark(&smacs.editor);
            break;
        }
        case SDLK_g: {
            smacs.editor.selection = false;
            smacs.editor.searching = false;
            break;
        }
        case SDLK_y: {
            editor_paste(&smacs.editor);
            break;
        }
        case SDLK_COMMA: {
            editor_duplicate_line(&smacs.editor);
            break;
        }
        case SDLK_s: {
            editor_user_search_forward(&smacs.editor);
            break;
        }
        case SDLK_r: {
            editor_user_search_backward(&smacs.editor);
            break;
        }
        case SDLK_w: {
            editor_cut(&smacs.editor);
            break;
        }
        case SDLK_x: {
            editor_user_extend_command(&smacs.editor);
            break;
        }
        }
    }
}

void alt_leader_mapping(SDL_Event event)
{
    if (event.key.keysym.mod & KMOD_ALT) {
        switch (event.key.keysym.sym) {
        case SDLK_v: {
            editor_scroll_down(&smacs.editor);
            break;
        }
        case SDLK_PERIOD: {
            editor_end_of_buffer(&smacs.editor);
            break;
        }
        case SDLK_COMMA: {
            editor_beginning_of_buffer(&smacs.editor);
            break;
        }
        case SDLK_w: {
            editor_copy_to_clipboard(&smacs.editor);
            smacs.editor.selection = false;
            break;
        }
        }
    }
}

bool extend_command_mapping(SDL_Event event)
{
    if (!smacs.editor.extend_command) return false;

    if (event.key.keysym.mod & KMOD_CTRL) {
        switch (event.key.keysym.sym) {
        case SDLK_g: {
            editor_user_input_clear(&smacs.editor);
            break;
        }
        }
    }

    switch (event.key.keysym.sym) {
    case SDLK_BACKSPACE: {
        editor_user_input_delete_backward(&smacs.editor);
        break;
    }
    case SDLK_RETURN: {
        //TODO: write start_with function
        if (strncmp(smacs.editor.user_input.data, "gl", 2) == 0) {
            editor_goto_line(&smacs.editor, (size_t) atoi(&smacs.editor.user_input.data[2]));
        }
        editor_user_input_clear(&smacs.editor);
        break;
    }
    }

    return true;
}

bool search_mapping(SDL_Event event, int *message_timeout)
{
    if (!smacs.editor.searching) return false;

    if (event.key.keysym.mod & KMOD_CTRL) {
        switch (event.key.keysym.sym) {
        case SDLK_g: {
            editor_user_input_clear(&smacs.editor);
            break;
        }
        }
    }

    switch (event.key.keysym.sym) {
    case SDLK_BACKSPACE: {
        editor_user_input_delete_backward(&smacs.editor);
        break;
    }
    case SDLK_RETURN: {
        if (!editor_user_search_next(&smacs.editor, smacs.notification)) {
            *message_timeout = MESSAGE_TIMEOUT;
        }
        break;
    }
    }

    return true;
}
