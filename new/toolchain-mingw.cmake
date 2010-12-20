
# This is a CMake toolchain file for ARM:
# http://vtk.org/Wiki/CMake_Cross_Compiling

# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchain-mingw.cmake ..

# It is here to assist Dick with verifying compilation of /new stuff with mingw (under linux)

set( CMAKE_SYSTEM_NAME Linux )

# Specific to Dick's machine, again for testing only:
include_directories( /svn/wxWidgets/include )


#-----<configuration>-----------------------------------------------

# configure only the lines within this <configure> block, typically

# specify the cross compiler
set( CMAKE_C_COMPILER   i586-mingw32msvc-gcc )
set( CMAKE_CXX_COMPILER i586-mingw32msvc-g++ )

# where is the target environment
set( CMAKE_FIND_ROOT_PATH /usr/i586-mingw32msvc )


#-----</configuration>-----------------------------------------------

# search for programs in the build host directories
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# for libraries and headers in the target directories
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )
