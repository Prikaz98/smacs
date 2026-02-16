#include <stdio.h>
#include <stdbool.h>
#include "tokenize.h"
#include "common.h"

int tokenize(Tokens *tokens, char *data, size_t data_len)
{
    if (data == 0) return 0;

    gb_clean(tokens);

    while (tokens->len < data_len) {
        if (data[tokens->len] == '"') {
            gb_append(tokens, TOKEN_STRING);

            while (tokens->len < data_len) {
                if (data[tokens->len] == '\\') {
                    gb_append(tokens, TOKEN_STRING);
                    gb_append(tokens, TOKEN_STRING);
                } else if (data[tokens->len] == '"') {
                    gb_append(tokens, TOKEN_STRING);
                    break;
                } else {
                    gb_append(tokens, TOKEN_STRING);
                }
            }
        } else if (strncmp(&data[tokens->len], "//", 2) == 0) {
            gb_append(tokens, TOKEN_COMMENT);
            gb_append(tokens, TOKEN_COMMENT);

            while (tokens->len < data_len) {
                if (data[tokens->len] == '\n') {
                    break;
                } else {
                    gb_append(tokens, TOKEN_COMMENT);
                }
            }
        } else {
            gb_append(tokens, TOKEN_TEXT);
        }
    }

    return 1;
}
