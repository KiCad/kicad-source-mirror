#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2007-2020 Kicad Developers, see AUTHORS.txt for contributors.
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

# Configure warnings for Clang and GCC
if( CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang" )
    # The SWIG-generated files tend to throw a lot of warnings, so
    # we do not add the warnings directly to the flags here but instead
    # keep track of them and add them to the flags later in a controlled manner
    # (that way we can not put any warnings on the SWIG-generated files)
    set( COMPILER_SUPPORTS_WARNINGS TRUE )

    # Establish -Wall early, so specialized relaxations of this may come
    # subsequently on the command line, such as in pcbnew/github/CMakeLists.txt
    set( WARN_FLAGS_C   "-Wall" )
    set( WARN_FLAGS_CXX "-Wall" )


    # Warn about missing override specifiers
    CHECK_CXX_COMPILER_FLAG( "-Wsuggest-override" COMPILER_SUPPORTS_WSUGGEST_OVERRIDE )

    if( COMPILER_SUPPORTS_WSUGGEST_OVERRIDE )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wsuggest-override" )
        message( STATUS "Enabling warning -Wsuggest-override" )
    endif()


    # This is Clang's version of -Wsuggest-override
    CHECK_CXX_COMPILER_FLAG( "-Winconsistent-missing-override" COMPILER_SUPPORTS_WINCONSISTENT_MISSING_OVERRIDE )

    if( COMPILER_SUPPORTS_WINCONSISTENT_MISSING_OVERRIDE )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Winconsistent-missing-override" )

        # Also use this to guard warning removal of the warning inside the code
        set( HAVE_WINCONSISTENT_MISSING_OVERRIDE true )

        message( STATUS "Enabling warning -Winconsistent-missing-override" )
    endif()


    # Warn on duplicated branches
    CHECK_CXX_COMPILER_FLAG( "-Wduplicated-branches" COMPILER_SUPPORTS_WDUPLICATED_BRANCHES )

    if( COMPILER_SUPPORTS_WDUPLICATED_BRANCHES )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wduplicated-branches" )
        message( STATUS "Enabling warning -Wduplicated-branches" )
    endif()


    # Warn on duplicated conditions
    CHECK_CXX_COMPILER_FLAG( "-Wduplicated-cond" COMPILER_SUPPORTS_WDUPLICATED_COND )

    if( COMPILER_SUPPORTS_WDUPLICATED_COND )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wduplicated-cond" )
        message( STATUS "Enabling warning -Wduplicated-cond" )
    endif()


    # Error on variable length arrays (gcc extension)
    CHECK_CXX_COMPILER_FLAG( "-Wvla" COMPILER_SUPPORTS_WVLA )

    if( COMPILER_SUPPORTS_WVLA )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Werror=vla" )
        message( STATUS "Enabling error for -Wvla" )
    endif()


    # Warn on implicit switch fallthrough
    CHECK_CXX_COMPILER_FLAG( "-Wimplicit-fallthrough" COMPILER_SUPPORTS_WIMPLICIT_FALLTHROUGH )

    if( COMPILER_SUPPORTS_WIMPLICIT_FALLTHROUGH )
        if( CMAKE_COMPILER_IS_GNUCXX )
            # GCC level 5 does not allow comments - mirrors the Clang warning
            set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wimplicit-fallthrough=5" )
        else()
            set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wimplicit-fallthrough" )
        endif()

        message( STATUS "Enabling warning -Wimplicit-fallthrough" )
    endif()


    # Error if there is a problem with function returns
    CHECK_CXX_COMPILER_FLAG( "-Wreturn-type" COMPILER_SUPPORTS_WRETURN_TYPE )

    if( COMPILER_SUPPORTS_WRETURN_TYPE )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Werror=return-type" )
        message( STATUS "Enabling error for -Wreturn-type" )
    endif()


    # Warn about shadowed variables
    CHECK_CXX_COMPILER_FLAG( "-Wshadow" COMPILER_SUPPORTS_WSHADOW )

    if( COMPILER_SUPPORTS_WSHADOW )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wshadow" )
        message( STATUS "Enabling warning -Wshadow" )
    endif()


    # Add additional warning flags to avoid signed/unsigned comparison
    CHECK_CXX_COMPILER_FLAG( "-Wsign-compare" COMPILER_SUPPORTS_WSIGN )

    if( COMPILER_SUPPORTS_WSIGN )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wsign-compare" )
        message( STATUS "Enabling warning -Wsign-compare" )
    endif()


    # Warn about missing initializers in construction
    CHECK_CXX_COMPILER_FLAG( "-Wmissing-field-initializers" COMPILER_SUPPORTS_WMISSING_INIT )

    if( COMPILER_SUPPORTS_WMISSING_INIT )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wmissing-field-initializers" )
        message( STATUS "Enabling warning -Wmissing-field-initializers" )
    endif()


    # Warn about empty if/for/while bodies
    CHECK_CXX_COMPILER_FLAG( "-Wempty-body" COMPILER_SUPPORTS_WEMPTY_BODY )

    if( COMPILER_SUPPORTS_WEMPTY_BODY )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wempty-body" )
        message( STATUS "Enabling warning -Wempty-body" )
    endif()


    # Warn about out of order intialization
    CHECK_CXX_COMPILER_FLAG( "-Wreorder" COMPILER_SUPPORTS_WREORDER )

    if( COMPILER_SUPPORTS_WREORDER )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wreorder" )
        message( STATUS "Enabling warning -Wreorder" )
    endif()


    # Warn about mismatched class/struct declarations
    CHECK_CXX_COMPILER_FLAG( "-Wmismatched-tags" COMPILER_SUPPORTS_WMISMATCHED_TAGS )

    if( COMPILER_SUPPORTS_WMISMATCHED_TAGS )
        # This warning flag is not supported when using PCH and g++ according to
        # https://gcc.gnu.org/bugzilla/show_bug.cgi?id=99642
        if( NOT ( KICAD_USE_PCH AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU" ) )
            set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wmismatched-tags" )
            message( STATUS "Enabling warning -Wmismatched-tags" )
        endif()
    endif()

    # Warn about improper move statements
    CHECK_CXX_COMPILER_FLAG( "-Wpessimizing-move" COMPILER_SUPPORTS_WPESSIMIZING_MOVE )

    if( COMPILER_SUPPORTS_WPESSIMIZING_MOVE )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wpessimizing-move" )
        message( STATUS "Enabling warning -Wpessimizing-move" )
    endif()

    CHECK_CXX_COMPILER_FLAG( "-Wredundant-move" COMPILER_SUPPORTS_WREDUNDANT_MOVE )

    if( COMPILER_SUPPORTS_WREDUNDANT_MOVE )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wredundant-move" )
        message( STATUS "Enabling warning -Wredundant-move" )
    endif()

    # See if the compiler will throw warnings on these conversions
    CHECK_CXX_COMPILER_FLAG( "-Wimplicit-int-float-conversion" COMPILER_SUPPORTS_WIMPLICIT_FLOAT_CONVERSION )

    if( COMPILER_SUPPORTS_WIMPLICIT_FLOAT_CONVERSION )
        # This one is different, it is used to guard warning removal for this inside the code
        set( HAVE_WIMPLICIT_FLOAT_CONVERSION true )
    endif()

    # See if the compiler will throw warnings on these conversions
    CHECK_CXX_COMPILER_FLAG( "-Wunused-const-variable" COMPILER_SUPPORTS_WUNUSED_CONST_VARIABLE )

    if( COMPILER_SUPPORTS_WUNUSED_CONST_VARIABLE )
        # This one is different, it is used to guard warning removal for this inside the code
        set( HAVE_WUNUSED_CONST_VARIABLE true )
    endif()

    # Suppress GCC warnings about unknown/unused attributes (e.g. cdecl, [[maybe_unused, etc)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-attributes" )
        set( CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-ignored-attributes" )
    endif()

    # Avoid ABI warnings, specifically one about an ABI change on ppc64el from gcc5 to gcc 6.
    CHECK_CXX_COMPILER_FLAG( "-Wpsabi" COMPILER_SUPPORTS_WPSABI )

    if( COMPILER_SUPPORTS_WPSABI )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} -Wno-psabi" )
        message( STATUS "Disabling warning -Wpsabi" )
    endif()

    # Append any additional warning flags so the end of the warning string
    if( KICAD_ADDITIONAL_WARN_FLAGS )
        set( WARN_FLAGS_CXX "${WARN_FLAGS_CXX} ${KICAD_ADDITIONAL_WARN_FLAGS}" )
        message( STATUS "Adding additional warning flags: ${KICAD_ADDITIONAL_WARN_FLAGS}")
    endif()

endif()

# MSVC specific warnings
if( MSVC )
    set( COMPILER_SUPPORTS_WARNINGS TRUE )

    # Establish /Wall early and selectively disable some very common warnings in kicad code
    # or warnings that really shouldn't be warnings. Also some warnings like implicit fallthrough
    # in case statements happen in msvc std lib and despite /external:env:INCLUDE leak
    # into build log generating thousands of noise entries.

    # Unlike gcc /Wall actually enables all warnings on msvc.

    # Warnings for C are not enabled since C files are mostly generated
    # set( WARN_FLAGS_C   "/external:W0 /external:env:INCLUDE /external:I${CMAKE_SOURCE_DIR}/thirdparty /Wall" )
    set( WARN_FLAGS_CXX "/external:W0 /external:env:INCLUDE /external:I${CMAKE_SOURCE_DIR}/thirdparty /Wall" )

    # disable 'type' : class 'type1' needs to have dll-interface to be used by clients of class 'type2'
    string( APPEND WARN_FLAGS_CXX " /wd4251" )
    # disable "function not inlined"
    string( APPEND WARN_FLAGS_CXX " /wd4710" )
    # disable "function selected for inline expansion"
    string( APPEND WARN_FLAGS_CXX " /wd4711" )
    # disable "bytes padding added"
    string( APPEND WARN_FLAGS_CXX " /wd4820" )
    # disable "unreferenced formal parameter"
    string( APPEND WARN_FLAGS_CXX " /wd4100" )
    # disable default/copy/move constructor/assignment implicitly defined as deleted
    string( APPEND WARN_FLAGS_CXX " /wd4623 /wd4625 /wd5026 /wd4626 /wd5027" )
    # disable "compiler will insert Spectre mitigation for..."
    string( APPEND WARN_FLAGS_CXX " /wd5045" )
    # disable "enumerator in switch for enum is not explicitly handled"
    string( APPEND WARN_FLAGS_CXX " /wd4061" )
    # disable "conversion from 'type_1' to 'type_2', signed/unsigned mismatch"
    string( APPEND WARN_FLAGS_CXX " /wd4245 /wd4365" )
    # disable "conversion from 'type1' to 'type2', possible loss of data"
    string( APPEND WARN_FLAGS_CXX " /wd4242 /wd5219" )
    # disable "member function does not override any base class virtual member function"
    string( APPEND WARN_FLAGS_CXX " /wd4263" )
    # disable "no override available for virtual member function from base 'class'; function is hidden"
    string( APPEND WARN_FLAGS_CXX " /wd4264" )
    # disable "no override available for virtual member function, function is hidden"
    string( APPEND WARN_FLAGS_CXX " /wd4266" )
    # disable "class has virtual functions, but its (non)trivial destructor is not virtual"
    string( APPEND WARN_FLAGS_CXX " /wd5204 /wd4265" )
    # disable "layout of class may have changed from a previous version of the compiler"
    string( APPEND WARN_FLAGS_CXX " /wd4371" )
    # disable "'this': used in base member initializer list"
    string( APPEND WARN_FLAGS_CXX " /wd4355" )
    # disable "relative include path contains '..'"
    string( APPEND WARN_FLAGS_CXX " /wd4464" )
    # disable "'const' variable is not used"
    string( APPEND WARN_FLAGS_CXX " /wd5264" )
    # disable "implicit fall-through occurs here" in case statement
    string( APPEND WARN_FLAGS_CXX " /wd5262" )
    # disable "unreferenced inline function has been removed"
    string( APPEND WARN_FLAGS_CXX " /wd4514" )
    # disable "compiler may not enforce left-to-right evaluation order in ..."
    string( APPEND WARN_FLAGS_CXX " /wd4868 /wd4866" )
    # disable "XXX is not defined as a preprocessor macro, replacing with '0'"
    string( APPEND WARN_FLAGS_CXX " /wd4668" )
    # disable "definition of implicit copy constructor for 'X' is deprecated because it has a user-provided destructor"
    string( APPEND WARN_FLAGS_CXX " /wd5267" )
    # disable "reinterpret_cast used between related classes"
    string( APPEND WARN_FLAGS_CXX " /wd4946" )
    # disable throw() is interpreted as declspec(nothrow)
    string( APPEND WARN_FLAGS_CXX " /wd4290" )
    # disable non-bool is explicitly cast to bool, forcing value of 0 or 1
    string( APPEND WARN_FLAGS_CXX " /wd4800" )
    # disable volatile access of '<expression>' is subject to /volatile:[iso|ms] setting;
    string( APPEND WARN_FLAGS_CXX " /wd4746" )
endif()
