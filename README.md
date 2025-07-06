# Smacs (Short eMACS)

Custom implementation of Emacs-like text editor.

![smacs.png](./smacs.png)

## Motivation

I like [Emacs](https://emacsdocs.org/) and I want to learn how this kind of app works internally.

## Mappings

In the following keybinding description I use standard Emacs key definition:
C - Ctrl
M - Alt (Meta)

General:
C-f - forward char
C-b - backward char
C-n - next line
C-p - previous line
C-a - move cursor to the beginning of the line
C-e - move cursor to the end of the line
C-v - scroll up
M-v - scroll down
M-< - beginning of the buffer
M-> - end of the buffer
F2 - save file

Deleting:
C-k - delete from cursor position to the end of the line
C-d - delete forward char
BACKSPACE - delete backward char

Selection:
C-SPC - set mark
M-w - copy to clipboard
C-y - paste from clipboard

View manipulation:
C-l recenter

# Information line
[${file_path} ($current_line_number)]

# Thanks
Everything works using [SDL](https://www.libsdl.org/).
Color theme inspired by [mindre-theme](https://github.com/erikbackman/mindre-theme).
