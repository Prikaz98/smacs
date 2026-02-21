#ifndef LEXER_H
#define LEXER_H

typedef struct {
	char **keywords;
	size_t keywords_len;
	char **types;
	size_t types_len;
} SimpleLexer;

void 
lexer_c_syntax(SimpleLexer *lexer);
size_t 
lexer_is_keyword(SimpleLexer *lexer, char *data);
size_t 
lexer_is_type(SimpleLexer *lexer, char *data);

#endif
