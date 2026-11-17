This directory contains the lemon parser main program file and template file.
It is used for building several parsers in common, and is provided because not
all distributions package it in a nice way. Note, this program is not installed
with KiCad and is only used as a build-time utility.

The files come from:
* lempar.c - https://www.sqlite.org/src/file/tool/lempar.c
* lemon.c - https://www.sqlite.org/src/file/tool/lemon.c

It has been released into the public domain with the sqlite project.

Local changes, to keep when updating from upstream:
* lemon.c - The %extra_argument and %extra_context fetch macros cast the fetched
  local to void, so a parser routine that never reads it raises no unused
  variable warning (MSVC C4189 in yy_destructor).
