#ifndef COMMON_H
#define COMMON_H

typedef struct {
    char *data;
    size_t len;
    size_t cap;
} StringBuilder;

#define SB_CAP_INIT 100

void sb_append(StringBuilder *sb, char ch);
void sb_append_many(StringBuilder *sb, char *str);
void sb_clean(StringBuilder *sb);
void sb_free(StringBuilder *sb);

/**
 * a starts_with b
 */
bool starts_with(char *a, char *b);
/**
 * cast utf8 char sequence to an interger value
 */
uint32_t utf8_chars_to_int(char *str, int len);

#endif
