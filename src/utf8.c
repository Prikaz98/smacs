#include "utf8.h"
#include <sys/param.h>

uint8_t utf8_size_char(char ch)
{
    if ((ch & 0x80) == 0) return 1;
    if ((ch & 0xE0) == 0xC0) return 2;
    if ((ch & 0xF0) == 0xE0) return 3;
    if ((ch & 0xF8) == 0xF0) return 4;

    return 1;
}

uint8_t utf8_size_char_backward(char *text, size_t from)
{
    register size_t i;
    size_t until;

    until = MIN(from - 4, 0);
    for (i = from; i >= until; --i) {
        if ((text[i] & 0xC0) == 0x80) {
            continue;
        }

        return utf8_size_char(text[i]);
    }

    return 1;
}
