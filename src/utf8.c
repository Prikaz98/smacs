#include "utf8.h"

uint8_t utf8_size_char(char ch)
{
    if ((ch & 0x80) == 0) {
        return 1;
    }

    if ((ch & 0xE0) == 0xC0) {
        return 2;
    }

    if ((ch & 0xF0) == 0xE0) {
        return 3;
    }

    if ((ch & 0xF8) == 0xF0) {
        return 4;
    }

    return 1;
}

uint8_t utf8_size_char_backward(char *text, size_t from)
{
    long i;

    for (i = from; i >= 0; --i) {
        if ((text[i] & 0xC0) == 0x80) {
            continue;
        }

        return utf8_size_char(text[i]);
    }

    return 1;
}
