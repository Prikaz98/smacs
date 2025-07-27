#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

bool starts_with(char *a, char *b)
{
    size_t len;

    len = strlen(b);

    return strncmp(a, b, len) == 0;
}

uint32_t utf8_chars_to_int(char *str, int len) {
    uint32_t result;
    assert(0 < len && len < 5);

    switch (len) {
    case 1:
        assert(strlen(str) >= 1);
        result = str[0];
        break;
    case 2:
        assert(strlen(str) >= 2);
        result = ((uint8_t) (str[0]) << 8) + ((uint8_t) str[1]);
        break;
    case 3:
        assert(strlen(str) >= 3);
        result = ((uint8_t) (str[0]) << 16) + ((uint8_t) (str[1]) << 8) + ((uint8_t) str[2]);
        break;
    case 4:
        assert(strlen(str) >= 4);
        result = ((uint8_t) (str[1]) << 24) + ((uint8_t) (str[1]) << 16) + ((uint8_t) (str[2]) << 8) + ((uint8_t) str[3]);
        break;
    }

    return result;
}
