#ifndef UTF8_H
#define UTF8_H

#include <stdio.h>

int utf8_size_char(char ch);
int utf8_size_char_backward(char *text, size_t from);

#endif
