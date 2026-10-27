This directory contains the nanodbc project from https://github.com/nanodbc/nanodbc

It is licensed under MIT, with the license text in this directory.


Note: The CMakeLists.txt file has been modified to remove the CXX flags nanodbc is
adding to the build. These flags were interfering with the Clang build, and also
including Werror is a bad idea.

Note: The cpp and header file have been modified to remove the #ifdef _clang_ from the
includes. This was needed when nanodbc changed the standard library, but now with GCC 13,
this is needed when building with the GCC standard library and clang.

Note: recent_error() has been modified to copy the whole SQLSTATE.  size() already excludes
the terminator, so subtracting one more dropped the last character ("08001" became "0800").

Note: The SQL_LOGIN_TIMEOUT calls in connect() ignore failures whose first diagnostic record
is HYC00 or HY092, allowing connection attempts when login timeout support is unavailable.

Note: catalog::columns::column_size() returns 0 when COLUMN_SIZE is NULL, as decimal_digits() already
does, instead of throwing null_access_error.
