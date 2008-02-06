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
#
# CMAKE_INSTALL_PREFIX (OPTIONAL)
#   
# KICAD_MINIZIP ON/OFF
#   
#
# KICAD_PYTHON ON/OFF
#    
#

# Our project is called 'kicad'. This is what it will be called in
# our makefiles.
PROJECT(kicad)

CMAKE_MINIMUM_REQUIRED(VERSION 2.4.6 FATAL_ERROR)

# Path to local CMake modules
SET(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Command line option to enable or disable building minizip. Minizip
# building is enabled by default. Use -DKICAD_MINZIP=OFF to disable
# building minizip.
OPTION(KICAD_MINIZIP "enable/disable building minizip (default ON)" ON)

# Comment this out if you don't want to build with Python support.
# OPTION(KICAD_PYTHON "enable/disable building with Python support (default OFF)")

# Set default flags for Release build.
SET(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
SET(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
SET(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

# Set default flags for Debug build.
SET(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
SET(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")

# Locations for install targets.
IF(UNIX)
    IF(APPLE)
    ELSE(APPLE)
        # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
        SET(CMAKE_INSTALL_PREFIX /usr/local CACHE PATH "")
        # When used later, "bin" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
        SET(KICAD_BIN bin CACHE PATH "Location of KiCad binaries.")
        SET(KICAD_PLUGINS lib/kicad/plugins CACHE PATH "Location of KiCad plugins.")
        SET(KICAD_DOCS share/doc/kicad CACHE PATH "Location of KiCad documentation files.")
        SET(KICAD_DATA share/kicad CACHE PATH "Location of KiCad data files.")
        SET(KICAD_MODULES ${KICAD_DATA}/modules)
        SET(KICAD_LIBRARY ${KICAD_DATA}/library)
        SET(KICAD_INTERNAT ${KICAD_DATA}/internat)
        SET(KICAD_TEMPLATE ${KICAD_DATA}/template)
    ENDIF(APPLE)
ENDIF(UNIX)

IF(WIN32)
    # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
    SET(CMAKE_INSTALL_PREFIX c:/kicad CACHE PATH "")
    # When used later, "winexe" and others with no leading / is relative to CMAKE_INSTALL_PREFIX.
    SET(KICAD_BIN winexe CACHE PATH "Location of KiCad binaries.")
    SET(KICAD_PLUGINS ${KICAD_BIN}/plugins)
    SET(KICAD_DOCS help)
    SET(KICAD_MODULES modules)
    SET(KICAD_LIBRARY library)
    SET(KICAD_INTERNAT internat)
    SET(KICAD_TEMPLATE template)
ENDIF(WIN32)

FIND_PACKAGE(OpenGL)
IF(OPENGL_FOUND)
    MESSAGE(STATUS "Check for installed OpenGL -- found")
ELSE(OPENGL_FOUND)
    MESSAGE(STATUS "Check for installed OpenGL -- not found")
    MESSAGE(FATAL_ERROR "OpenGL was not found - it is required to build KiCad")
ENDIF(OPENGL_FOUND)

FIND_PACKAGE(Boost)
IF(Boost_FOUND)
    MESSAGE(STATUS "Check for installed Boost -- found")
ELSE(Boost_FOUND)
    MESSAGE(STATUS "Check for installed Boost -- not found")
    MESSAGE(FATAL_ERROR "Boost was not found - it is required to build KiCad")
ENDIF(Boost_FOUND)

# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html
SET(wxWidgets_USE_LIBS base core adv gl html net)
FIND_PACKAGE(wxWidgetsCVS)
IF(wxWidgets_FOUND)
    MESSAGE(STATUS "Check for installed wxWidgets -- found")
ELSE(wxWidgets_FOUND)
    MESSAGE(STATUS "Check for installed wxWidgets -- not found")
    MESSAGE(FATAL_ERROR "wxWidgets was not found - it is required to build KiCad")
ENDIF(wxWidgets_FOUND)

# Include wxWidgets macros.
INCLUDE(${wxWidgets_USE_FILE})

# Include paths.
INCLUDE_DIRECTORIES(
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/share)

# CMake will look in these dirs for nested 'CMakeLists.txt' files.
# Binaries.
ADD_SUBDIRECTORY(common)
ADD_SUBDIRECTORY(3d-viewer)
ADD_SUBDIRECTORY(cvpcb)
ADD_SUBDIRECTORY(eeschema)
ADD_SUBDIRECTORY(gerbview)
ADD_SUBDIRECTORY(kicad)
ADD_SUBDIRECTORY(pcbnew)
ADD_SUBDIRECTORY(polygon)
# Resources.
ADD_SUBDIRECTORY(internat)
ADD_SUBDIRECTORY(help)
ADD_SUBDIRECTORY(library)
ADD_SUBDIRECTORY(modules)
ADD_SUBDIRECTORY(template)
