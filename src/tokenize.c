#include <stdio.h>
#include <stdbool.h>
#include <ctype.h>
#include "tokenize.h"
#include "common.h"

void tokenize_read_string_literal(Tokens *tokens, char *data, size_t data_len)
{
	gb_append(tokens, TOKEN_STRING);

	while (tokens->len < data_len) {
		if (data[tokens->len] == '\\') {
			gb_append(tokens, TOKEN_STRING);
			gb_append(tokens, TOKEN_STRING);
		} else if (data[tokens->len] == '$') {
			gb_append(tokens, TOKEN_TEXT);
			if (data[tokens->len] == '{') {
				gb_append(tokens, TOKEN_TEXT);
				while (tokens->len < data_len) {
					gb_append(tokens, TOKEN_TEXT);
					if (data[tokens->len] == '}') {
						gb_append(tokens, TOKEN_TEXT);
						break;
					}
				}
			} else {
				while (tokens->len < data_len) {
					gb_append(tokens, TOKEN_TEXT);
					if (ispunct(data[tokens->len]) || isspace(data[tokens->len])) {
						break;
					}
				}
			}
		} else if (data[tokens->len] == '"') {
			gb_append(tokens, TOKEN_STRING);
			break;
		} else {
			gb_append(tokens, TOKEN_STRING);
		}
	}
}

void tokenize_read_char_literal(Tokens *tokens, char *data, size_t data_len)
{
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
}

void tokenize_read_single_line_comment(Tokens *tokens, char *data, size_t data_len)
{
	gb_append(tokens, TOKEN_COMMENT);
	gb_append(tokens, TOKEN_COMMENT);

	while (tokens->len < data_len) {
		gb_append(tokens, TOKEN_COMMENT);
		if (data[tokens->len] == '\n') {
			break;
		}
	}
}

void tokenize_read_multi_line_comment(Tokens *tokens, char *data, size_t data_len)
{
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
}

void tokenize_read_hexadecimal_number(Tokens *tokens, char *data, size_t data_len)
{
	gb_append(tokens, TOKEN_NUMBER);
	gb_append(tokens, TOKEN_NUMBER);

	while (tokens->len < data_len) {
		gb_append(tokens, TOKEN_NUMBER);
		if (!isxdigit(data[tokens->len])) {
			break;
		}
	}
}

void tokenize_read_decimal_number(Tokens *tokens, char *data, size_t data_len)
{
	gb_append(tokens, TOKEN_NUMBER);

	while (tokens->len < data_len) {
		if (!isdigit(data[tokens->len]) &&
			data[tokens->len] != '.' &&
			data[tokens->len] != '_') {
			break;
		}
		gb_append(tokens, TOKEN_NUMBER);
	}
}

int tokenize(Tokens *tokens, char *data, size_t data_len)
{
	if (data == 0) return 0;

	gb_clean(tokens);

	while (tokens->len < data_len) {
		if (data[tokens->len] == '\'') {
			tokenize_read_char_literal(tokens, data, data_len);
		} else if (data[tokens->len] == '"') {
			tokenize_read_string_literal(tokens, data, data_len);
		} else if (strncmp(&data[tokens->len], "//", 2) == 0) {
			tokenize_read_single_line_comment(tokens, data, data_len);
		} else if (strncmp(&data[tokens->len], "/*", 2) == 0) {
			tokenize_read_multi_line_comment(tokens, data, data_len);
		} else if (strncmp(&data[tokens->len], "0x", 2) == 0) {
			tokenize_read_hexadecimal_number(tokens, data, data_len);
		} else if (isdigit(data[tokens->len])) {
			tokenize_read_decimal_number(tokens, data, data_len);
		} else if (strncmp(&data[tokens->len], "false", 5) == 0) {
			for (int i = 0; i < 5; ++i) {
				gb_append(tokens, TOKEN_BOOLEAN);
			}
		} else if (strncmp(&data[tokens->len], "true", 4) == 0) {
			for (int i = 0; i < 4; ++i) {
				gb_append(tokens, TOKEN_BOOLEAN);
			}
		} else {
			bool stop_loop = false;
			while (tokens->len < data_len) {
				switch (data[tokens->len]) {
				//TODO(ivan): isspace(ch) maybe?
				case '\t': case '\n': case ' ':
				case '\r': case '\f': case '\v':
				case '{': case '(': case '[':
				case ':':
					gb_append(tokens, TOKEN_TEXT);
					stop_loop = true;
					break;
				case '"': case '\'':
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
		}
	}
	return 1;
}
