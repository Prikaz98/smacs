#include <stdio.h>
#include <stdbool.h>
#include "tokenize.h"
#include "common.h"

bool is_number(char ch)
{
    return 47 < ch && ch < 58;
}

int tokenize(Tokens *tokens, char *data, size_t data_len)
{
    if (data == 0) return 0;

    gb_clean(tokens);

    while (tokens->len < data_len) {
        if (data[tokens->len] == '\'') {
            gb_append(tokens, TOKEN_STRING);

            while (tokens->len < data_len) {
                if (data[tokens->len] == '\\') {
                    gb_append(tokens, TOKEN_STRING);
                    gb_append(tokens, TOKEN_STRING);
                } else if (data[tokens->len] == '\'') {
                    gb_append(tokens, TOKEN_STRING);
                    break;
                } else {
                    gb_append(tokens, TOKEN_STRING);
                }
            }
        } else if (data[tokens->len] == '"') {
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
        } else if (strncmp(&data[tokens->len], "/*", 2) == 0) {
            gb_append(tokens, TOKEN_COMMENT);
            gb_append(tokens, TOKEN_COMMENT);

            while (tokens->len < data_len) {
                if (strncmp(&data[tokens->len], "*/", 2) == 0) {
                    gb_append(tokens, TOKEN_COMMENT);
                    gb_append(tokens, TOKEN_COMMENT);

                    break;
                } else {
                    gb_append(tokens, TOKEN_COMMENT);
                }
            }
        } else if (strncmp(&data[tokens->len], "0x", 2) == 0) {
            gb_append(tokens, TOKEN_NUMBER);
            gb_append(tokens, TOKEN_NUMBER);
        	
            while (tokens->len < data_len) {
            	if (is_number(data[tokens->len]) ||
            		(64 < data[tokens->len] && data[tokens->len] < 71) || //Upper/lower case hex ascii
            		(96 < data[tokens->len] && data[tokens->len] < 103)) {
	                gb_append(tokens, TOKEN_NUMBER);
            	} else {
            		break;
            	}
            }
        } else if (!is_number(data[tokens->len])) {
            bool stop_loop = false;
            while (tokens->len < data_len) {
                switch (data[tokens->len]) {
                case '\t': case '\n':case ' ':
                case '{': case '(': case '[':
                case ':':
                   gb_append(tokens, TOKEN_TEXT);
                   stop_loop = true;
                   break;
                default:
                    gb_append(tokens, TOKEN_TEXT);
                    break;
                }

                if (stop_loop) {
                    break;
                }
            }
        } else {
            gb_append(tokens, TOKEN_NUMBER);
        }
    }

    return 1;
}
