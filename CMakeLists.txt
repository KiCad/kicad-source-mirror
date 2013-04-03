project(kicad)

# The minimum CMake version requirement could be different under unix, OSX,  or Windows
if(WIN32)
    cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)   # win32 and win64
elseif(APPLE)
    cmake_minimum_required(VERSION 2.8.0 FATAL_ERROR)   # OSX
else()
    cmake_minimum_required(VERSION 2.6.4 FATAL_ERROR)   # Linux, Unix, and everything else.
endif()

# Path to local CMake modules.
set(CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules)

#
# KiCad build options should be added below.
#
# If you add a new build option, please add it's state to the CopyVersionInfoToClipboard()
# function in common/basicframe.cpp so that build option settings can be included in bug
# reports.
#

option(USE_PCBNEW_NANOMETRES
    "Use nanometers for Pcbnew internal units instead of deci-mils (default ON)." ON)

# Russian GOST patch
option(wxUSE_UNICODE "enable/disable building unicode (default OFF)")
option(KICAD_GOST "enable/disable building using GOST notation for multiple gates per package (default OFF)")

#for those who bored with uppercase
option(KICAD_KEEPCASE "turn-off automatic component name conversion to uppercase if selected")

option(USE_WX_GRAPHICS_CONTEXT
    "Use wxGraphicsContext for rendering (default OFF). Warning, this is experimental")

option(USE_WX_OVERLAY
    "Use wxOverlay: Always ON for MAC (default OFF). Warning, this is experimental")

#One of these 2 option *must* be set to ON:
option(KICAD_STABLE_VERSION
    "set this option to ON to build the stable version of KICAD. mainly used to set version ID (default OFF)"
    )

option(KICAD_TESTING_VERSION
    "set this option to ON to build the testing version of KICAD. mainly used to set version ID (default OFF)"
    )

option(KICAD_SCRIPTING
    "set this option ON to build the scripting support inside kicad binaries"
    )

option(KICAD_SCRIPTING_MODULES
    "set this option ON to build kicad modules that can be used from scripting languages"
    )

option(KICAD_SCRIPTING_WXPYTHON
    "set this option ON to build wxpython implementation for wx interface building in python and py.shell"
    )

# when option KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES is enabled:
# PYTHON_EXECUTABLE can be defined when invoking cmake
# ( use -DPYTHON_EXECUTABLE=<python path>/python.exe or python2 )
# when not defined by user, the default is python.exe under Windows and python2 for others
# python binary file should be is exec path.


option(USE_FP_LIB_TABLE "Use the new footprint library table implementation. (default OFF)")

#Set version option (stable or testing)

if(KICAD_STABLE_VERSION AND KICAD_TESTING_VERSION )
    message(FATAL_ERROR "Only one KiCad build version option KICAD_TESTING_VERSION or KICAD_STABLE_VERSION can be set to ON")
elseif(NOT KICAD_STABLE_VERSION AND NOT KICAD_TESTING_VERSION)
    message(FATAL_ERROR "Either KiCad build version option KICAD_TESTING_VERSION or KICAD_STABLE_VERSION must be set to ON")
elseif(KICAD_STABLE_VERSION)
    add_definitions(-DKICAD_STABLE_VERSION)
    message( "Build stable version of KiCad")
else()
    add_definitions(-DKICAD_TESTING_VERSION)
    message("Build testing (unstable) version of KiCad")
endif()


#================================================
# Set flags for GCC.
#================================================

if(CMAKE_COMPILER_IS_GNUCXX)

    set(KICAD_GCC_RELEASE_BUILD_FLAGS "-O2")
    set(KICAD_GCC_RELEASE_DEBUG_FLAGS "")

    execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion
        OUTPUT_VARIABLE GCC_VERSION
        OUTPUT_STRIP_TRAILING_WHITESPACE)

    # Added -Wno-narrowing on 10/7/12 to prevent a huge number of warnings when
    # compiling with GCC 4.7.  This appears to be caused by and int to unsigned
    # conversion in the Boost polygon library.  At some point in the future when
    # Boost is updated to the next version, -Wno-narrowing should be removed to
    # see if the problem has been resolved.  Wayne.
    #
    # Also note the optimization level is -O1 instead of the usual -O2 level
    # because boost::polygon has a function (inflate polygon) broken by
    # the -O2 level with GCC 4.7 (works fine with  with GCC 4.6).
    # This lower optimization level does not have a significant change on the speed.
    #
    # As newer versions of GCC and/or Boost are released, this code needs reviewed to
    # determine if the problems above have been fixed either in Boost or GCC.
    if(GCC_VERSION VERSION_GREATER 4.7 OR GCC_VERSION VERSION_EQUAL 4.7)
        set(KICAD_GCC_RELEASE_BUILD_FLAGS "-Wno-narrowing -O1")
        set(KICAD_GCC_DEBUG_BUILD_FLAGS "-Wno-narrowing")
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL Debug)
        message(STATUS
            "Setting GCC version ${GCC_VERSION} build flags \"${KICAD_GCC_DEBUG_BUILD_FLAGS}\"")
    else()
        message(STATUS
            "Setting GCC version ${GCC_VERSION} build flags \"${KICAD_GCC_RELEASE_BUILD_FLAGS}\"")
    endif()

    if(MINGW)
        # According to some sources, under Windows -fPIC option is not needed:
        # http://mingw.5.n7.nabble.com/Option-fPIC-not-supported-td18480.html

        # Set default flags for Release build.
        set(CMAKE_C_FLAGS_RELEASE "-Wall ${KICAD_GCC_RELEASE_BUILD_FLAGS} -DNDEBUG")
        set(CMAKE_CXX_FLAGS_RELEASE "-Wall ${KICAD_GCC_RELEASE_BUILD_FLAGS} -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s -static-libgcc -static-libstdc++")

        # Set default flags for Debug build.
        set(CMAKE_C_FLAGS_DEBUG "-Wall ${KICAD_GCC_DEBUG_BUILD_FLAGS} -g3 -ggdb3 -DDEBUG")
        set(CMAKE_CXX_FLAGS_DEBUG "-Wall ${KICAD_GCC_DEBUG_BUILD_FLAGS} -g3 -ggdb3 -DDEBUG")
        set(CMAKE_MODULE_LINKER_FLAGS "-static-libgcc -static-libstdc++") # SWIG macros on Windows

    else()
        # We build DLL/DSOs from static libraries, so create position independent code
        # for all cases, since we do not have DLL/DSO specific static libraries.
        # This flag could be localized to any object file going into a DLL/DSO in the future.
        set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fPIC" )

        # Thou shalt not link vaporware and tell us it's a valid DSO:
        set(CMAKE_SHARED_LINKER_FLAGS "-Wl,--no-undefined")
        set(CMAKE_MODULE_LINKER_FLAGS "-Wl,--no-undefined") # needed by SWIG macros on linux

        # Set default flags for Release build.
        set(CMAKE_C_FLAGS_RELEASE "${KICAD_GCC_RELEASE_BUILD_FLAGS} -Wall -DNDEBUG")
        set(CMAKE_CXX_FLAGS_RELEASE "${KICAD_GCC_RELEASE_BUILD_FLAGS} -Wall -DNDEBUG")
        set(CMAKE_EXE_LINKER_FLAGS_RELEASE "-s")

        # Set default flags for Debug build.
        set(CMAKE_C_FLAGS_DEBUG "${KICAD_GCC_DEBUG_BUILD_FLAGS} -Wall  -g3 -ggdb3 -DDEBUG")
        set(CMAKE_CXX_FLAGS_DEBUG "${KICAD_GCC_DEBUG_BUILD_FLAGS} -Wall -g3 -ggdb3 -DDEBUG")
    endif()

endif(CMAKE_COMPILER_IS_GNUCXX)

if(wxUSE_UNICODE)
    add_definitions(-DwxUSE_UNICODE)
endif()

if(KICAD_GOST)
    add_definitions(-DKICAD_GOST)
endif()

if(KICAD_KEEPCASE)
    add_definitions(-DKICAD_KEEPCASE)
endif()

if(USE_WX_OVERLAY OR APPLE)
    add_definitions(-DUSE_WX_OVERLAY)
endif()

if(KICAD_SCRIPTING)
    add_definitions(-DKICAD_SCRIPTING)
endif()

if(KICAD_SCRIPTING_MODULES)
    add_definitions(-DKICAD_SCRIPTING_MODULES)
endif()

if(KICAD_SCRIPTING_WXPYTHON)
    add_definitions(-DKICAD_SCRIPTING_WXPYTHON)
endif()

if(USE_WX_GRAPHICS_CONTEXT)
    add_definitions(-DUSE_WX_GRAPHICS_CONTEXT)
endif()

# Allow user to override the default settings for adding images to menu items.  By default
# images in menu items are enabled on all platforms except OSX.  This can be over ridden by
# defining -DUSE_IMAGES_IN_MENUS=ON/OFF to force the preferred behavior.
if(NOT DEFINED USE_IMAGES_IN_MENUS)
    if(NOT APPLE)
        set(USE_IMAGES_IN_MENUS ON)
    endif()
else()
    if(USE_IMAGES_IN_MENUS)
        set(USE_IMAGES_IN_MENUS ON)
    endif()
endif()

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
endif()

if(MINGW)
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
endif()

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

include(Functions)

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
if(MINGW AND USE_WX_GRAPHICS_CONTEXT)
    find_package(GdiPlus)
    check_find_package_result(GDI_PLUS_FOUND "GDI+")
endif()

# Find Python and other scripting resources
if(KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES)
    set(PythonInterp_FIND_VERSION)
    find_package(PythonInterp)
    check_find_package_result(PYTHONINTERP_FOUND "Python Interpreter")

    # Get the correct Python site package install path from the Python interpreter found by
    # FindPythonInterp unless the user specifically defined a custom path.
    if(NOT PYTHON_SITE_PACKAGE_PATH)
        execute_process(COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig;print\"%s\"%distutils.sysconfig.get_python_lib()"
            OUTPUT_VARIABLE PYTHON_SITE_PACKAGE_PATH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            )

        if(NOT PYTHON_SITE_PACKAGE_PATH)
            message(FATAL_ERROR "Error occurred while attemping to find the Python site library path.")
        endif()
    endif()

    set(PYTHON_DEST "${PYTHON_SITE_PACKAGE_PATH}" CACHE PATH "Python module install path.")
    mark_as_advanced(PYTHON_DEST)
    message( STATUS "Python module install path: ${PYTHON_DEST}")
    find_package(PythonLibs)
    include_directories(${PYTHON_INCLUDE_PATH}
                        ./scripting)
endif(KICAD_SCRIPTING OR KICAD_SCRIPTING_MODULES)



# Automagically create version header file.
include(CreateBzrVersionHeader)
create_bzr_version_header()

if(EXISTS ${CMAKE_SOURCE_DIR}/include/config.h)
    # This file may exist (created by an alternate process to the svn test above),
    # e.g. could be created by a checkout script that produces a source tar file.
    # This directive means the about box will have the svn date & revision in it,
    # but the hard coded release date (program version) will be preserved.
    add_definitions(-DHAVE_SVN_REVISION)
endif()

# For include_directories(BEFORE ...), which _reverses_
# the order during insertion, so put first wanted last, which is
# ${CMAKE_SOURCE_DIR/include.  Use '.' for current source dir since
# we don't want expansion here and now, and don't need expansion of
# ${CMAKE_CURRENT_SOURCE_DIR}, '.' will work for any source directory.
set(INC_BEFORE
    .
    ${CMAKE_SOURCE_DIR}/include
    )

set(INC_AFTER
    #include <config.h>
    ${CMAKE_BINARY_DIR}
    )

#================================================
# Let CMake look in these directories for nested
# 'CMakeLists.txt' files to process
#================================================

############
# Binaries #
############

add_subdirectory(bitmaps_png)
add_subdirectory(common)
add_subdirectory(3d-viewer)
add_subdirectory(cvpcb)
add_subdirectory(eeschema)
add_subdirectory(gerbview)
add_subdirectory(kicad)
add_subdirectory(pcbnew)
add_subdirectory(pcbnew/pcad2kicadpcb_plugin)
add_subdirectory(polygon)
add_subdirectory(potrace)
add_subdirectory(bitmap2component)
add_subdirectory(pcb_calculator)
add_subdirectory(tools)
#add_subdirectory(new)


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
    add_custom_target( doxygen-docs
        ${CMAKE_COMMAND} -E remove_directory Documentation/doxygen
        COMMAND ${DOXYGEN_EXECUTABLE}
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        DEPENDS Doxyfile
        COMMENT "building doxygen docs into directory Documentation/doxygen/html"
        )
else()
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
endif()

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
endif()

include(CTest)
