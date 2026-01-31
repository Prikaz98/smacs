#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <stdbool.h>
#include <assert.h>
#include <stdint.h>

#include "common.h"

void *gb_append_(void *data, size_t *pcap, size_t size)
{
    void *ptr;
    size_t ptr_cap;

    ptr_cap = *pcap;

    ptr_cap = ptr_cap == 0 ? 10 : ptr_cap * 2;
    ptr = realloc(data, ptr_cap * size);
    if (ptr == NULL) {
        fprintf(stderr, "No more free space\n");
        free(data);
        exit(EXIT_FAILURE);
    }

    *pcap = ptr_cap;
    return ptr;
}

void
sb_append(StringBuilder *sb, char ch)
{
    if (sb->len >= sb->cap) {
        sb->cap = sb->cap == 0 ? SB_CAP_INIT : sb->cap * 2;
        sb->data = (char*) realloc(sb->data, sb->cap * sizeof(*sb->data));
        memset(&sb->data[sb->len], 0, sb->cap - sb->len);
    }
    sb->data[sb->len++] = ch;
}

void
sb_append_many(StringBuilder *sb, char *str)
{
    size_t len = strlen(str);

    for (register size_t i = 0; i < len; ++i) {
        sb_append(sb, str[i]);
    }
}

void
sb_append_manyl(StringBuilder *sb, char *str, size_t len)
{
    for (register size_t i = 0; i < len; ++i) {
        sb_append(sb, str[i]);
    }
}

void
sb_clean(StringBuilder *sb)
{
    sb->len = 0;
    if (sb->cap) memset(&sb->data[sb->len], 0, sb->cap);
}

void
sb_free(StringBuilder *sb)
{
    free(sb->data);
    sb->data = NULL;
    sb->len = 0;
    sb->cap = 0;
}

bool
starts_with(char *a, char *b)
{
    size_t len;

    len = strlen(b);
    if (a == NULL || b == NULL) return false;
    if (strlen(a) < len) return false;

    return strncmp(a, b, len) == 0;
}

bool
contains_ignore_case(char *a, size_t a_len, char *b, size_t b_len)
{
    register size_t i;

    if (a == NULL || b == NULL) return false;
    if (a_len < b_len) return false;

    for (i = 0; i < a_len; ++i) {
        if (strncasecmp(&a[i], b, b_len) == 0) {
            return true;
        }
    }

    return false;
}

uint32_t utf8_chars_to_int(char *str, int len) {
    uint32_t result;
    assert(0 < len && len < 5 && "Invalid utf8 len");

    switch (len) {
    case 1:
        assert(strlen(str) >= 1 && "Invalid len; Expected 1");
        result = str[0];
        break;
    case 2:
        assert(strlen(str) >= 2 && "Invalid len; Expected 2");
        result = ((uint8_t) (str[0]) << 8) + ((uint8_t) str[1]);
        break;
    case 3:
        assert(strlen(str) >= 3 && "Invalid len; Expected 3");
        result = ((uint8_t) (str[0]) << 16) + ((uint8_t) (str[1]) << 8) + ((uint8_t) str[2]);
        break;
    case 4:
        assert(strlen(str) >= 4 && "Invalid len; Expected 4");
        result = ((uint8_t) (str[1]) << 24) + ((uint8_t) (str[1]) << 16) + ((uint8_t) (str[2]) << 8) + ((uint8_t) str[3]);
        break;
    }

    return result;
}

char *strdup(const char *str)
{
    size_t len = strlen(str);
    char *ds = calloc(len+1, sizeof(char));

    memcpy(ds, str, len);
    ds[len] = '\0';
    return ds;
}

#ifdef OS_LINUX
uint64_t rdtsc(void)
{
    unsigned int lo,hi;
    __asm__ __volatile__ ("rdtsc" : "=a" (lo), "=d" (hi));
    return ((uint64_t)hi << 32) | lo;
}
#endif
