#include <stdio.h>
#include <string.h>
#include <assert.h>

#include "../src/tokenize.h"

static char *TOKEN_KIND_STRING[] = {
	"TOKEN_TEXT",
	"TOKEN_STRING",
	"TOKEN_COMMENT"
};

int main(void)
{
	//"#inculde <stdio.h>\nint main(void)\n{\nprintf(\"Hello, World\n\");\nreturn 0;\n}";
	char *cfile = "char *str = \"hello\"; //array hello\nint i = 0;";
	size_t cfile_len = strlen(cfile);

	Tokens tokens = {0};
	assert(tokenize(&tokens, cfile, cfile_len) && "Should return success");

	for (size_t i; i < tokens.len; ++i) {
		fprintf(stderr, "i: %ld(%c) = %s\n", i, cfile[i], TOKEN_KIND_STRING[tokens.data[i]]);
	}

	//TokenKind expected[] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0};
	//size_t expected_len = sizeof(expected) / sizeof(TokenKind);
	//
	//assert(memcmp(&tokens.data[0], &expected[0], expected_len) == 0 && "Expected data does not match with result");
	return 0;
}
