#ifndef UTF8_H
#define UTF8_H

#include <stdio.h>
#include <stdint.h>

uint8_t utf8_size_char(char ch);
uint8_t utf8_size_char_backward(char *text, size_t from);

#endif
