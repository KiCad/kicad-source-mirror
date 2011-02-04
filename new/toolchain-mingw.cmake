
# This is a CMake toolchain file for ARM:
# http://vtk.org/Wiki/CMake_Cross_Compiling

# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=../../toolchain-mingw.cmake ..

# It is here to assist Dick with verifying compilation of /new stuff with mingw (under linux)

set( CMAKE_SYSTEM_NAME Linux )

#-----<configuration>-----------------------------------------------

# configure only the lines within this <configure> block, typically

# default is specific to Dick's machine, again for testing only:
set( WX_MINGW_BASE /opt/wx2.9-mingw )

# specify the cross compiler
set( CMAKE_C_COMPILER   i586-mingw32msvc-gcc )
set( CMAKE_CXX_COMPILER i586-mingw32msvc-g++ )

# where is the target environment
set( CMAKE_FIND_ROOT_PATH /usr/i586-mingw32msvc )

include_directories( ${WX_MINGW_BASE}/include )



#-----</configuration>-----------------------------------------------

# search for programs in the build host directories
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# for libraries and headers in the target directories
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

# try and pre-load this variable, or do it later in ccmake
set( wxWidgets_CONFIG_EXECUTABLE ${WX_MINGW_BASE}/bin/wx-config )
