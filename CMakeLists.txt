#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2007-2021 Kicad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
#

cmake_minimum_required( VERSION 3.10 FATAL_ERROR )

# Default to CMAKE_BUILD_TYPE = Release unless overridden on command line
# http://www.cmake.org/pipermail/cmake/2008-September/023808.html
if( DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Set to either \"Release\" or \"Debug\"" )
else()
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Set to either \"Release\" or \"Debug\"" )
endif()

project( kicad )

include( GNUInstallDirs )
include( CMakeDependentOption )

# Path to local CMake modules.
set( CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/CMakeModules )

# On Windows, binaries created by link option -g3 are very large (more than 1Gb for pcbnew,
# and more than 3Gb for the full kicad suite)
# This option create binaries using link option -g1 that create much smaller files, but
# there are less info in debug (but the file names and line numbers are available)
cmake_dependent_option( BUILD_SMALL_DEBUG_FILES "In debug build: create smaller binaries." OFF "NOT MSVC" OFF )


#
# KiCad build options should be added below.
#
# If you add a new build option, please add it's state to the
# OnCopyVersionInfo() function in common/dialog_about/dialog_about.cpp
# so that build option settings can be included in bug reports.
#

option( KICAD_SCRIPTING_WXPYTHON
    "Build wxPython implementation for wx interface building in Python and py.shell (default ON)."
    ON )
mark_as_advanced( KICAD_SCRIPTING_WXPYTHON )

option( KICAD_USE_OCE
    "Build tools and plugins related to OpenCascade Community Edition (default OFF)"
    OFF )

option( KICAD_USE_OCC
    "Build tools and plugins related to OpenCascade Technology (overrides KICAD_USE_OCE, default ON)"
    ON )

option( KICAD_INSTALL_DEMOS
    "Install KiCad demos and examples (default ON)"
    ON )

option( KICAD_BUILD_QA_TESTS
    "Build software Quality assurance unit tests (default ON)"
    ON )

option( KICAD_SPICE
    "Build KiCad with internal Spice simulator."
    ON )

option( KICAD_BUILD_I18N
    "Build the translation libraries."
    OFF )

option( KICAD_I18N_UNIX_STRICT_PATH
    "Use standard Unix locale lookup path (default OFF)."
    OFF )

# Not supported by all platforms (for instance mingw)
option( KICAD_SANITIZE_ADDRESS
    "Build KiCad with sanitizer options. WARNING: Not compatible with gold linker"
    OFF )

# Not supported by all platforms (for instance mingw)
option( KICAD_SANITIZE_THREADS
    "Build KiCad with sanitizer options. WARNING: Not compatible with gold linker"
    OFF )

option( KICAD_STDLIB_DEBUG
    "Build KiCad with libstdc++ debug flags enabled."
    OFF )

option( KICAD_STDLIB_LIGHT_DEBUG
    "Build KiCad with libstdc++ with -Wp,-D_GLIBCXX_ASSERTIONS flag enabled. Not as intrusive as KICAD_STDLIB_DEBUG"
    OFF )

cmake_dependent_option( KICAD_WIN32_BUILD_PARALLEL_CL_MP
    "Build in parallel using the /MP compiler option (default OFF for safety reasons)"
    OFF "WIN32" OFF )

option( KICAD_USE_VALGRIND
    "Build KiCad with valgrind stack tracking enabled."
    OFF )

option( KICAD_NETLIST_QA
    "Run eeschema netlist QA tests (requires Python 3)"
    OFF )

cmake_dependent_option( KICAD_WIN32_DPI_AWARE
    "Turn on DPI awareness for Windows builds only"
    OFF "WIN32" OFF )

cmake_dependent_option( KICAD_WIN32_CONTEXT_WINFIBER
    "Use win32 fibers for libcontext"
    OFF "WIN32" OFF )

option( KICAD_USE_EGL
    "Build KiCad with EGL backend support for Wayland."
    OFF )

cmake_dependent_option( KICAD_USE_BUNDLED_GLEW
    "Use the bundled version of GLEW - only available when KICAD_USE_EGL is set"
    ON "KICAD_USE_EGL"
    OFF )

option( KICAD_DRC_PROTO
    "Build the DRC prototype QA tool"
    OFF )

option( KICAD_GAL_PROFILE
    "Enable profiling info for GAL"
    OFF )

# Global setting: exports are explicit
set( CMAKE_CXX_VISIBILITY_PRESET "hidden" )
set( CMAKE_VISIBILITY_INLINES_HIDDEN ON )

# Global setting: build everything position independent
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

# Global setting: Use C++14
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Enable additional valgrind instrumentation for stack tracking in libcontext
if( KICAD_USE_VALGRIND )
    add_definitions( -DKICAD_USE_VALGRIND )
endif()

if( KICAD_GAL_PROFILE )
    add_definitions( -DKICAD_GAL_PROFILE )
endif()

# Ensure DEBUG is defined for all platforms in Debug builds
# change to add_compile_definitions() after minimum required CMake version is 3.12
set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS $<$<CONFIG:Debug>:DEBUG> )


# Add option to add user directories for linker, if any
LINK_DIRECTORIES( ${LINK_DIRECTORIES_PATH} )

set( KICAD_CONFIG_DIR "kicad" CACHE STRING "Location of user specific KiCad config files" )
mark_as_advanced( KICAD_CONFIG_DIR )
add_definitions( -DKICAD_CONFIG_DIR=${KICAD_CONFIG_DIR} )

# Set default data file path to CMAKE_INSTALL_PREFIX if it wasn't specified during the
# CMake configuration.  The value of DEFAULT_INSTALL_PATH is expanded in config.h and
# used in the source code to define the base path for kicad search paths and environment
# variables.
if( NOT DEFAULT_INSTALL_PATH )
    set( DEFAULT_INSTALL_PATH "${CMAKE_INSTALL_PREFIX}"
         CACHE
         PATH
         "Location of KiCad data files." )
endif()

message( STATUS "KiCad install dir: <${DEFAULT_INSTALL_PATH}>" )

# Generate build system specific header file.
include( PerformFeatureChecks )
perform_feature_checks()


# Setup the compiler warnings
include( ${CMAKE_MODULE_PATH}/Warnings.cmake )

if( KICAD_WIN32_CONTEXT_WINFIBER )
    set(LIBCONTEXT_USE_WINFIBER true)
    add_compile_definitions(LIBCONTEXT_USE_WINFIBER)
else()
    set(LIBCONTEXT_USE_WINFIBER false)
endif()

if( WIN32 )
    # define UNICODE and_UNICODE definition on Windows. KiCad uses unicode.
    # Both definitions are required
    add_definitions(-DUNICODE -D_UNICODE)

    # In fully standards-compliant mode, M_PI et al. are defined only if
    # _USE_MATH_DEFINES is set.
    add_definitions( -D_USE_MATH_DEFINES )

    # Used for the resource compiler and other arch dependent steps
    if( MSVC )
        # CMake does not set CMAKE_SYSTEM_PROCESSOR correctly for MSVC
        # and it will always return the host instead of the target arch
        if("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM64")
            set( KICAD_BUILD_ARCH "arm64" )
            set( KICAD_BUILD_ARCH_ARM64 1 )
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM")
            set( KICAD_BUILD_ARCH "arm" )
            set( KICAD_BUILD_ARCH_ARM 1 )
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
            set( KICAD_BUILD_ARCH "x86" )
            set( KICAD_BUILD_ARCH_X86 1 )
        elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
            set( KICAD_BUILD_ARCH "x64" )
            set( KICAD_BUILD_ARCH_X64 1 )
        else()
            message(FATAL_ERROR "Failed to determine the MSVC target architecture: ${MSVC_C_ARCHITECTURE_ID}")
        endif()
    else()
        if ( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
            set( KICAD_BUILD_ARCH "x86" )
            set( KICAD_BUILD_ARCH_X86 1 )
        elseif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
            set( KICAD_BUILD_ARCH "x64" )
            set( KICAD_BUILD_ARCH_X64 1 )
        endif()
    endif()

    add_definitions( -DKICAD_BUILD_ARCH=${KICAD_BUILD_ARCH} )

    if( KICAD_WIN32_DPI_AWARE )
        add_definitions( -DKICAD_WIN32_DPI_AWARE=1 )
    endif()
endif()

if(KICAD_BUILD_ARCH_X64)
    add_definitions( -DKICAD_BUILD_ARCH_X64 )
elseif(KICAD_BUILD_ARCH_X86)
    add_definitions( -DKICAD_BUILD_ARCH_X86 )
elseif(KICAD_BUILD_ARCH_ARM64)
    add_definitions( -DKICAD_BUILD_ARCH_ARM64 )
elseif(KICAD_BUILD_ARCH_ARM)
    add_definitions( -DKICAD_BUILD_ARCH_ARM )
endif()

#================================================
# Provide access to CCACHE
#================================================
if (USE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    get_property(RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
    set(RULE_LAUNCH_COMPILE "${RULE_LAUNCH_COMPILE} ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${RULE_LAUNCH_COMPILE})

    get_property(RULE_LAUNCH_LINK GLOBAL PROPERTY RULE_LAUNCH_LINK)
    set(RULE_LAUNCH_LINK "${RULE_LAUNCH_LINK} ${CCACHE_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ${RULE_LAUNCH_LINK})

    message(STATUS "Used ${CCACHE_FOUND} for compilation.")
  else(CCACHE_FOUND)
    message(STATUS "CCache was requested but not found.")
  endif(CCACHE_FOUND)
endif(USE_CCACHE)


#================================================
# Provide access to DISTCC
#================================================
if (USE_DISTCC)
  find_program(DISTCC_FOUND distcc)
  if(DISTCC_FOUND)
    get_property(RULE_LAUNCH_COMPILE GLOBAL PROPERTY RULE_LAUNCH_COMPILE)
    set(RULE_LAUNCH_COMPILE "${RULE_LAUNCH_COMPILE} ${DISTCC_FOUND}")
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ${RULE_LAUNCH_COMPILE})
    message(STATUS "Using ${DISTCC_FOUND} for distributed build.")
  else(DISTCC_FOUND)
    message(INFO "Distcc was requested but not found.")
  endif(DISTCC_FOUND)
endif(USE_DISTCC)


#================================================
# Set flags for GCC, or treat llvm as GCC
#================================================

if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )

    # Set 32-bit flag for GCC to prevent excess precision
    if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
        set( CMAKE_CXX_FLAGS "-ffloat-store ${CMAKE_CXX_FLAGS}" )
    endif()

    # Link flags in Debug: -g1 and -ggdb1 or -g1 and -ggdb3 can be used.
    # Level 1 produces minimal information, enough for making basic backtraces.
    # This includes descriptions of functions and external variables, and line number tables,
    # but no information about local variables.
    # Level 3 includes full information, but binaries are much larger.
    if( BUILD_SMALL_DEBUG_FILES )
        set( CMAKE_C_FLAGS_DEBUG   "-g1 -ggdb1" )
        set( CMAKE_CXX_FLAGS_DEBUG "-g1 -ggdb1" )
    else()
        set( CMAKE_C_FLAGS_DEBUG   "-g3 -ggdb3" )
        set( CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb3" )
    endif()

    if( KICAD_SANITIZE_ADDRESS )
        add_definitions( -DKICAD_SANITIZE_ADDRESS )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_SANITIZE_VECTOR -fsanitize=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer" )

        # ASAN shouldn't be used with these options (https://github.com/google/sanitizers/wiki/AddressSanitizer#faq)
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -U_FORTIFY_SOURCE" )
    endif()

    if( KICAD_SANITIZE_THREADS )
        add_definitions( -DKICAD_SANITIZE_THREADS )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_SANITIZE_VECTOR -fsanitize=thread -fno-optimize-sibling-calls -fno-omit-frame-pointer" )

        # Based on this warning https://github.com/JuliaLang/julia/blob/29d5158d27ddc3983ae2e373c4cd05569b9ead6c/src/julia.h#L77
        if( CMAKE_CXX_COMPILER_ID STREQUAL "Clang" AND CMAKE_CXX_COMPILER_VERSION VERSION_LESS 11 )
            message( WARNING "Clang version 11 and below may leak memory when running the thread sanitizer.\n"
                             "Be careful when using this compiler.")
        endif()

        # Just duplicate the same flags as ASAN here, because why not.
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -U_FORTIFY_SOURCE" )
    endif()

    if( KICAD_STDLIB_DEBUG )
        add_definitions( -DKICAD_STDLIB_DEBUG )
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG" )
    elseif( KICAD_STDLIB_LIGHT_DEBUG )
        # useless if KICAD_STDLIB_DEBUG is ON.
        # this option makes some controls to trap out of bound memory access.
        add_definitions( -DKICAD_STDLIB_LIGHT_DEBUG )
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wp,-D_GLIBCXX_ASSERTIONS" )
    endif()

    if( MINGW )
        list(APPEND mingw_resource_compiler_INCLUDE_DIRS ${CMAKE_SOURCE_DIR}/resources/msw/ )
        # include dir to allow getting the version header
        list(APPEND mingw_resource_compiler_INCLUDE_DIRS ${CMAKE_BINARY_DIR}/ )
        list(APPEND mingw_resource_compiler_DEFINES KICAD_BUILD_ARCH=${KICAD_BUILD_ARCH} )
        if( KICAD_WIN32_DPI_AWARE )
            list(APPEND mingw_resource_compiler_DEFINES KICAD_WIN32_DPI_AWARE=1 )
        endif()

        set( CMAKE_EXE_LINKER_FLAGS_RELEASE "-s" )

        # Since version 2.8.5, Cmake uses a response file (.rsp) to
        # pass the list of include paths to gcc
        # unfortunately, under mingw32+msys, at least with gcc 4.8 and previous,
        # this file is not always expanded and in this case include paths defined in this file
        # are not taken in account ( this is the case of wxWidgets includes )
        # If it is the case, disable this response file for includes ( See Windows-GNU.cmake module )
        if( false ) #set to true to disable the include response file
            if( WIN32 AND MSYS AND NOT CMAKE_CROSSCOMPILING )
                # fixme: it is needed only with MSYS+MINGW32? or always under MINGW
                if( ${CMAKE_SIZEOF_VOID_P} MATCHES 4 )
                    set( CMAKE_CXX_USE_RESPONSE_FILE_FOR_INCLUDES 0 )
                endif()
            endif()
        endif()

        # for some reasons, cmake does do use always a response file to send the list of objects
        # to the archiver, and because this list can be very long, and can create issue
        # when it is used in a command line, force use of a response file to store it
        SET( CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1 )

        # The MinGW compiler can use the microsoft system snprintf as standard and it has a broken
        # API with respect to the C99 standard, so make sure we force it to use its own compliant
        # snprintf
        add_definitions(-D__USE_MINGW_ANSI_STDIO=1)

        # Allow linking for Boost for the UUID against bcrypt
        set( EXTRA_LIBS "bcrypt" )
    endif()

    if( APPLE )
        set( CMAKE_LD_FLAGS "${CMAKE_LD_FLAGS} -headerpad_max_install_names") # needed by fixbundle
    endif()

endif( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )

if( MSVC )
    include_directories( ${CMAKE_SOURCE_DIR}/resources/msw/ )

    # Disallow implicit linking for Boost
    add_definitions( -DBOOST_ALL_NO_LIB )
    # But allow it for the UUID so that it will link against bcrypt
    add_definitions( -DBOOST_UUID_FORCE_AUTO_LINK )
    # Disable MSVC's deprecation warnings
    add_definitions( -D_CRT_SECURE_NO_WARNINGS -D_CRT_NONSTDC_NO_DEPRECATE -D_SCL_SECURE_NO_WARNINGS )
    # Hide Windows's min() and max() macros
    add_definitions( -DNOMINMAX )
    # source and execution charset are UTF-8
    string( APPEND CMAKE_CXX_FLAGS " /utf-8" )
    # C4290: throw() is interpreted as declspec(nothrow)
    string( APPEND CMAKE_CXX_FLAGS " /wd4290" )
    # C4800: non-bool is explicitly cast to bool, forcing value of 0 or 1
    string( APPEND CMAKE_CXX_FLAGS " /wd4800" )
    # /Zi: create PDB
    string( APPEND CMAKE_CXX_FLAGS " /Zi" )
    # /GF: enable string pooling
    string( APPEND CMAKE_CXX_FLAGS_RELEASE " /GF" )
    string( APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " /GF" )
    # /Gy: Enable Function-Level Linking
    string( APPEND CMAKE_CXX_FLAGS_RELEASE " /Gy" )
    string( APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " /Gy" )
    # Avoid fatal error because /GF + swig wrapper exceed standard obj file limits
    string( APPEND CMAKE_CXX_FLAGS " /bigobj" )

    # Exception handling
    # Remove the potential default EHsc option cmake doesn't allow us to remove easily
    string( REPLACE "/EHsc" "" CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} )
    if( KICAD_WIN32_CONTEXT_WINFIBER )
    # /EHsc: ensure standard exception C++ unwinding is enabled, "extern C" can never throw exceptions
    string( APPEND CMAKE_CXX_FLAGS " /EHsc" )
    else()
    # the asm libcontext implementation a slight change of rules
    # /EHs: ensure standard exception C++ unwinding is enabled, "extern C" MAY throw exceptions
    string( APPEND CMAKE_CXX_FLAGS " /EHs" )
    endif()

    foreach( type EXE SHARED MODULE)
        # /DEBUG: create PDB
        string( APPEND CMAKE_${type}_LINKER_FLAGS " /DEBUG /MANIFEST:NO" )
        # /OPT:REF: omit unreferenced code
        string( APPEND CMAKE_${type}_LINKER_FLAGS_RELEASE " /OPT:REF /MANIFEST:NO" )
        string( APPEND CMAKE_${type}_LINKER_FLAGS_RELWITHDEBINFO " /OPT:REF /MANIFEST:NO" )
        # /OPT:ICF: fold common data
        string( APPEND CMAKE_${type}_LINKER_FLAGS_RELEASE " /OPT:ICF /MANIFEST:NO" )
        string( APPEND CMAKE_${type}_LINKER_FLAGS_RELWITHDEBINFO " /OPT:ICF /MANIFEST:NO" )
    endforeach()

    # Let cl.exe parallelize builds
    if( KICAD_WIN32_BUILD_PARALLEL_CL_MP )
        string( APPEND CMAKE_CXX_FLAGS " /MP" )
    endif()
endif()

if( KICAD_SCRIPTING_WXPYTHON )
    add_definitions( -DKICAD_SCRIPTING_WXPYTHON )
endif()

if( KICAD_SPICE )
    add_definitions( -DKICAD_SPICE )
endif()

if( KICAD_USE_OCE )
    add_definitions( -DKICAD_USE_OCE )
endif()

if( KICAD_USE_OCC )
    add_definitions( -DKICAD_USE_OCC )
    remove_definitions( -DKICAD_USE_OCE )
    unset( KICAD_USE_OCE CACHE )
endif()

if( KICAD_USE_EGL AND UNIX AND NOT APPLE )
    message( STATUS "Configuring KiCad for the wxGLCanvas EGL backend" )
    add_definitions( -DKICAD_USE_EGL )
elseif( KICAD_USE_EGL )
    message( STATUS "Ignoring KICAD_USE_EGL since not on Linux" )
endif()

# KIFACE_SUFFIX is the file extension used for top level program modules which
# implement the KIFACE interface.  A valid suffix starts with a period '.'.

if( false )
    # This is the eventual situation near milestone C) of modular-kicad blueprint.
    # Until then we use .kiface extension so we don't collide with python DSO.
    set( KIFACE_SUFFIX  ${CMAKE_SHARED_MODULE_SUFFIX} )
else()
    # Temporary situation until we can dovetail the python DSO into the kiface DSO.
    set( KIFACE_SUFFIX  ".kiface" )
endif()

# KIFACE_PREFIX is a basename prefix used for top level program modules which
# implement the KIFACE.
set( KIFACE_PREFIX  "_" )
#message( STATUS "KIFACE_SUFFIX:${KIFACE_SUFFIX}  KIFACE_PREFIX:${KIFACE_PREFIX}" )


#================================================
# Locations for install targets.
#================================================
if( NOT APPLE )
    # Everything without leading / is relative to CMAKE_INSTALL_PREFIX.
    set( KICAD_BIN bin
        CACHE PATH "Location of KiCad binaries." )

    # Do not make these variables "PATH" b/c cmake will truncate them and we need the full path
    if( NOT IS_ABSOLUTE ${CMAKE_INSTALL_DATADIR} )
        set( KICAD_DATA ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/kicad
            CACHE STRING "Location of KiCad data files." )
        set( KICAD_DOCS ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_DATADIR}/doc/kicad
            CACHE STRING "Location of KiCad documentation files." )
    else()
        set( KICAD_DATA ${CMAKE_INSTALL_DATADIR}/kicad
            CACHE STRING "Location of KiCad data files." )
        set( KICAD_DOCS ${CMAKE_INSTALL_DATADIR}/doc/kicad
            CACHE STRING "Location of KiCad documentation files." )
    endif()

    set( KICAD_LIBRARY_DATA ${KICAD_DATA}
        CACHE STRING "Location of KiCad stock EDA library data" )

    if( WIN32 )
        set( KICAD_PLUGINS ${KICAD_BIN}/scripting/plugins
            CACHE PATH "Location of KiCad plugins." )

        set( KICAD_LIB ${KICAD_BIN}
            CACHE PATH "Location of KiCad shared objects" )

        set( KICAD_USER_PLUGIN ${KICAD_BIN}/plugins
            CACHE PATH "Location of KiCad user-loaded plugins" )
    else()
        set( KICAD_PLUGINS ${KICAD_DATA}/plugins
            CACHE PATH "Location of KiCad plugins." )

        set( KICAD_LIB ${CMAKE_INSTALL_LIBDIR}
            CACHE PATH "Location of KiCad shared objects" )

        set( KICAD_USER_PLUGIN ${CMAKE_INSTALL_LIBDIR}/kicad/plugins
            CACHE PATH "Location of KiCad user-loaded plugins" )
    endif()

    set( KICAD_DEMOS ${KICAD_DATA}/demos
        CACHE PATH "Location of KiCad demo files." )
    set( KICAD_TEMPLATE ${KICAD_LIBRARY_DATA}/template
        CACHE PATH "Location of KiCad template files." )
else()
    # everything without leading / is relative to CMAKE_INSTALL_PREFIX.
    # CMAKE_INSTALL_PREFIX is root of .dmg image
    set( KICAD_BIN ${CMAKE_INSTALL_PREFIX}
        CACHE PATH "Location of KiCad binaries." FORCE )

    # some paths to single app bundle
    set( OSX_BUNDLE_MAIN "KiCad.app" )
    set( OSX_BUNDLE_BIN_DIR "Contents/MacOS" )
    set( OSX_BUNDLE_LIB_DIR "Contents/Frameworks" )
    set( OSX_BUNDLE_KIFACE_DIR "Contents/PlugIns" )
    set( OSX_BUNDLE_SUP_DIR "Contents/SharedSupport" )
    set( OSX_BUNDLE_APP_DIR "Contents/Applications" )
    set( OSX_BUNDLE_BUILD_DIR "${CMAKE_BINARY_DIR}/kicad/${OSX_BUNDLE_MAIN}" )
    set( OSX_BUNDLE_BUILD_BIN_DIR "${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_BIN_DIR}" )
    set( OSX_BUNDLE_BUILD_LIB_DIR "${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_LIB_DIR}" )
    set( OSX_BUNDLE_BUILD_KIFACE_DIR "${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_KIFACE_DIR}" )
    set( OSX_BUNDLE_BUILD_PLUGIN_DIR "${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_KIFACE_DIR}" )
    set( OSX_BUNDLE_INSTALL_DIR "${KICAD_BIN}/${OSX_BUNDLE_MAIN}" )
    set( OSX_BUNDLE_INSTALL_BIN_DIR "${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_BIN_DIR}" )
    set( OSX_BUNDLE_INSTALL_LIB_DIR "${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_LIB_DIR}" )
    set( OSX_BUNDLE_INSTALL_KIFACE_DIR "${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_KIFACE_DIR}" )
    set( OSX_BUNDLE_INSTALL_PLUGIN_DIR "${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_KIFACE_DIR}" )

    # everything provided with the application bundle goes into
    # KiCad.app/Contents/SharedSupport => accessible via GetDataDir()
    # everything else to the .dmg image
    set( KICAD_DATA ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_SUP_DIR}
        CACHE PATH "Location of KiCad data files." FORCE )
    set( KICAD_LIB ${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_LIB_DIR}
        CACHE PATH "Location of KiCad shared objects" FORCE )
    set( KICAD_USER_PLUGIN ${OSX_BUNDLE_INSTALL_PLUGIN_DIR}
        CACHE PATH "Location of KiCad user-loaded plugins" FORCE )
    set( KICAD_TEMPLATE ${KICAD_DATA}/template
        CACHE PATH "Location of KiCad template files." FORCE )
    set( KICAD_PLUGINS ${KICAD_DATA}/plugins
        CACHE PATH "Location of KiCad plugins." FORCE )
    set( KICAD_DOCS doc
        CACHE PATH "Location of KiCad documentation files." FORCE )
    set( KICAD_DEMOS demos
        CACHE PATH "Location of KiCad demo files." FORCE )

    # RPATH settings for building shared libraries
    set( CMAKE_MACOSX_RPATH FALSE )

    # Override default paths for fixup_bundle
    set( OSX_BUNDLE_OVERRIDE_PATHS "
        function( gp_item_default_embedded_path_override item default_embedded_path_var )
            # by default, embed things right next to the main bundle executable:
            set( path \"@executable_path/../../Contents/MacOS\" )
            set( overridden 0 )

            # embed .dylibs right next to the main bundle executable:
            if( item MATCHES \"\\\\.dylib$\" )
                set( path \"@executable_path/../Frameworks\" )
                set( overridden 1 )
            endif()

            set( \${default_embedded_path_var} \"\${path}\" PARENT_SCOPE )
        endfunction(gp_item_default_embedded_path_override)

        # If `BU_CHMOD_BUNDLE_ITEMS` is not set, `install_name_tool` will fail to re-write some
        # loader paths due to lack of writable permissions if the build dependencies were installed
        # by brew (or didn't have writable permissions)
        set ( BU_CHMOD_BUNDLE_ITEMS ON )
        "
        )
endif()

mark_as_advanced( KICAD_BIN
    KICAD_PLUGINS
    KICAD_USER_PLUGIN
    KICAD_LIB
    KICAD_DATA
    KICAD_LIBRARY_DATA
    KICAD_DOCS
    KICAD_DEMOS
    KICAD_TEMPLATE )

include( Functions )

include( ExternalProject )

#================================================
# Find libraries that are needed to build KiCad.
#================================================
include( CheckFindPackageResult )

if( KICAD_BUILD_I18N )
    find_package( Gettext REQUIRED )
endif()

#
# Find OpenGL library, required
#
set( OpenGL_GL_PREFERENCE "LEGACY" )     # CMake 3.11+ setting; see 'cmake --help-policy CMP0072'
find_package( OpenGL REQUIRED )

#
# Find GLEW library, required
#
# The EGL canvas on GTK requires the use of a GLEW version compiled with an EGL flag.
# The one built in the thirdparty directory has the flag for EGL set, so we use it unless told
# otherwise. Then we search for the system GLEW version and use that instead.
#
if( KICAD_USE_EGL AND KICAD_USE_BUNDLED_GLEW AND UNIX AND NOT APPLE )
    if( OpenGL_EGL_FOUND )
        message( STATUS "Found OpenGL EGL library: ${OPENGL_egl_LIBRARY}" )
    else()
        message( FATAL_ERROR "OpenGL EGL library not found" )
    endif()

    # Add the custom GLEW target
    add_subdirectory( thirdparty/glew )

    # Set the standard package variables to point to our custom target to mimic the system version.
    set( GLEW_LIBRARIES glew )
    set( GLEW_FOUND TRUE )
    include_directories( SYSTEM $<TARGET_PROPERTY:glew,INCLUDE_DIRECTORIES> )
else()
    find_package( GLEW REQUIRED )
    check_find_package_result( GLEW_FOUND "GLEW" )
    include_directories( SYSTEM ${GLEW_INCLUDE_DIR} )
endif()

#
# Find GLM library, required
#
find_package( GLM 0.9.8 REQUIRED )
add_definitions( -DGLM_FORCE_CTOR_INIT )
include_directories( SYSTEM ${GLM_INCLUDE_DIR} )

#
# Find zlib library, required
#
find_package(ZLIB REQUIRED)
check_find_package_result( ZLIB_FOUND "ZLIB" )
include_directories( SYSTEM ${ZLIB_INCLUDE_DIRS} )

#
# Find CURL library, required
#
find_package( CURL REQUIRED )
include_directories( SYSTEM ${CURL_INCLUDE_DIRS} )

#
# Find Cairo library, required
#
find_package( Cairo 1.12 REQUIRED )
include_directories( SYSTEM ${CAIRO_INCLUDE_DIR} )

find_package( Pixman 0.30 REQUIRED )
include_directories( SYSTEM ${PIXMAN_INCLUDE_DIR} )

#
# Find Boost headers, required.
find_package( Boost 1.59.0 REQUIRED )
include_directories( SYSTEM ${Boost_INCLUDE_DIR} )


# Include MinGW resource compiler.
include( MinGWResourceCompiler )

if( WIN32 )
    # we need the gdiplus library for cairo printing on windows
    set( GDI_PLUS_LIBRARIES gdiplus )
endif()

# Find ngspice library, required for integrated circuit simulator
if( KICAD_SPICE )
    find_package( ngspice REQUIRED )
endif()

# Find OpenCascade Community Edition, required for STEP plugin and tools
if( KICAD_USE_OCE )
    set( OCC_LIBRARIES TKBinXCAF TKPCAF TKSTEP TKXDESTEP TKIGES TKXDEIGES )

    find_package( OCE 0.18 REQUIRED ${OCC_LIBRARIES} )
    include_directories( SYSTEM ${OCE_INCLUDE_DIRS} )

elseif( KICAD_USE_OCC )
    find_package(OpenCASCADE)
    if( NOT OCC_FOUND )
        MESSAGE( FATAL_ERROR "================================================================\n"
                             " KICAD_USE_OCC=True but OpenCASCADE was not found!\n"
                             "================================================================\n")
    endif()
    if( OCC_VERSION_STRING VERSION_LESS 6.9.0 )
        MESSAGE( FATAL_ERROR "================================================================\n"
                             "OpenCASCADE version ${OCC_VERSION_STRING} was found.\n"
                             "  KiCad requires a minimum version of 6.9.0\n"
                             "================================================================\n")
    endif()
    include_directories( SYSTEM ${OCC_INCLUDE_DIR} )
endif()

# Assist with header file searching optimization:
# INC_BEFORE and INC_AFTER are two lists which go at the front and back of the
# header file search lists, respectively.
# INC_BEFORE is for use with "include_directories( BEFORE ...)", which _reverses_
# the order during insertion.  (So put first wanted last, which is
# ${CMAKE_SOURCE_DIR/include.)  Use '.' for current source dir since
# we don't want expansion here and now, which would happen if using ${CMAKE_CURRENT_SOURCE_DIR}.
# Instead we use '.' which is applicable to any source directory below here as a result of
# this lack of expansion.
set( INC_BEFORE
    .
    ${CMAKE_SOURCE_DIR}/include
    )

set( INC_AFTER
    ${CMAKE_BINARY_DIR}
    )

#
# Find Python and other scripting resources
#

# SWIG 3.0 or later require for C++11 support.
find_package( SWIG 3.0 REQUIRED )
include( ${SWIG_USE_FILE} )

# pybind11 is header-only, so include the subdir
add_subdirectory(thirdparty/pybind11)

# Make sure that we get our pybind11 and not the system pybind11 (ours is patched to work with wx)
include_directories( BEFORE SYSTEM ${PYBIND11_INCLUDE_DIR} )

set( PythonInterp_FIND_VERSION 3.6 )
set( PythonLibs_FIND_VERSION 3.6 )

find_package( PythonInterp )

check_find_package_result( PYTHONINTERP_FOUND "Python Interpreter" )

# Get the correct Python site package install path from the Python interpreter found by
# FindPythonInterp unless the user specifically defined a custom path.
if( NOT PYTHON_SITE_PACKAGE_PATH )
    execute_process( COMMAND ${PYTHON_EXECUTABLE} -c "import distutils.sysconfig;print(\"%s\"%distutils.sysconfig.get_python_lib(plat_specific=0, standard_lib=0, prefix=''))"
        OUTPUT_VARIABLE PYTHON_SITE_PACKAGE_PATH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        )

    if( NOT PYTHON_SITE_PACKAGE_PATH )
        message( FATAL_ERROR "Error occurred while attempting to find the Python site library path." )
    endif()
endif()

if( APPLE )
    set( OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR ${OSX_BUNDLE_LIB_DIR}/Python.framework/Versions/${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/lib/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}/site-packages)
    set( OSX_BUNDLE_INSTALL_PYTHON_SITE_PACKAGES_DIR "${OSX_BUNDLE_INSTALL_DIR}/${OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR}")
    set( PYTHON_DEST "${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR}"
        CACHE PATH "Python module install path."
        )
    # change to add_compile_definitions() after minimum required CMake version is 3.12
    set_property( DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR="${OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR}")
elseif( VCPKG_TOOLCHAIN )
    set( PYTHON_DEST "${CMAKE_INSTALL_PREFIX}/bin/Lib/site-packages"
        CACHE PATH "Python module install path."
        )
else()
    set( PYTHON_DEST "${PYTHON_SITE_PACKAGE_PATH}"
        CACHE PATH "Python module install path."
        )
endif()
message( STATUS "Python module install path: ${PYTHON_DEST}" )

find_package( PythonLibs 3.6 REQUIRED )

# Infrequently needed headers go at end of search paths, append to INC_AFTER which
# although is used for all components, should be a harmless hit for something like eeschema
# so long as unused search paths are at the end like this.
set( INC_AFTER ${INC_AFTER} ${PYTHON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/scripting )


if( KICAD_SCRIPTING_WXPYTHON )
    # Find the wxPython installation
    find_package( wxPython REQUIRED )

    if( WXPYTHON_VERSION VERSION_LESS 4.0.0 )
        message( FATAL_ERROR "wxPython Phoenix is required" )
    endif()
endif()

# GTK3 is required on Linux
if( UNIX AND NOT APPLE AND KICAD_SCRIPTING_WXPYTHON )
    if( NOT "${WXPYTHON_TOOLKIT}" STREQUAL "gtk3" )
        message( FATAL_ERROR "GTK3-based wxPython/Phoenix toolkit is required.")
    endif()
endif()

message( STATUS "Found ${WXPYTHON_FLAVOR} "
    "${WXPYTHON_VERSION}/${WXPYTHON_TOOLKIT} "
    "(wxWidgets ${WXPYTHON_WXVERSION})" )

#
# Find wxWidgets library, required
#

# Turn on wxWidgets compatibility mode for some classes
add_definitions( -DWX_COMPATIBILITY )


# Check if '--toolkit=xxx' option has been passed
string( REGEX MATCH "--toolkit=([a-zA-Z0-9]+)"
    WXWIDGETS_REQUESTED_TOOLKIT "${wxWidgets_CONFIG_OPTIONS}" )

if( WXWIDGETS_REQUESTED_TOOLKIT
        AND NOT WXWIDGETS_REQUESTED_TOOLKIT STREQUAL "--toolkit=${WXPYTHON_TOOLKIT}" )
    message( WARNING "wxWidgets and wxPython must be based on the same toolkit.\n"
        "It will be fixed automatically if you skip the '--toolkit=xxx' "
        "wxWidgets_CONFIG_OPTIONS parameter.")
elseif( UNIX AND NOT APPLE )
    # Force the use of GTK3 on Linux
    set( wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} "--toolkit=gtk3" )
else()
    # Use the same toolkit as wxPython otherwise there will be a symbol conflict
    set( wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} "--toolkit=${WXPYTHON_TOOLKIT}" )
endif()

# Require the same wxWidgets version as is used by wxPython
set( wxWidgets_REQ_VERSION ${WXPYTHON_WXVERSION} )

# See line 49 of CMakeModules/FindwxWidgets.cmake
set( wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --static=no )

find_package( wxWidgets ${wxWidgets_REQ_VERSION} COMPONENTS gl aui adv html core net base propgrid xml stc REQUIRED )

# Include wxWidgets macros.
include( ${wxWidgets_USE_FILE} )

if( MINGW )
    # This needs to be on a separate line to protect against a broken FindWxWidgets.cmake in vcpkg
    if( ${wxWidgets_VERSION_STRING} VERSION_LESS 3.1 )
        # Work around a bug in wx < 3.1 -- when wx itself is compiled with
        # compiler extensions enabled, it assumes these are also available for
        # applications.
        add_definitions( -U__STRICT_ANSI__ )
    endif()
endif()

if( APPLE )
    # Remove app bundles in ${KICAD_BIN} before installing anything new.
    # Must be defined before all includes so that it is executed first.
    install( CODE "
            message( STATUS \"Removing existing application bundles...\" )
            # Remove links to standalone apps
            file( REMOVE \"${KICAD_BIN}/Bitmap2Component.app\" )
            file( REMOVE \"${KICAD_BIN}/Eeschema.app\" )
            file( REMOVE \"${KICAD_BIN}/GerbView.app\" )
            file( REMOVE \"${KICAD_BIN}/PCB Calculator.app\" )
            file( REMOVE \"${KICAD_BIN}/Pcbnew.app\" )
            file( REMOVE \"${KICAD_BIN}/Page Layout Editor.app\" )
            # Remove main bundle
            file( REMOVE_RECURSE ${KICAD_BIN}/${OSX_BUNDLE_MAIN} )
        " COMPONENT Runtime
        )
endif()

#================================================
# Add the documentation
#================================================
find_package( Doxygen )

add_subdirectory( Documentation )

# Generate config.h.
configure_file( ${PROJECT_SOURCE_DIR}/CMakeModules/config.h.cmake
    ${CMAKE_BINARY_DIR}/config.h )

###
# Generate Map file
###
if( KICAD_MAKE_LINK_MAPS )
    # Currently only works on linux/gcc
    if( UNIX AND NOT APPLE )
        set( MAKE_LINK_MAPS true )
    else()
        set( MAKE_LINK_MAPS false )
    endif()
endif()

#================================================
# "make uninstall" rules
#================================================
configure_file(
    "${CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY )

add_custom_target( uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" )


#================================================
# Installation
#================================================

###
# Install scripts
###
if( UNIX AND NOT APPLE )
    install( DIRECTORY scripts
        DESTINATION ${KICAD_DOCS}
        COMPONENT resources
        PATTERN "*.bat" EXCLUDE
        )
endif()


include( CTest )
enable_testing()

if( UNIX AND NOT APPLE )

    # Create a *.deb file:
    set( CPACK_GENERATOR "DEB" )
    set( CPACK_DEBIAN_PACKAGE_MAINTAINER "http://launchpad.net/kicad" )

    set( CPACK_PACKAGE_VERSION_MAJOR 1 )
    set( CPACK_PACKAGE_VERSION_MINOR 0 )
    set( CPACK_PACKAGE_VERSION_PATCH 0 )
    #set( CPACK_PACKAGE_CONTACT Firstname Lastname <email@company.com> )
    set( CPACK_PACKAGE_DESCRIPTION_SUMMARY "KiCad built by CMake build system." )

    include( CPack )

endif()

#================================================
# Let CMake look in these directories for nested
# 'CMakeLists.txt' files to process
#================================================

# Binaries ( CMake targets )
add_subdirectory( resources )
add_subdirectory( thirdparty )
add_subdirectory( bitmaps_png )
add_subdirectory( libs )
add_subdirectory( scripting )
add_subdirectory( common )
add_subdirectory( 3d-viewer )
add_subdirectory( eeschema )
add_subdirectory( gerbview )
add_subdirectory( pcbnew )
add_subdirectory( pagelayout_editor )
add_subdirectory( bitmap2component )
add_subdirectory( pcb_calculator )
add_subdirectory( plugins )             # 3D plugins must be built before kicad
add_subdirectory( cvpcb )               # must be after pcbnew
add_subdirectory( kicad )               # should follow pcbnew, eeschema
add_subdirectory( tools )
add_subdirectory( utils )
add_subdirectory( qa )

# Demos
if( KICAD_INSTALL_DEMOS )
    add_subdirectory( demos )
endif ( KICAD_INSTALL_DEMOS )

# I18n Translations
if( KICAD_BUILD_I18N )
    add_subdirectory( translation )
endif()

add_subdirectory( template )
