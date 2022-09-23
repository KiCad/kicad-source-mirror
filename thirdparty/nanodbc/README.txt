This directory contains the nanodbc project from https://github.com/nanodbc/nanodbc

It is licensed under MIT, with the license text in this directory.


Note: The CMakeLists.txt file has been modified to remove the CXX flags nanodbc is
adding to the build. These flags were interfering with the Clang build, and also
including Werror is a bad idea.
