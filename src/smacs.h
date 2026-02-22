#ifndef SMACS_H
#define SMACS_H

#include <stdbool.h>
#include "render.h"

#define FONT_SIZE       14
#define MESSAGE_TIMEOUT 5
#define TAB_SIZE        8
#define LEADING         1 /* space between raws */

#define NEWLINE "\n"
#define SPACE   " "
#define TAB     "\t"

int smacs_launch(char *home_dir, char *ttf_path, char *fallback_ttf_path, char *file_path);
bool ctrl_leader_mapping(Smacs *smacs, SDL_Event *event, int *message_timeout);
bool alt_leader_mapping(Smacs *smacs, SDL_Event *event);
bool search_mapping(Smacs *smacs, SDL_Event *event, int *message_timeout);
bool extend_command_mapping(Smacs *smacs, SDL_Event *event, int *message_timeout);
bool completion_command_mapping(Smacs *smacs, SDL_Event *event);

bool mini_buffer_event_handle(Smacs *smacs, SDL_Event *event);
bool completion_event_handle(Smacs *smacs, SDL_Event *event);

#endif
