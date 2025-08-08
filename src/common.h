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

void *gb_append_(void *data, size_t *pcap, size_t size);

#define gb_append(gb, elem)                                                   \
    *((gb)->len == (gb)->cap                                                  \
      ? (gb)->data = gb_append_((gb)->data, &(gb)->cap, sizeof(*(gb)->data)), \
        (gb)->data + (gb)->len++                                              \
      : (gb)->data + (gb)->len++) = elem

/**
 * a starts_with b
 */
bool starts_with(char *a, char *b);
/**
 * cast utf8 char sequence to an interger value
 */
uint32_t utf8_chars_to_int(char *str, int len);

#endif
