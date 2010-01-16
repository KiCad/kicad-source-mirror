project(kicad)

if(WIN32)
    cmake_minimum_required(VERSION 2.6.1 FATAL_ERROR)
else(WIN32)
    cmake_minimum_required(VERSION 2.6.0 FATAL_ERROR)
endif(WIN32)

# Path to local CMake modules.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Command line option to enable or disable building minizip. Minizip
# building is enabled by default. Use -DKICAD_MINZIP=OFF to disable
# building minizip.
option(KICAD_MINIZIP "enable/disable building minizip (default ON)" ON)

# Russian GOST and CYRILLIC patch
option(KICAD_CYRILLIC "enable/disable building using cyrillic (needs unicode) (default OFF)")
option(wxUSE_UNICODE "enable/disable building unicode (default OFF)")
option(KICAD_GOST "enable/disable building using GOST notation for multiple gates per package (default OFF)")


if(APPLE)
    option(KICAD_AUIMANAGER "Enable use of wxAuiManager (default ON)" ON)
    option(KICAD_AUITOOLBAR "Enable use of wxAuiToolBar (default ON)" ON)
else(APPLE)
    option(KICAD_AUIMANAGER "Enable use of wxAuiManager (default OFF)" OFF)
    option(KICAD_AUITOOLBAR "Enable use of wxAuiToolBar (default OFF)" OFF)
endif(APPLE)


# Comment this out if you don't want to build with Python support.
# OPTION(KICAD_PYTHON "enable/disable building with Python support (default OFF)")


#================================================
# Set flags for GCC.
#================================================

if(CMAKE_COMPILER_IS_GNUCXX)
    # Set default flags for Release build.
    set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

    # Set default flags for Debug build.
    set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(KICAD_CYRILLIC)
    add_definitions(-DKICAD_CYRILLIC)
endif(KICAD_CYRILLIC)

if(wxUSE_UNICODE)
    add_definitions(-DwxUSE_UNICODE)
endif(wxUSE_UNICODE)

if(KICAD_GOST)
    add_definitions(-DKICAD_GOST)
endif(KICAD_GOST)

if(KICAD_AUIMANAGER)
    add_definitions(-DKICAD_AUIMANAGER)
endif(KICAD_AUIMANAGER)

if(KICAD_AUITOOLBAR)
    add_definitions(-DKICAD_AUITOOLBAR)
endif(KICAD_AUITOOLBAR)

# Locations for install targets.
set(KICAD_BIN bin
    CACHE PATH "Location of KiCad binaries.")

if(UNIX)
    # Everything without leading / is relative to CMAKE_INSTALL_PREFIX.
    set(KICAD_PLUGINS lib/kicad/plugins
        CACHE PATH "Location of KiCad plugins.")
    set(KICAD_DATA share/kicad
        CACHE PATH "Location of KiCad data files.")
    set(KICAD_DOCS share/doc/kicad
        CACHE PATH "Location of KiCad documentation files.")
endif(UNIX)

if(WIN32)
    # Like all variables, CMAKE_INSTALL_PREFIX can be over-ridden on the command line.
    set(CMAKE_INSTALL_PREFIX c:/kicad
        CACHE PATH "")
    # Everything without leading / is relative to CMAKE_INSTALL_PREFIX.
    set(KICAD_PLUGINS ${KICAD_BIN}/plugins
        CACHE PATH "Location of KiCad plugins.")
    set(KICAD_DATA share
        CACHE PATH "Location of KiCad data files.")
    set(KICAD_DOCS doc
        CACHE PATH "Location of KiCad documentation files.")
endif(WIN32)

set(KICAD_DEMOS ${KICAD_DATA}/demos
    CACHE PATH "Location of KiCad demo files.")
set(KICAD_INTERNAT ${KICAD_DATA}/internat
    CACHE PATH "Location of KiCad i18n files.")
set(KICAD_TEMPLATE ${KICAD_DATA}/template
    CACHE PATH "Location of KiCad template files.")

mark_as_advanced(KICAD_BIN
                 KICAD_PLUGINS
                 KICAD_DATA
                 KICAD_DOCS
                 KICAD_DEMOS
                 KICAD_INTERNAT
                 KICAD_TEMPLATE)


#================================================
# Find libraries that are needed to build KiCad.
#================================================
include(CheckFindPackageResult)

#######################
# Find OpenGL library #
#######################
find_package(OpenGL QUIET)
check_find_package_result(OPENGL_FOUND "OpenGL")

######################
# Find Boost library #
######################
# kicad now includes needed boost files.
# the two next lines can be uncommented to use the native boost lib.
# but this is not a good idea
#find_package(Boost 1.36 QUIET)
#check_find_package_result(Boost_FOUND "Boost")

##########################
# Find wxWidgets library #
##########################
# Here you can define what libraries of wxWidgets you need for your
# application. You can figure out what libraries you need here;
# http://www.wxwidgets.org/manuals/2.8/wx_librarieslist.html

if( KICAD_AUIMANAGER OR KICAD_AUITOOLBAR )
    find_package(wxWidgets COMPONENTS gl aui adv html core net base QUIET)

    # On Apple only wxwidgets 2.9 or higher doesn't need to find aui part of base
    if(APPLE)
		find_package(wxWidgets COMPONENTS gl adv html core net base QUIET)
	endif(APPLE)
else( KICAD_AUIMANAGER OR KICAD_AUITOOLBAR )
    find_package(wxWidgets COMPONENTS gl adv html core net base QUIET)
endif( KICAD_AUIMANAGER OR KICAD_AUITOOLBAR )
check_find_package_result(wxWidgets_FOUND "wxWidgets")


# Include wxWidgets macros.
include(${wxWidgets_USE_FILE})

# Include MinGW resource compiler.
include(MinGWResourceCompiler)


# Generate build system specific header file.
include(PerformFeatureChecks)
perform_feature_checks()

# Automagically create version header file.
include(CreateSVNVersionHeader)
create_svn_version_header()

if(EXISTS ${CMAKE_SOURCE_DIR}/include/config.h)
    # This file may exist (created by an alternate process to the svn test above),
    # e.g. could be created by a checkout script that produces a source tar file.
    # This directive means the about box will have the svn date & revision in it,
    # but the hard coded release date (program version) will be preserved.
    add_definitions(-DHAVE_SVN_REVISION)
endif(EXISTS ${CMAKE_SOURCE_DIR}/include/config.h)


# Include paths.
include_directories(${CMAKE_SOURCE_DIR}/include
                    ${CMAKE_SOURCE_DIR}/share
                    ${CMAKE_BINARY_DIR})


#================================================
# Let CMake look in these directories for nested
# 'CMakeLists.txt' files to process
#================================================

############
# Binaries #
############
add_subdirectory(3d-viewer)
add_subdirectory(bitmaps)
add_subdirectory(common)
add_subdirectory(cvpcb)
add_subdirectory(eeschema)
add_subdirectory(gerbview)
add_subdirectory(kicad)
add_subdirectory(pcbnew)
add_subdirectory(polygon)
add_subdirectory(polygon/kbool/src)

#############
# Resources #
#############
add_subdirectory(demos)
add_subdirectory(internat)
add_subdirectory(template)


#================================================
# Installation parameters
#================================================
install(FILES INSTALL.txt
        DESTINATION ${KICAD_DOCS}
        COMPONENT resources)
install(FILES resources/freeroute.jnlp
        DESTINATION ${KICAD_BIN}
        COMPONENT resources)
###
# Install scripts
###
if(UNIX)
    install(DIRECTORY scripts
            DESTINATION ${KICAD_DOCS}
            COMPONENT resources
            PATTERN ".svn" EXCLUDE)
endif(UNIX)
###
# FreeDesktop .desktop and MIME resources
###
if(UNIX)
    # Set paths
    set(UNIX_MIME_DIR resources/linux/mime)
    set(UNIX_MIMELNK_FILES ${UNIX_MIME_DIR}/mimelnk)
    set(UNIX_MIME_FILES ${UNIX_MIME_DIR}/mime)
    set(UNIX_ICONS_FILES ${UNIX_MIME_DIR}/icons)
    set(UNIX_APPLICATIONS_FILES ${UNIX_MIME_DIR}/applications)

    # Install Mimelnk directory
    install(DIRECTORY ${UNIX_MIMELNK_FILES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share
            COMPONENT resources
            PATTERN ".svn" EXCLUDE)

    # Install Mime directory
    install(DIRECTORY ${UNIX_ICONS_FILES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share
            COMPONENT resources
            PATTERN ".svn" EXCLUDE)

    # Install Icons
    install(DIRECTORY ${UNIX_MIME_FILES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share
            COMPONENT resources
            PATTERN ".svn" EXCLUDE)

    # Install Applications directory (.desktop files)
    install(DIRECTORY ${UNIX_APPLICATIONS_FILES}
            DESTINATION ${CMAKE_INSTALL_PREFIX}/share
            COMPONENT resources
            PATTERN ".svn" EXCLUDE)
endif(UNIX)
