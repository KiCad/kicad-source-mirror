project(kicad)

# test the minimum Cmake version requirement (could be different under unix or Windows
if(WIN32)
    cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)
else(WIN32)
    cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)
endif(WIN32)

# Path to local CMake modules.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

# Command line option to enable or disable building minizip. Minizip
# building is enabled by default. Use -DKICAD_MINZIP=OFF to disable
# building minizip.
option(KICAD_MINIZIP "enable/disable building minizip (default ON)" ON)

# Russian GOST patch
option(wxUSE_UNICODE "enable/disable building unicode (default OFF)")
option(KICAD_GOST "enable/disable building using GOST notation for multiple gates per package (default OFF)")

#for those who bored with uppercase
option(KICAD_KEEPCASE "turn-off automatic component name conversion to uppercase if selected")

option(USE_WX_ZOOM "Use wxDC to perform zooming (default OFF). Warning, this is experimental" )

option(USE_WX_GRAPHICS_CONTEXT
       "Use wxGraphicsContext for rendering (default OFF). Warning, this is experimental")

option(USE_WX_OVERLAY
       "Use wxOverlay: Always ON for MAC (default OFF). Warning, this is experimental")

option(USE_BOOST_POLYGON_LIBRARY
       "Use boost polygon library instead of Kbool to calculate filled areas in zones (default ON)."
       ON )

#================================================
# Set flags for GCC.
#================================================

if(CMAKE_COMPILER_IS_GNUCXX)
    # Set default flags for Release build.
    set(CMAKE_C_FLAGS_RELEASE "-Wall -O2 -DNDEBUG ")
    set(CMAKE_CXX_FLAGS_RELEASE "-Wall -O2 -DNDEBUG")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -static-libgcc")

    # Set default flags for Debug build.
    set(CMAKE_C_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
    set(CMAKE_CXX_FLAGS_DEBUG "-Wall -g3 -ggdb3 -DDEBUG")
endif(CMAKE_COMPILER_IS_GNUCXX)

if(wxUSE_UNICODE)
    add_definitions(-DwxUSE_UNICODE)
endif(wxUSE_UNICODE)

if(KICAD_GOST)
    add_definitions(-DKICAD_GOST)
endif(KICAD_GOST)

if(KICAD_KEEPCASE)
    add_definitions(-DKICAD_KEEPCASE)
endif(KICAD_KEEPCASE)

if(USE_WX_ZOOM)
    add_definitions(-DUSE_WX_ZOOM)
endif(USE_WX_ZOOM)

if(USE_WX_OVERLAY OR APPLE)
    add_definitions(-DUSE_WX_OVERLAY)
endif(USE_WX_OVERLAY OR APPLE)


if(USE_WX_GRAPHICS_CONTEXT)
    set( USE_WX_ZOOM ON )
    add_definitions(-DUSE_WX_ZOOM)
    add_definitions(-DUSE_WX_GRAPHICS_CONTEXT)
endif(USE_WX_GRAPHICS_CONTEXT)

if(USE_BOOST_POLYGON_LIBRARY)
    set( USE_BOOST_POLYGON_LIBRARY ON )
    add_definitions(-DUSE_BOOST_POLYGON_LIBRARY)
endif(USE_BOOST_POLYGON_LIBRARY)

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
set(KICAD_TEMPLATE ${KICAD_DATA}/template
    CACHE PATH "Location of KiCad template files.")

mark_as_advanced(KICAD_BIN
                 KICAD_PLUGINS
                 KICAD_DATA
                 KICAD_DOCS
                 KICAD_DEMOS
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

# On Apple only wxwidgets 2.9 or higher doesn't need to find aui part of base
if(APPLE)
    find_package(wxWidgets COMPONENTS gl adv html core net base xml QUIET)
else(APPLE)
    find_package(wxWidgets COMPONENTS gl aui adv html core net base xml QUIET)
endif(APPLE)
check_find_package_result(wxWidgets_FOUND "wxWidgets")


# Include wxWidgets macros.
include(${wxWidgets_USE_FILE})

# Include MinGW resource compiler.
include(MinGWResourceCompiler)

# Generate build system specific header file.
include(PerformFeatureChecks)
perform_feature_checks()

# Find GDI+ on windows if wxGraphicsContext is available.
if(WIN32 AND USE_WX_GRAPHICS_CONTEXT)
    find_package(GdiPlus)
    check_find_package_result(GDI_PLUS_FOUND "GDI+")
endif(WIN32 AND USE_WX_GRAPHICS_CONTEXT)

# Automagically create version header file.
include(CreateBzrVersionHeader)
create_bzr_version_header()

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
add_subdirectory(potrace)
add_subdirectory(bitmap2component)

#############
# Resources #
#############
add_subdirectory(demos)
add_subdirectory(template)

#================================================
# Doxygen Output
#================================================
find_package(Doxygen)
if(DOXYGEN_FOUND)
    add_custom_target( doxygen-docs ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS Doxyfile )
else(DOXYGEN_FOUND)
    message( STATUS "WARNING: Doxygen not found - doxygen-docs (Source Docs) target not created" )
endif()


#================================================
# make uninstall rules
#================================================
configure_file(
  "${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
  "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
  IMMEDIATE @ONLY)

add_custom_target(uninstall
  "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake")


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

include(CTest)
