
# This is a CMake toolchain file so we can using Mingw to build Windows32 binaries.
# http://vtk.org/Wiki/CMake_Cross_Compiling

# usage
# cmake -DCMAKE_TOOLCHAIN_FILE=./toolchain-mingw32.cmake ../

set( CMAKE_SYSTEM_NAME Windows )
set( CMAKE_SYSTEM_PROCESSOR i686 )

#-----<configuration>-----------------------------------------------

# configure only the lines within this <configure> block, typically

set( TC_PATH /usr/bin )
set( CROSS_COMPILE x86_64-w64-mingw32- )

# specify the cross compiler
set( CMAKE_C_COMPILER   ${TC_PATH}/${CROSS_COMPILE}gcc )
set( CMAKE_CXX_COMPILER ${TC_PATH}/${CROSS_COMPILE}g++ )
set( CMAKE_RC_COMPILER  ${TC_PATH}/${CROSS_COMPILE}windres )

# One compiler generates either 32 bit or 64 bit code, -m32 says generate 32 bit:
set( CMAKE_CXX_FLAGS_INIT       "-m32"  CACHE STRING "C++ flag which generates 32 bit code." )
set( CMAKE_C_FLAGS_INIT         "-m32"  CACHE STRING "C flag which generates 32 bit code." )
set( CMAKE_SHARED_LINKER_FLAGS  "-m32"  CACHE STRING "Linker flag which finds 32 bit code." )
set( CMAKE_MODULE_LINKER_FLAGS  "-m32"  CACHE STRING "Linker flag which finds 32 bit code." )
set( CMAKE_EXE_LINKER_FLAGS     "-m32"  CACHE STRING "Linker flag which finds 32 bit code." )

# Tell the 64 bit toolchain to generate a PE32 object file when running windres:
set( CMAKE_RC_FLAGS             "-F pe-i386" CACHE STRING "windres flag which generates 32 bit code." )


# where is the target environment
set( CMAKE_FIND_ROOT_PATH /usr/x86_64-w64-mingw32 )

#-----</configuration>-----------------------------------------------

# search for programs in the build host directories
set( CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER )

# for libraries and headers in the target directories
set( CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY )
set( CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY )

