#ifndef TOKENIZE_H
#define TOKENIZE_H

typedef enum {
    TOKEN_TEXT,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_NUMBER,
} TokenKind;

typedef struct {
    size_t len, cap;
    TokenKind *data;
} Tokens;

int tokenize(Tokens *tokens, char *data, size_t data_len);
#endif
