#ifndef COMMON_H
#define COMMON_H

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StringBuilder;

#define SB_CAP_INIT 100

#define sb_append(sb, ch)                                               \
    do {                                                                \
        if (sb->len >= sb->cap) {                                       \
            sb->cap = sb->cap == 0 ? SB_CAP_INIT : sb->cap * 2;         \
            sb->data = realloc(sb->data, sb->cap * sizeof(*sb->data));  \
            memset(&sb->data[sb->len], 0, sb->cap - sb->len);           \
        }                                                               \
        sb->data[sb->len++] = ch;                                       \
    } while(0)

#define sb_append_many(sb, str)                    \
    do {                                           \
        for (size_t z = 0; z < strlen(str); ++z) { \
            sb_append(sb, str[z]);                 \
        }                                          \
    } while(0)

#define sb_clean(sb)                            \
    do {                                        \
        sb->len = 0;                            \
        memset(&sb->data[sb->len], 0, sb->cap); \
    } while(0)

#define sb_free(sb)                             \
    do {                                        \
        free(sb->data);                         \
        sb->len = 0;                            \
        sb->cap = 0;                            \
    } while(0)


/**
 * a starts_with b
 */
bool starts_with(char *a, char *b);

#endif
