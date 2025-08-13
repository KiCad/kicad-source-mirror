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

cmake_minimum_required( VERSION 3.22 FATAL_ERROR )

# Generate DEPFILES without transforming relative paths
cmake_policy( SET CMP0116 OLD )

# Needed to allow the doxygen-python process that depends on swig generation running twice to occur
# The current structure requires the add_custom_command to get called twice, the NEW behavior deduplicates it
cmake_policy( SET CMP0113 OLD )

#  Enable CMAKE_MSVC_DEBUG_INFORMATION_FORMAT to override the cmake pdb flags easily
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
endif (POLICY CMP0141)

# Default to CMAKE_BUILD_TYPE = Release unless overridden on command line
# http://www.cmake.org/pipermail/cmake/2008-September/023808.html
if( DEFINED CMAKE_BUILD_TYPE )
    set( CMAKE_BUILD_TYPE ${CMAKE_BUILD_TYPE} CACHE STRING "Set to either \"Release\" or \"Debug\"" )
else()
    set( CMAKE_BUILD_TYPE Release CACHE STRING "Set to either \"Release\" or \"Debug\"" )
endif()

project( kicad )

# Create a default build type for our QA that doesn't include `NDEBUG`
set(CMAKE_CXX_FLAGS_QABUILD "-Os -g1 -ggdb1")

include( GNUInstallDirs )
include( CMakeDependentOption )
include( CheckCXXCompilerFlag )

# Output compile_commands.json for various LSP and other users
set( CMAKE_EXPORT_COMPILE_COMMANDS ON )

# Path to KiCad's CMake modules.
set( KICAD_CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/cmake" )
set( CMAKE_MODULE_PATH       "${KICAD_CMAKE_MODULE_PATH}" )

include( ConfigurePlatform )

if( WIN32 )
    # Without WIN32_LEAN_AND_MEAN defined including windows.h pulls in legacy
    # winsocks.h which conflicts with winsocks2.h used by modern libraries such
    # as curl.
    # There is no harm in defining this globally, at worst some additional
    # headers have to be included explicitly.
    add_compile_definitions( WIN32_LEAN_AND_MEAN )
endif()

if( APPLE )
    # Apple, in their infinite wisdom, has decided to mark OpenGL as deprecated.
    # Luckily we can silence warnings about its deprecation.
    add_compile_definitions( GL_SILENCE_DEPRECATION=1 )
endif()

# Create a flag to identify the non-Apple unix systems
if( UNIX AND NOT APPLE )
    set( UNIX_NOT_APPLE ON )
else()
    set( UNIX_NOT_APPLE OFF )
endif()

if( APPLE OR WIN32 )
    set( APPLE_OR_WIN32 ON )
else()
    set( APPLE_OR_WIN32 OFF )
endif()

#
# KiCad build options should be added below.
#
# If you add a new build option, please add its state to the
# OnCopyVersionInfo() function in common/dialog_about/dialog_about.cpp
# so that build option settings can be included in bug reports.
#

####################################
# Feature flags
####################################
option( KICAD_SPICE_QA
    "Build software Quality assurance unit tests for spice (default ON)"
    ON )

option( KICAD_USE_SENTRY
    "Build KiCad with support for sentry app metrics"
    OFF )

option( KICAD_SIGNAL_INTEGRITY
    "Build tools for signal integrity analysis ( default ON )"
    ON )

option( KICAD_BUILD_I18N
    "Build the translation libraries."
    OFF )

option( KICAD_BUILD_QA_TESTS
    "Build software Quality assurance unit tests (default ON)"
    ON )

option( KICAD_SCRIPTING_WXPYTHON
    "Build wxPython implementation for wx interface building in Python and py.shell (default ON)."
    ON )
mark_as_advanced( KICAD_SCRIPTING_WXPYTHON )

option( KICAD_UPDATE_CHECK
    "Build in update check"
    ON )

cmake_dependent_option( KICAD_USE_BUNDLED_GLEW
    "Use the bundled version of GLEW - only available when KICAD_USE_EGL is set"
    ON "KICAD_USE_EGL"
    OFF )

cmake_dependent_option( KICAD_WAYLAND
    "Support Wayland features"
    ON "KICAD_USE_EGL"
    OFF )

# Options to build with flatpak platform defaults (only on Linux)
cmake_dependent_option( KICAD_BUILD_FLATPAK
    "Build KiCad with metadata, launchers, icons, and MIME info following flatpak conventions"
    OFF "UNIX_NOT_APPLE"
    OFF )

cmake_dependent_option( KICAD_BUILD_NIGHTLY_FLATPAK
    "Build a Nightly flatpak (requires KICAD_BUILD_FLATPAK=ON)"
    OFF "UNIX_NOT_APPLE"
    OFF )

cmake_dependent_option( KICAD_WIN32_DPI_AWARE
    "Turn on DPI awareness for Windows builds only"
    OFF "WIN32"
    OFF )

cmake_dependent_option( KICAD_WIN32_VERIFY_CODESIGN
    "When enabled, verifies the code signing signature of certain DLLs loaded during runtime."
    OFF "WIN32"
    OFF )

# On Windows, binaries created by link option -g3 are very large (more than 1Gb for pcbnew,
# and more than 3Gb for the full kicad suite)
# This option create binaries using link option -g1 that create much smaller files, but
# there are less info in debug (but the file names and line numbers are available)
cmake_dependent_option( KICAD_BUILD_SMALL_DEBUG_FILES
    "Create smaller binaries in debug builds."
    OFF "NOT MSVC"
    OFF )

cmake_dependent_option( KICAD_APPLE_MAKE_RELOCATEABLE_BUNDLE
    "On macOS, post-process the build to make it relocateable (default ON)"
    ON "APPLE" OFF )

# Old versions of CMake and/or protobuf can require the FindProtobuf.cmake,
# but modern platforms require this to be off
option( KICAD_USE_CMAKE_FINDPROTOBUF "Use FindProtobuf provided by CMake" OFF )
mark_as_advanced( KICAD_USE_CMAKE_FINDPROTOBUF )

####################################
# Installation options
####################################
option( KICAD_INSTALL_DEMOS
    "Install KiCad demos and examples (default ON)"
    ON )

option( KICAD_I18N_UNIX_STRICT_PATH
    "Use standard Unix locale lookup path (default OFF)."
    OFF )

cmake_dependent_option( KICAD_WIN32_INSTALL_PDBS
    "Installs debug pdb to the bin folder, DO NOT USE (NOR REQUIRED) FOR DEBUGGING THE SOURCE ON MSVC. This is purely for release building.
    MSVC can find the PDBs without this."
    OFF "WIN32"
    OFF )

####################################
# Compiler options/tools
####################################

# Not supported by all platforms (for instance mingw)
option( KICAD_SANITIZE_ADDRESS
    "Build KiCad with address sanitizer options. WARNING: Not compatible with gold linker"
    OFF )

# Not supported by all platforms (for instance mingw)
option( KICAD_SANITIZE_THREADS
    "Build KiCad with thread sanitizer options. WARNING: Not compatible with gold linker"
    OFF )

# Enable additional valgrind instrumentation for stack tracking in libcontext
option( KICAD_USE_VALGRIND
    "Build KiCad with valgrind stack tracking enabled."
    OFF )

option( KICAD_STDLIB_DEBUG
    "Build KiCad with libstdc++ debug flags enabled."
    OFF )

option( KICAD_STDLIB_LIGHT_DEBUG
    "Build KiCad with libstdc++ with -Wp,-D_GLIBCXX_ASSERTIONS flag enabled. Not as intrusive as KICAD_STDLIB_DEBUG"
    OFF )

cmake_dependent_option( KICAD_WIN32_BUILD_PARALLEL_CL_MP
    "Build in parallel using the /MP compiler option (default OFF for safety reasons)"
    OFF "WIN32"
    OFF )

cmake_dependent_option( KICAD_WIN32_LTCG
    "Enable Whole Program Optimization"
    OFF "WIN32"
    OFF )

option( KICAD_USE_PCH
    "Enable precompiled header support"
    ON )

# Advanced option to make link maps (only available on linux)
cmake_dependent_option( KICAD_MAKE_LINK_MAPS
    "Create link maps for artifacts"
    OFF "UNIX_NOT_APPLE"
    OFF )
mark_as_advanced( KICAD_MAKE_LINK_MAPS )

set( KICAD_INIT_VARIABLES "Default" CACHE STRING "Specify how compiler should initialize variables" )
set_property( CACHE KICAD_INIT_VARIABLES PROPERTY STRINGS
              "Default"         # Equivalent to pattern in debug mode (but not when Valgrind is used), zero in release mode
              "Off"             # Don't initialize anything
              "Zero"            # Initialize trivial variables to 0
              "Pattern" )       # Initialize trivial variables to a non-zero pattern to flag them (usually 0xAA...)

####################################
# Debugging/profiling
####################################
option( KICAD_DRC_PROTO
    "Build the DRC prototype QA tool"
    OFF )

option( KICAD_BUILD_PEGTL_DEBUG_TOOL
    "Build the PEGTL parser debugging/playground QA tool"
    OFF )

option( KICAD_BUILD_PNS_DEBUG_TOOL
    "Build the P&S debugging/playground QA tool"
    OFF )

option( KICAD_GAL_PROFILE
    "Enable profiling info for GAL"
    OFF )

option( KICAD_IPC_API
    "Enable IPC API"
    ON )

option( KICAD_IDF_TOOLS
    "Build additional idf tools"
    ON )

# Global setting: exports are explicit
set( CMAKE_CXX_VISIBILITY_PRESET "hidden" )
set( CMAKE_VISIBILITY_INLINES_HIDDEN ON )

# Global setting: build everything position independent
set( CMAKE_POSITION_INDEPENDENT_CODE ON )

# Global setting: Use C++20
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Pass defines into KiCad code based on the build options set
add_compile_definitions( $<$<BOOL:${KICAD_SCRIPTING_WXPYTHON}>:KICAD_SCRIPTING_WXPYTHON> )
add_compile_definitions( $<$<BOOL:${KICAD_SIGNAL_INTEGRITY}>:KICAD_SIGNAL_INTEGRITY> )

add_compile_definitions( $<$<BOOL:${KICAD_USE_VALGRIND}>:KICAD_USE_VALGRIND> )
add_compile_definitions( $<$<BOOL:${KICAD_GAL_PROFILE}>:KICAD_GAL_PROFILE> )
add_compile_definitions( $<$<BOOL:${KICAD_WIN32_VERIFY_CODESIGN}>:KICAD_WIN32_VERIFY_CODESIGN> )
add_compile_definitions( $<$<BOOL:${KICAD_UPDATE_CHECK}>:KICAD_UPDATE_CHECK> )

if( KICAD_IPC_API )
    add_definitions( -DKICAD_IPC_API )
endif()

# Ensure DEBUG is defined for all platforms in Debug builds
add_compile_definitions( $<$<CONFIG:Debug>:DEBUG> )

# Add option to add user directories for linker, if any
link_directories( ${LINK_DIRECTORIES_PATH} )

set( KICAD_CONFIG_DIR "kicad" CACHE STRING "Location of user specific KiCad config files" )
mark_as_advanced( KICAD_CONFIG_DIR )
add_compile_definitions( KICAD_CONFIG_DIR=${KICAD_CONFIG_DIR} )

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
include( ${KICAD_CMAKE_MODULE_PATH}/Warnings.cmake )

if( WIN32 )
    # define UNICODE and_UNICODE definition on Windows. KiCad uses unicode.
    # Both definitions are required
    add_compile_definitions( UNICODE _UNICODE)

    # In fully standards-compliant mode, M_PI et al. are defined only if
    # _USE_MATH_DEFINES is set.
    add_compile_definitions( _USE_MATH_DEFINES )

    add_compile_definitions( $<$<BOOL:${KICAD_WIN32_DPI_AWARE}>:KICAD_WIN32_DPI_AWARE=1> )
endif()

add_compile_definitions( $<$<BOOL:${KICAD_BUILD_ARCH_X86}>:KICAD_BUILD_ARCH_X86> )
add_compile_definitions( $<$<BOOL:${KICAD_BUILD_ARCH_X64}>:KICAD_BUILD_ARCH_X64> )
add_compile_definitions( $<$<BOOL:${KICAD_BUILD_ARCH_ARM}>:KICAD_BUILD_ARCH_ARM> )
add_compile_definitions( $<$<BOOL:${KICAD_BUILD_ARCH_ARM64}>:KICAD_BUILD_ARCH_ARM64> )

#================================================
# Provide access to CCACHE
#================================================
if (USE_CCACHE)
  find_program(CCACHE_FOUND ccache)
  if(CCACHE_FOUND)
    # Set ccache on the actual lang
    list(PREPEND CMAKE_C_COMPILER_LAUNCHER ${CCACHE_FOUND})
    list(PREPEND CMAKE_CXX_COMPILER_LAUNCHER ${CCACHE_FOUND})

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
    list(APPEND CMAKE_C_COMPILER_LAUNCHER ${DISTCC_FOUND})
    list(APPEND CMAKE_CXX_COMPILER_LAUNCHER ${DISTCC_FOUND})

    message(STATUS "Using ${DISTCC_FOUND} for distributed build.")
  else(DISTCC_FOUND)
    message(INFO "Distcc was requested but not found.")
  endif(DISTCC_FOUND)
endif(USE_DISTCC)


#================================================
# Setup compiler-specific flags
#================================================
if( CMAKE_COMPILER_IS_GNUCXX )
    # Set 32-bit flag for GCC to prevent excess precision (not needed for Clang)
    if( CMAKE_SIZEOF_VOID_P EQUAL 4 )
        set( CMAKE_CXX_FLAGS "-ffloat-store ${CMAKE_CXX_FLAGS}" )
    endif()
endif( CMAKE_COMPILER_IS_GNUCXX )

if( CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    # Clang needs this flag or wxStrings, etc. won't be viewable in the debugger
    set( CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fstandalone-debug" )
    set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fstandalone-debug" )
endif()

if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    # Link flags in Debug: -g1 and -ggdb1 or -g1 and -ggdb3 can be used.
    # Level 1 produces minimal information, enough for making basic backtraces.
    # This includes descriptions of functions and external variables, and line number tables,
    # but no information about local variables.
    # Level 3 includes full information, but binaries are much larger.
    if( KICAD_BUILD_SMALL_DEBUG_FILES )
        set( CMAKE_C_FLAGS_DEBUG   "-g1 -ggdb1" )
        set( CMAKE_CXX_FLAGS_DEBUG "-g1 -ggdb1" )
    else()
        set( CMAKE_C_FLAGS_DEBUG   "-g3 -ggdb3" )
        set( CMAKE_CXX_FLAGS_DEBUG "-g3 -ggdb3" )
    endif()

    check_cxx_compiler_flag( "-ftrivial-auto-var-init=pattern" COMPILER_SUPPORTS_TRIVIAL_PATTERN_INIT )
    if( COMPILER_SUPPORTS_TRIVIAL_PATTERN_INIT )
        set( VAR_PATTERN_INIT_FLAGS -ftrivial-auto-var-init=pattern )
    endif()

    check_cxx_compiler_flag( "-ftrivial-auto-var-init=zero" COMPILER_SUPPORTS_TRIVIAL_ZERO_INIT )
    if( COMPILER_SUPPORTS_TRIVIAL_ZERO_INIT )
        set( VAR_ZERO_INIT_FLAGS -ftrivial-auto-var-init=zero )
    else()
        # Clang 15 and below required a special flag to enable the zero-init flag, so try that
        check_cxx_compiler_flag( "-ftrivial-auto-var-init=zero -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang"
                                 COMPILER_REQUIRES_ENABLE_TRIVIAL_ZERO_INIT )
        if( COMPILER_REQUIRES_ENABLE_TRIVIAL_ZERO_INIT )
            set( VAR_ZERO_INIT_FLAGS -ftrivial-auto-var-init=zero -enable-trivial-auto-var-init-zero-knowing-it-will-be-removed-from-clang )
        endif()
    endif()

    if( NOT KICAD_INIT_VARIABLES STREQUAL "Off" )
        if( KICAD_INIT_VARIABLES STREQUAL "Default" )
            # Disable when using valgrind to preserve reporting of uninitialized variables
            if( NOT KICAD_USE_VALGRIND )
                # Use pattern init in debug mode only, use zero init every other time
                if( CMAKE_BUILD_TYPE STREQUAL "Debug" )
                    set( VAR_INIT_FLAGS ${VAR_PATTERN_INIT_FLAGS} )
                else()
                    set( VAR_INIT_FLAGS ${VAR_ZERO_INIT_FLAGS} )
                endif()
            endif()
        elseif( KICAD_INIT_VARIABLES STREQUAL "Zero" )
            set( VAR_INIT_FLAGS ${VAR_ZERO_INIT_FLAGS} )
        elseif( KICAD_INIT_VARIABLES STREQUAL "Pattern" )
            set( VAR_INIT_FLAGS ${VAR_PATTERN_INIT_FLAGS} )
        endif()

        if( VAR_INIT_FLAGS )
            add_compile_options( ${VAR_INIT_FLAGS} )
            message( STATUS "Initializing trivial variables (${VAR_INIT_FLAGS})" )
        endif()
    endif()

    if( KICAD_SANITIZE_ADDRESS )
        add_compile_definitions( KICAD_SANITIZE_ADDRESS )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D_GLIBCXX_SANITIZE_VECTOR -fsanitize=address -fsanitize-recover=address -fno-optimize-sibling-calls -fsanitize-address-use-after-scope -fno-omit-frame-pointer" )

        # ASAN shouldn't be used with these options (https://github.com/google/sanitizers/wiki/AddressSanitizer#faq)
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-stack-protector -U_FORTIFY_SOURCE" )
    endif()

    if( KICAD_SANITIZE_THREADS )
        add_compile_definitions( KICAD_SANITIZE_THREADS )
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
        add_compile_definitions( KICAD_STDLIB_DEBUG )
        set( CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -D_GLIBCXX_DEBUG" )
    elseif( KICAD_STDLIB_LIGHT_DEBUG )
        # useless if KICAD_STDLIB_DEBUG is ON.
        # this option makes some controls to trap out of bound memory access.
        add_compile_definitions( KICAD_STDLIB_LIGHT_DEBUG )
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

        # for some reasons, cmake does do use always a response file to send the list of objects
        # to the archiver, and because this list can be very long, and can create issue
        # when it is used in a command line, force use of a response file to store it
        set( CMAKE_CXX_USE_RESPONSE_FILE_FOR_OBJECTS 1 )

        # The MinGW compiler can use the microsoft system snprintf as standard and it has a broken
        # API with respect to the C99 standard, so make sure we force it to use its own compliant
        # snprintf
        add_compile_definitions( __USE_MINGW_ANSI_STDIO=1 )

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
    add_compile_definitions( BOOST_ALL_NO_LIB )
    # But allow it for the UUID so that it will link against bcrypt
    add_compile_definitions( BOOST_UUID_FORCE_AUTO_LINK )
    # Disable MSVC's deprecation warnings
    add_compile_definitions( _CRT_SECURE_NO_WARNINGS _CRT_NONSTDC_NO_DEPRECATE _SCL_SECURE_NO_WARNINGS )
    # Hide Windows's min() and max() macros
    add_compile_definitions( NOMINMAX )
    # source and execution charset are UTF-8
    string( APPEND CMAKE_CXX_FLAGS " /utf-8" )

    # cmake 3.25 or higher requried for MSVC, should be fine
    if( USE_CCACHE AND CCACHE_FOUND )
        # Equivalent to /Z7, ensures cmake doesn't fight over the default
        # ccache may encounter external PDB file write errors due to paralleism
        # Using the embedded PDB mode is ok for development use of ccache
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "Embedded")
    else()
        # Equivalent to /Zi, ensures cmake doesn't fight over the default
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "ProgramDatabase")
    endif()

    # /Zc:__cplusplus: define this macro according to C++ standard version
    string( APPEND CMAKE_CXX_FLAGS " /Zc:__cplusplus" )
    # /Zc:inline: enforces C++11 requirement that inline functions must have a definition in the same unit
    string( APPEND CMAKE_CXX_FLAGS " /Zc:inline" )
    # /GF: enable string pooling
    string( APPEND CMAKE_CXX_FLAGS_RELEASE " /GF" )
    string( APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " /GF" )
    # /Gy: Enable Function-Level Linking
    string( APPEND CMAKE_CXX_FLAGS_RELEASE " /Gy" )
    string( APPEND CMAKE_CXX_FLAGS_RELWITHDEBINFO " /Gy" )
    # Avoid fatal error because /GF + swig wrapper exceed standard obj file limits
    string( APPEND CMAKE_CXX_FLAGS " /bigobj" )
    # /permissive-: This puts MSVC into compliance mode, this flag is default on with C++20
    # but we want to turn it on to enable debug performance improvements available under C++17 in MSVC 17.5+
    string( APPEND CMAKE_CXX_FLAGS " /permissive-" )

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

    if( KICAD_SANITIZE_ADDRESS )
        add_definitions( -DKICAD_SANITIZE_ADDRESS )
        add_definitions( -D_DISABLE_VECTOR_ANNOTATION )
        string( APPEND CMAKE_CXX_FLAGS " /fsanitize=address" )
    endif()

    if( KICAD_WIN32_LTCG )
        # we are implementing this manually because CMake's LTCG option is incomplete
        # https://gitlab.kitware.com/cmake/cmake/-/issues/20484
        string( APPEND CMAKE_CXX_FLAGS_RELEASE " /GL" )
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

        if( KICAD_WIN32_LTCG )
            # we are implementing this manually because CMake's LTCG option is incomplete
            # https://gitlab.kitware.com/cmake/cmake/-/issues/20484
            string( APPEND CMAKE_${type}_LINKER_FLAGS_RELEASE " /LTCG" )
        endif()
    endforeach()

    # Let cl.exe parallelize builds
    if( KICAD_WIN32_BUILD_PARALLEL_CL_MP )
        string( APPEND CMAKE_CXX_FLAGS " /MP" )
    endif()

    set( CMAKE_INSTALL_UCRT_LIBRARIES TRUE )
    include( InstallRequiredSystemLibraries )
endif()

# KIFACE_SUFFIX is the file extension used for top level program modules which
# implement the KIFACE interface.  A valid suffix starts with a period '.'.

if( WIN32 )
    # We use .kiface extension so we don't collide with python DSO. (Linux/mac issue?)
    # Windows works fine with the native dll extension
    # which also resolves metadata issues because windows wants to see the dll extension
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
    if( WIN32 )
        set( KICAD_BIN bin
            CACHE PATH "Location of KiCad binaries." )
    else()
        set( KICAD_BIN ${CMAKE_INSTALL_BINDIR}
            CACHE PATH "Location of KiCad binaries." )
    endif()

    # For now, the kifaces are just in the normal bin folder
    set( KICAD_KIFACE ${KICAD_BIN}
        CACHE PATH "Location of KiCad kifaces." )

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

    # Required to run from build folder
    if( PYTHON_FRAMEWORK )
        list(APPEND CMAKE_BUILD_RPATH ${PYTHON_FRAMEWORK} )
    endif()

    # RPATH settings for building shared libraries
    set( CMAKE_MACOSX_RPATH FALSE )
endif()

mark_as_advanced( KICAD_BIN
    KICAD_KIFACE
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
# Find GLM library, required
#
find_package( GLM 0.9.8 REQUIRED )
add_compile_definitions( GLM_FORCE_CTOR_INIT )
include_directories( SYSTEM ${GLM_INCLUDE_DIR} )

#
# Find zlib library, required
#
find_package(ZLIB REQUIRED)

#
# Find Zstd library, required
#
find_package(ZSTD REQUIRED)

#
# Find libcurl, required
#
find_package( CURL REQUIRED )

if( UNIX AND NOT APPLE )
    find_package( SPNAV REQUIRED )
    include_directories( SYSTEM ${SPNAV_INCLUDE_DIR} )

    find_package( Poppler REQUIRED )
    include_directories( SYSTEM ${POPPLER_INCLUDE_DIRS} )
endif()

#
# Find Cairo library, required
#
find_package( Cairo 1.12 REQUIRED )
include_directories( SYSTEM ${CAIRO_INCLUDE_DIR} )

find_package( Pixman 0.30 REQUIRED )
include_directories( SYSTEM ${PIXMAN_INCLUDE_DIR} )

# Find libgit2, required
find_package( libgit2 1.5 REQUIRED )

# Set include directories for libgit
include_directories(${LIBGIT2_INCLUDE_DIRS})

#
# Find Boost headers and libraries, required.
set( BOOST_REQUESTED_COMPONENTS locale )    # locale is required by nanoodbc/database libraries

if( KICAD_BUILD_QA_TESTS )
    # required by the unit test framework
    list( APPEND BOOST_REQUESTED_COMPONENTS unit_test_framework )
endif()

find_package( Boost 1.71.0 REQUIRED COMPONENTS ${BOOST_REQUESTED_COMPONENTS} )

#
# Libraries required for outline font support.
if( MSVC )
# Earlier than 2.11.1 contain a crash bug specific to MSVC built freetype
set( FREETYPE_MIN_VERSION 2.11.1 )
endif()

find_package( Freetype ${FREETYPE_MIN_VERSION} REQUIRED )
include_directories( SYSTEM ${FREETYPE_INCLUDE_DIRS} )

find_package( HarfBuzz REQUIRED )
include_directories( SYSTEM ${HarfBuzz_INCLUDE_DIRS} )

find_package( Fontconfig REQUIRED )

# Include MinGW resource compiler.
include( MinGWResourceCompiler )

# Find ngspice library, required for integrated circuit simulator
find_package( ngspice REQUIRED )

# Find OpenCascade, required for STEP plugin and tools
find_package(OCC)
if( NOT OCC_FOUND )
    MESSAGE( FATAL_ERROR "================================================================\n"
                         " OpenCASCADE was not found!\n"
                         "================================================================\n")
endif()
if( OCC_VERSION_STRING VERSION_LESS 7.5.0 )
    MESSAGE( FATAL_ERROR "================================================================\n"
                         "OpenCASCADE version ${OCC_VERSION_STRING} was found.\n"
                         "  KiCad requires a minimum version of 7.5.0\n"
                         "================================================================\n")
endif()
include_directories( SYSTEM ${OCC_INCLUDE_DIR} )

# Try to find Protobuf with modern approach first, fallback to legacy if needed
set( _protobuf_found FALSE )

if( NOT KICAD_USE_CMAKE_FINDPROTOBUF )
    message( STATUS "Attempting to find Protobuf using CONFIG mode..." )
    find_package( Protobuf QUIET CONFIG )
    if( Protobuf_FOUND )
        set( Protobuf_LIBRARY "protobuf::libprotobuf" )
        set( _protobuf_found TRUE )
        message( STATUS "Found Protobuf using CONFIG mode" )
    else()
        message( STATUS "CONFIG mode failed, falling back to FindProtobuf.cmake" )
        set( KICAD_USE_CMAKE_FINDPROTOBUF ON )
    endif()
endif()

if( NOT _protobuf_found )
    message( STATUS "Using FindProtobuf.cmake to locate Protobuf..." )
    include( FindProtobuf )
    find_package( Protobuf REQUIRED )
    if( Protobuf_FOUND )
        message( STATUS "Found Protobuf using FindProtobuf.cmake" )
    endif()
endif()

if( NOT Protobuf_PROTOC_EXECUTABLE )
    if( NOT KICAD_USE_CMAKE_FINDPROTOBUF)
        set( Protobuf_PROTOC_EXECUTABLE "protobuf::protoc" )
    else()
        MESSAGE( FATAL_ERROR "Protobuf compiler not found. Please install protobuf-compiler." )
    endif()
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

# SWIG 4.0 or later require for proper C++11 support.
find_package( SWIG 4.0 REQUIRED )
include( ${SWIG_USE_FILE} )

set( PythonInterp_FIND_VERSION 3.6 )
set( PythonLibs_FIND_VERSION 3.6 )

find_package( PythonInterp ${PythonInterp_FIND_VERSION} )

check_find_package_result( PYTHONINTERP_FOUND "Python Interpreter" )

# Get the correct Python site package install path from the Python interpreter found by
# FindPythonInterp unless the user specifically defined a custom path.
if( NOT PYTHON_SITE_PACKAGE_PATH )
    execute_process( COMMAND ${PYTHON_EXECUTABLE} -c "import sys, sysconfig
if 'deb_system' in sysconfig.get_scheme_names():
    scheme = 'deb_system'
elif sys.version_info > (3, 10):
    scheme = sysconfig.get_default_scheme()
else:
    scheme = sysconfig._get_default_scheme()
print(\"%s\"%sysconfig.get_path('platlib', vars={'platbase': '.'}, scheme=scheme))"
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

    add_compile_definitions( OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR="${OSX_BUNDLE_PYTHON_SITE_PACKAGES_DIR}" )
elseif( VCPKG_TOOLCHAIN )
    set( PYTHON_DEST "${CMAKE_INSTALL_PREFIX}/bin/Lib/site-packages"
        CACHE PATH "Python module install path."
        )
else()
    set( PYTHON_DEST "${PYTHON_SITE_PACKAGE_PATH}"
        CACHE PATH "Python module install path."
        )

    if( IS_ABSOLUTE ${PYTHON_SITE_PACKAGE_PATH} )
        set( PYTHON_FULL_DEST "${PYTHON_SITE_PACKAGE_PATH}"
            CACHE PATH "Python module full install path."
            )
    else()
        set( PYTHON_FULL_DEST "${CMAKE_INSTALL_PREFIX}/${PYTHON_SITE_PACKAGE_PATH}"
            CACHE PATH "Python module full install path."
            )
    endif()
endif()
message( STATUS "Python module install path: ${PYTHON_DEST}" )

find_package( PythonLibs 3.6 REQUIRED )

# pybind11 is header-only, so include the subdir
add_subdirectory(thirdparty/pybind11)

# Make sure that we get our pybind11 and not the system pybind11 (ours is patched to work with wx)
include_directories( BEFORE SYSTEM ${PYBIND11_INCLUDE_DIR} )

# Infrequently needed headers go at end of search paths, append to INC_AFTER which
# although is used for all components, should be a harmless hit for something like eeschema
# so long as unused search paths are at the end like this.
set( INC_AFTER ${INC_AFTER} ${PYTHON_INCLUDE_DIRS} ${CMAKE_CURRENT_SOURCE_DIR}/scripting )


if( KICAD_SCRIPTING_WXPYTHON )
    # Find the wxPython installation
    if( NOT VCPKG_TOOLCHAIN AND NOT KICAD_BUILD_ARCH_ARM64 )
        find_package( wxPython REQUIRED )

        if( WXPYTHON_VERSION VERSION_LESS 4.0.0 )
            message( FATAL_ERROR "wxPython Phoenix is required" )
        endif()

        if( WXPYTHON_WXVERSION VERSION_LESS 3.2.0 )
            message( FATAL_ERROR "wxWidgets 3.2.0 or greater is required" )
        endif()
    endif()

endif()

# GTK3 is required on Linux
if( UNIX AND NOT APPLE AND KICAD_SCRIPTING_WXPYTHON )
    if( NOT "${WXPYTHON_TOOLKIT}" STREQUAL "gtk3" )
        message( FATAL_ERROR "GTK3-based wxPython/Phoenix toolkit is required.")
    endif()
endif()

if( WXPYTHON_FLAVOR )
    message( STATUS "Found wxPython ${WXPYTHON_FLAVOR} "
        "${WXPYTHON_VERSION}/${WXPYTHON_TOOLKIT} "
        "(wxWidgets ${WXPYTHON_WXVERSION})" )
endif()

#
# Find wxWidgets library, required
#

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
if( KICAD_SCRIPTING_WXPYTHON )
    set( wxWidgets_REQ_VERSION ${WXPYTHON_WXVERSION} )
else()
    set( wxWidgets_REQ_VERSION 3.2.0 )
endif()

# See line 49 of cmake/FindwxWidgets.cmake
set( wxWidgets_CONFIG_OPTIONS ${wxWidgets_CONFIG_OPTIONS} --static=no )

find_package( wxWidgets ${wxWidgets_REQ_VERSION} COMPONENTS gl aui adv html core net base propgrid xml stc richtext webview REQUIRED )

# Include wxWidgets macros.
include( ${wxWidgets_USE_FILE} )

# Detect wxWidgets port being used
if( wxWidgets_FIND_STYLE STREQUAL "win32" )
    string(REGEX MATCH "(msw|qt|gtk)" KICAD_WX_PORT "${WX_CONFIGURATION}")

    if( KICAD_WX_PORT STREQUAL "qt" )
        add_compile_definitions( __WXQT__ )
    elseif( KICAD_WX_PORT STREQUAL "gtk" )
        add_compile_definitions( __WXGTK__ )
    endif()
elseif( _wx_selected_config )
    string(REGEX MATCH "(msw|qt|gtk|osx)" KICAD_WX_PORT "${_wx_selected_config}")
endif()

if( KICAD_WX_PORT )
    message( STATUS "Detected wxWidgets port: ${KICAD_WX_PORT}")
else()
    message( FATAL_ERROR "Unable to detect wxWidgets port")
endif()

message(STATUS "wxUSE_GLCANVAS_EGL=${wxWidgets_GLCANVAS_EGL}")

if( NOT MSVC )
    if( ${wxWidgets_VERSION_STRING} VERSION_LESS 3.2 )
        message( FATAL_ERROR "wxWidgets 3.2.0 or greater is required" )
    endif()
endif()

if( MINGW )
    # This needs to be on a separate line to protect against a broken FindWxWidgets.cmake in vcpkg
    if( ${wxWidgets_VERSION_STRING} VERSION_LESS 3.1 )
        # Work around a bug in wx < 3.1 -- when wx itself is compiled with
        # compiler extensions enabled, it assumes these are also available for
        # applications.
        add_definitions( -U__STRICT_ANSI__ )
    endif()
endif()

#
# Find OpenGL library, required
#
if( wxWidgets_GLCANVAS_EGL EQUAL 1 )
    set( KICAD_USE_EGL ON )
    find_package( OpenGL REQUIRED COMPONENTS OpenGL EGL )
else()
    set( KICAD_USE_EGL OFF )
    set( OpenGL_GL_PREFERENCE "LEGACY" )     # CMake 3.11+ setting; see 'cmake --help-policy CMP0072'
    find_package( OpenGL REQUIRED )
endif()

if( KICAD_USE_EGL )
    message( STATUS "Configuring KiCad for the wxGLCanvas EGL backend" )
    add_compile_definitions( KICAD_USE_EGL )
endif()

if( KICAD_WAYLAND )
    message( STATUS "Configuring KiCad to support Wayland features" )
    add_compile_definitions( KICAD_WAYLAND )
endif()

#
# Find GLEW library, required
#
# The EGL canvas on GTK requires the use of a GLEW version compiled with an EGL flag.
# The one built in the thirdparty directory has the flag for EGL set, so we use it unless told
# otherwise. Then we search for the system GLEW version and use that instead.
#
if( KICAD_USE_EGL AND KICAD_USE_BUNDLED_GLEW )
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


if( KICAD_USE_SENTRY )
    set( KICAD_SENTRY_DSN "" CACHE STRING "The sentry DSN used with sentry integration" )
    if ( KICAD_SENTRY_DSN STREQUAL "")
        message( FATAL_ERROR "Sentry DSN must be configured with KICAD_USE_SENTRY" )
    endif()

    if( WIN32 )
        set( CRASHPAD_WER_ENABLED ON )
    endif()
    add_subdirectory( thirdparty/sentry-native )
    add_compile_definitions( KICAD_USE_SENTRY )
endif()

if( KICAD_IPC_API )
    find_package( nng REQUIRED )
endif()

#================================================
# Add the doxygen target
#================================================
find_package( Doxygen )

add_subdirectory( doxygen )

# Generate config.h.
configure_file( ${PROJECT_SOURCE_DIR}/cmake/config.h.cmake
    ${CMAKE_BINARY_DIR}/config.h )


#================================================
# "make uninstall" rules
#================================================
configure_file(
    "${KICAD_CMAKE_MODULE_PATH}/cmake_uninstall.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake"
    IMMEDIATE @ONLY )

add_custom_target( uninstall
    "${CMAKE_COMMAND}" -P "${CMAKE_CURRENT_BINARY_DIR}/cmake_uninstall.cmake" )


#================================================
# Installation
#================================================

include( CTest )
enable_testing()
list( APPEND CMAKE_CTEST_ARGUMENTS "--output-on-failure" )

#================================================
# Let CMake look in these directories for nested
# 'CMakeLists.txt' files to process
#================================================

# Binaries ( CMake targets )
add_subdirectory( api )
add_subdirectory( resources )
add_subdirectory( thirdparty )
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
add_subdirectory( kicad )               # should follow pcbnew, eeschema, api
add_subdirectory( tools )
add_subdirectory( utils )

if( KICAD_BUILD_QA_TESTS )
    add_subdirectory( qa )
endif()

# Demos
if( KICAD_INSTALL_DEMOS )
    add_subdirectory( demos )
endif ( KICAD_INSTALL_DEMOS )

# I18n Translations
if( KICAD_BUILD_I18N )
    add_subdirectory( translation )
endif()

if( APPLE )
    set( KICAD_OSX_CODESIGN ON
            CACHE BOOL "Sign KiCad.app on macOS" FORCE )
    set( KICAD_OSX_SIGNING_ID "-"
            CACHE STRING "macOS Signing ID, defaults to ad-hoc" FORCE )
    set( KICAD_OSX_SIGNING_USE_SECURE_TIMESTAMP OFF
            CACHE BOOL "When signing on macOS, add a secure timestamp" FORCE )
    set( KICAD_OSX_SIGNING_USE_HARDENED_RUNTIME OFF
            CACHE BOOL "When signing on macOS, use the Hardened Runtime" FORCE )
    set( KICAD_OSX_SIGNING_ENTITLEMENTS_FILE ""
            CACHE FILEPATH "Path to entitlements file for macOS signing" FORCE )

    # We have to sign the app bundle after *all* the files are copied in the bundle
    # otherwise the signature will be invalid.

    # As of CMP0082 (CMake 3.14), the install rules are run in order, so we can just
    # declare it last.

    add_subdirectory( signing )
endif()
