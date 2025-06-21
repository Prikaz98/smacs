#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <strings.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#define SCREEN_WIDTH     800
#define SCREEN_HEIGHT    800
#define BACKGROUND_COLOR 0x161616
#define TEXT_COLOR       (SDL_Color) {255, 255, 255, 0}
#define TTF_PATH         "iosevka-regular.ttf"
#define FONT_SIZE        24
#define FPS              60

char BUFFER[1024];
const SDL_Rect BACKGROUND = (SDL_Rect) {0, 0, SCREEN_WIDTH, SCREEN_HEIGHT};

void draw_text(SDL_Renderer *renderer, int x, int y, size_t len, TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect);

int append(char arr[], char *text, size_t pos)
{
    size_t text_size = strlen(text);
    memcpy(&arr[pos], text, text_size);
    return pos + text_size;
}

int main()
{
    SDL_Rect text_rect;
    SDL_Texture *text_texture;

    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Could not initialize SDL: %s\n", SDL_GetError());
        return 1;
    }

    if (TTF_Init() < 0) {
        fprintf(stderr, "Could not initialize SDL TTF: %s\n", SDL_GetError());
        return 1;
    }

    TTF_Font *font = TTF_OpenFont(TTF_PATH, FONT_SIZE);
    if (font == NULL) {
        fprintf(stderr, "Could not open ttf: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("ceditor", 0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_RESIZABLE);
    if (window == NULL) {
        fprintf(stderr, "Could not open SDL window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, 0, 0);
    if (renderer == NULL) {
        fprintf(stderr, "Could not initilize render  of SDL window: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Surface* surface = SDL_GetWindowSurface(window);
    if (surface == NULL) {
        fprintf(stderr, "Could not get surface of SDL window: %s\n", SDL_GetError());
        return 1;
    }

    size_t pos = 0;
    bool quit = false;

    while (!quit) {
      SDL_Event event = {0};
      const Uint32 start = SDL_GetTicks();

      while (SDL_PollEvent(&event)) {
          draw_text(renderer, 0, 0, pos, font, &text_texture, &text_rect);

          SDL_SetRenderDrawColor(renderer, 22, 22, 22, 0);
          SDL_RenderClear(renderer);
          SDL_RenderCopy(renderer, text_texture, NULL, &text_rect);
          SDL_RenderPresent(renderer);

          switch (event.type) {
          case SDL_QUIT: {
              quit = true;
              break;
          }
          case SDL_TEXTINPUT: {
              pos = append(BUFFER, event.text.text, pos);
              break;
          }
          }

      }

      const Uint32 duration = SDL_GetTicks() - start;
      const Uint32 delta_time_ms = 1000 / FPS;
      if (duration < delta_time_ms) {
          SDL_Delay(delta_time_ms - duration);
      }
    }

    TTF_Quit();

    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}

void draw_text(SDL_Renderer *renderer, int x, int y, size_t len, TTF_Font *font, SDL_Texture **texture, SDL_Rect *rect)
{
    if (len > 0) {
        int text_width, text_height;

        SDL_Surface *surface = TTF_RenderText_Solid(font, BUFFER, TEXT_COLOR);
        *texture = SDL_CreateTextureFromSurface(renderer, surface);

        text_width = surface->w;
        text_height = surface->h;

        SDL_FreeSurface(surface);
        rect->x = x;
        rect->y = y;
        rect->w = text_width;
        rect->h = text_height;
    }
}
