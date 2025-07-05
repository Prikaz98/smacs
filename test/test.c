#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include "./../src/editor.h"

int main()
{
    Editor editor = (Editor) {0};
    editor.buffer = (Buffer) {0};
    editor.position = 0;

    char *str = "hello";
    editor_insert(&editor, str);

    assert(editor.position == strlen(str));
    assert(editor.buffer.capacity == BUF_CAP);
    assert(editor.buffer.lines_count == 1);


    editor_delete_backward(&editor);
    assert(editor.position == (strlen(str) - 1));


    assert(editor.position == 0);

    editor_insert(&editor, str);
    assert(editor.position == strlen(str));
    assert(editor.buffer.capacity == BUF_CAP);

    return 0;
}
