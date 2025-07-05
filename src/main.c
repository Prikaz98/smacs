#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <assert.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "editor.h"
#include "render.h"

#define SCREEN_WIDTH    600
#define SCREEN_HEIGHT   600
#define FONT_SIZE       17
//#define TTF_PATH        "fonts/AcPlus_IBM_VGA_8x16.ttf"
#define TTF_PATH        "fonts/iosevka-regular.ttf"
#define FPS             60
#define MESSAGE_TIMEOUT 100
#define TAB_SIZE        4
#define NEWLINE         "\n"
#define SPACE           " "

const unsigned int BG_COLOR = 0xF5F5F5;
const unsigned int FG_COLOR = 0x2E3331;
const unsigned int RG_COLOR = 0xCFD8DC;

static Smacs smacs = {0};

//TODO: write function copy/paste/cut
//TODO: forward-word/backward-word
//TODO: search
//TODO: show line numbers
//TODO: undo/redo

int main(int argc, char *argv[])
{
    char *file_path;
    size_t i;
    char message[256] = {0};
    int win_h, message_timeout, font_y;
    Uint32 duration, delta_time_ms, start;

    if (argc != 2) {
        fprintf(stderr, "Usage %s <file_path>\n", argv[0]);
        return 1;
    }

    file_path = argv[1];

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL TTF: %s\n", SDL_GetError());
        return 1;
    }

    smacs.font = TTF_OpenFont(TTF_PATH, FONT_SIZE);
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

    smacs.bg = (SDL_Color) {BG_COLOR >> 16, BG_COLOR >> 8 & 0xFF, BG_COLOR & 0xFFFF, 0};
    smacs.fg = (SDL_Color) {FG_COLOR >> 16, FG_COLOR >> 8 & 0xFF, FG_COLOR & 0xFFFF, 0};
    smacs.rg = (SDL_Color) {RG_COLOR >> 16, RG_COLOR >> 8 & 0xFF, RG_COLOR & 0xFFFF, 0};

    smacs.editor = (Editor) {0};
    editor_read_file(&smacs.editor, file_path);
    smacs.editor.buffer.arena = (Arena) {0, SCREEN_HEIGHT / FONT_SIZE};

    SDL_Event event = {0};

    while (!quit) {
      start = SDL_GetTicks();

      while (SDL_PollEvent(&event)) {
          switch (event.type) {
          case SDL_QUIT: {
              quit = true;
              break;
          }
          case SDL_TEXTINPUT: {
              if (!(event.key.keysym.mod & KMOD_CTRL) && !(event.key.keysym.mod & KMOD_ALT)) {
                  editor_insert(&smacs.editor, event.text.text);
              }
              break;
          }
          case SDL_MOUSEWHEEL: {
              editor_mwheel_scroll(&smacs.editor, event.wheel.y);
              break;
          }
          case SDL_KEYDOWN: {
              //main mapping
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
                      editor_recenter(&smacs.editor);
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
                      break;
                  }
                  case SDLK_y: {
                      editor_paste(&smacs.editor);
                      break;
                  }
                  }
              }

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

              switch (event.key.keysym.sym) {
              case SDLK_BACKSPACE: {
                  editor_delete_backward(&smacs.editor);
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
                  if (editor_save(&smacs.editor.buffer, file_path) == 0) {
                      sprintf(message, "Wrote %s", file_path);
                      message_timeout = MESSAGE_TIMEOUT;
                  }
                  break;
              }
              }
              break;
          }
          }
      }

      SDL_SetRenderDrawColor(smacs.renderer, smacs.bg.r, smacs.bg.g, smacs.bg.b, smacs.bg.a);
      SDL_RenderClear(smacs.renderer);

      SDL_GetWindowSize(smacs.window, NULL, &win_h);
      TTF_SizeText(smacs.font, "h", NULL, &font_y);

      smacs.editor.buffer.arena.show_lines = (win_h / font_y) + 1;
      //TODO: Draw mode line

      if (message_timeout > 0) {
          //TODO: Find more better place to draw message
          render_draw_text(&smacs, 0, win_h - font_y, &message[0]);
          message_timeout--;
      }

      render_draw_smacs(&smacs);

      SDL_RenderPresent(smacs.renderer);

      duration = SDL_GetTicks() - start;
      delta_time_ms = 1000 / FPS;
      if (duration < delta_time_ms) {
          SDL_Delay(delta_time_ms - duration);
      }
    }

    editor_destroy(&smacs.editor);

    SDL_DestroyRenderer(smacs.renderer);
    SDL_DestroyWindow(smacs.window);

    TTF_CloseFont(smacs.font);
    TTF_Quit();

    SDL_Quit();

    return 0;
}
