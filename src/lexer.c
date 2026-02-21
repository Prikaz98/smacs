#include "render.h"
#include "lexer.h"

const char *c_keywords[] = {
	"auto", "break", "case", "const", "continue", "default", "do",
	"else", "enum", "extern", "for", "goto", "if", "register",
	"return", "sizeof", "static", "struct", "switch", "typedef",
	"volatile", "while", "alignas", "alignof", "try", "throw", "private", "protected",
	"public", "continue;"
};
#define c_keywords_count (sizeof(c_keywords)/sizeof(c_keywords[0]))

const char *c_types[] = {"signed", "unsigned", "char", "short", "int", "long", "float",
						 "double", "void", "bool", "true", "false", "size_t", "NULL",
						 "#include", "#define", "uint32_t", "#ifndef", "#endif"};

#define c_types_count (sizeof(c_types)/sizeof(c_types[0]))

void
lexer_c_syntax(SimpleLexer *lexer)
{
	lexer->keywords = (char **)c_keywords;
	lexer->keywords_len = c_keywords_count;
	lexer->types = (char **)c_types;
	lexer->types_len = c_types_count;
}

size_t
lexer_is_keyword(SimpleLexer *lexer, char *data)
{
	size_t len, data_len, i;
	char *keyword;

	if (lexer == NULL) return 0;
	if (data == NULL) return 0;

	data_len = strlen(data);
	if (data_len == 0) return 0;

	for (i = 0; i < lexer->keywords_len; ++i) {
		keyword = lexer->keywords[i];
		len = strlen(keyword);
		if (len >= data_len) continue;

		if (strncmp(data, keyword, len) == 0 &&
			(data_len == len || data[len] == ' ' || data[len] == '\t' || data[len] == '\n')) return len;
	}

	return 0;
}

size_t
lexer_is_type(SimpleLexer *lexer, char *data)
{
	size_t len, data_len, i;
	char *type;

	if (lexer == NULL) return 0;
	if (data == NULL) return 0;

	data_len = strlen(data);
	if (data_len == 0) return 0;

	for (i = 0; i < lexer->types_len; ++i) {
		type = lexer->types[i];
		len = strlen(type);
		if (len > data_len) continue;

		if (strncmp(data, type, len) == 0 &&
			(data_len == len || data[len] == ' ' || data[len] == '\t' || data[len] == '\n')) return len;
	}

	return 0;
}
