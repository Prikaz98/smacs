#include <string.h>
#include <stdbool.h>

bool starts_with(char *a, char *b)
{
    size_t len;

    len = strlen(b);

    return strncmp(a, b, len) == 0;
}
