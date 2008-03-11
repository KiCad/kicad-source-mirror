# List of variables that may be set from command line:
#
# CMAKE_BUILD_TYPE Release/Debug (REQUIRED)
#   Choose build type: Release/Debug
#
# wxWidgets_USE_DEBUG ON/OFF (REQUIRED)
#   Should be set to ON only when CMAKE_BUILD_TYPE=Debug.
#
# wxWidgets_USE_STATIC ON/OFF (OPTIONAL)
#   
#
# CMAKE_VERBOSE_MAKEFILE ON/OFF (OPTIONAL)
#   Turns ON/OFF verbose build messages.
#   You can also pass VERBOSE=1 to make for the same effect.
#
# CMAKE_INSTALL_PREFIX (OPTIONAL)
#   
#
# KICAD_MINIZIP ON/OFF
#   
#
# KICAD_PYTHON ON/OFF
#   
#

# Our project is called 'kicad'. This is what it will be called in
# our makefiles.
project(kicad)

cmake_minimum_required(VERSION 2.4.6 FATAL_ERROR)

# Path to local CMake modules
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Command line option to enable or disable building minizip. Minizip
# building is enabled by default. Use -DKICAD_MINZIP=OFF to disable
# building minizip.
option(KICAD_MINIZIP "enable/disable building minizip (default ON)" ON)

# Comment this out if you don't want to build with Python support.
# OPTION(KICAD_PYTHON "enable/disable building with Python support (default OFF)")

# Set flags for GNU compilers.
if(CMAKE_COMPILER_IS_GNUCXX)
    # Set default flags for Release build.
    set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

    # Set default flags for Debug build.
    set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

# Locations for install targets.
set(KICAD_BIN bin CACHE PATH "Location of KiCad binaries.")

if(UNIX)
    if(APPLE)
    else(APPLE)
        # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
        set(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "")
        # When used later, "bin" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
        set(KICAD_PLUGINS lib/kicad/plugins CACHE PATH "Location of KiCad plugins.")
        set(KICAD_DATA share/kicad CACHE PATH "Location of KiCad data files.")
    endif(APPLE)
endif(UNIX)

if(WIN32)
    # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
    set(CMAKE_INSTALL_PREFIX c:/kicad CACHE PATH "")
    # When used later, "winexe" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
    set(KICAD_PLUGINS ${KICAD_BIN}/plugins)
    set(KICAD_DATA share CACHE PATH "Location of KiCad data files.")
endif(WIN32)

set(KICAD_DEMOS ${KICAD_DATA}/demos)
set(KICAD_INTERNAT ${KICAD_DATA}/internat)
set(KICAD_TEMPLATE ${KICAD_DATA}/template)

find_package(OpenGL)
if(OPENGL_FOUND)
    message(STATUS "Check for installed OpenGL -- found")
else(OPENGL_FOUND)
    message(STATUS "Check for installed OpenGL -- not found")
    message(FATAL_ERROR "OpenGL was not found - it is required to build KiCad")
endif(OPENGL_FOUND)

find_package(Boost)
if(Boost_FOUND)
    message(STATUS "Check for installed Boost -- found")
else(Boost_FOUND)
    message(STATUS "Check for installed Boost -- not found")
    message(FATAL_ERROR "Boost was not found - it is required to build KiCad")
endif(Boost_FOUND)

# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
set(wxWidgets_USE_LIBS base core adv gl html net)
find_package(wxWidgetsCVS)
if(wxWidgets_FOUND)
    message(STATUS "Check for installed wxWidgets -- found")
else(wxWidgets_FOUND)
    message(STATUS "Check for installed wxWidgets -- not found")
    message(FATAL_ERROR "wxWidgets was not found - it is required to build KiCad")
endif(wxWidgets_FOUND)

# Include wxWidgets macros.
include(${wxWidgets_USE_FILE})

# Include paths.
include_directories(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/share)

# CMake will look in these dirs for nested 'CMakeLists.txt' files.
# Binaries.
add_subdirectory(common)
add_subdirectory(3d-viewer)
add_subdirectory(cvpcb)
add_subdirectory(eeschema)
add_subdirectory(gerbview)
add_subdirectory(kicad)
add_subdirectory(pcbnew)
add_subdirectory(polygon)
# Resources.
add_subdirectory(demos)
add_subdirectory(internat)
add_subdirectory(template)
