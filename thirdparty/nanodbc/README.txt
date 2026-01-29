This directory contains the nanodbc project from https://github.com/nanodbc/nanodbc

It is licensed under MIT, with the license text in this directory.


Note: The CMakeLists.txt file has been modified to remove the CXX flags nanodbc is
adding to the build. These flags were interfering with the Clang build, and also
including Werror is a bad idea.

Note: The cpp and header file have been modified to remove the #ifdef _clang_ from the
includes. This was needed when nanodbc changed the standard library, but now with GCC 13,
this is needed when building with the GCC standard library and clang.

Note: The cpp file has been modified to add fallback handling for SQLDescribeParam failures.
Microsoft non-SQL ODBC drivers (Access, Excel, CSV) do not implement SQLDescribeParam,
causing parameter binding to fail. The fix falls back to SQL_VARCHAR with size 255 when
SQLDescribeParam fails, matching the approach used by the r-dbi/odbc project.
See https://gitlab.com/kicad/code/kicad/-/issues/15880
