#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"

void sb_append(StringBuilder *sb, char ch)
{
    if (sb->len >= sb->cap) {
        sb->cap = sb->cap == 0 ? SB_CAP_INIT : sb->cap * 2;
        sb->data = (char*) realloc(sb->data, sb->cap * sizeof(*sb->data));
        memset(&sb->data[sb->len], 0, sb->cap - sb->len);
    }
    sb->data[sb->len++] = ch;
}

void sb_append_many(StringBuilder *sb, char *str)
{
    for (size_t i = 0; i < strlen(str); ++i) {
        sb_append(sb, str[i]);
    }
}

void sb_clean(StringBuilder *sb)
{
    sb->len = 0;
    if (sb->cap) memset(&sb->data[sb->len], 0, sb->cap);
}

void sb_free(StringBuilder *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

bool starts_with(char *a, char *b)
{
    size_t len;

    len = strlen(b);
    if (a == NULL || b == NULL) return false;
    if (strlen(a) < len) return false;

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
