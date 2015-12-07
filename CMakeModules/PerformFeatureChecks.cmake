#
# Check for platform specific features and generate configuration header..
#
# This cmake file was written to create a platform specific configuration
# header to handle differences between build platforms.  Please add new
# feature checks to this file.  Always check the wxWidgets headers first
# before adding new feature checks.  The wxWidgets build system does a
# very good job of handling platform and compiler differences.
#
# Should you feel the need to do this:
#
# #ifdef SYSTEM_A
# #  include <some_header_for_system_a.h>
# #elif SYSTEM_B
# #  include <some_other_header_for_system_b.h>
# #elif SYSTEM_C
# #  include <yet_another_header_for_system_c.h>
# #endif
#
# in your source, don't.  It is not portable nor is it maintainable.
# Use cmake to detect system specific dependencies and update the
# configuration header instead.
#
# See this link for information on writing cmake system checks:
#
#     http://www.vtk.org/Wiki/CMake_HowToDoPlatformChecks
#
# More importantly see "Recommendations for Writing Autoconf Macros" in:
#
#     http://www.lrde.epita.fr/~adl/dl/autotools.pdf
#
# for an explanation of why you should do this.  Even though this is an
# autotools tutorial.  This section clearly explains why checking for
# features is superior to checking for systems.  The previous section of
# this tutorial shows how to create a system independent check for _mkdir().
# Consider it a benchmark when writing your own feature tests.
#

macro( perform_feature_checks )

    include( CheckIncludeFile )
    #include( CheckFunctionExists )
    include( CheckLibraryExists )
    include( CheckSymbolExists )
    include( CheckIncludeFileCXX )
    include( CheckCXXSymbolExists )
    include( CheckCXXSourceCompiles )
    include( CheckCXXCompilerFlag )


    check_include_file( "malloc.h" HAVE_MALLOC_H )

    # FIXME: Visual C++ does not support the "not" keyword natively.  It is
    #        defined as a macro in <iso646.h>.  There should be a cmake macro
    #        to check if compiler supports the not keyword natively.  If not,
    #        then check for <iso646.h> and include it.  Although it doesn't
    #        appear to cause any problems with other compilers, that doesn't
    #        mean won't fail somewhere down the line.
    check_include_file( "iso646.h" HAVE_ISO646_H )

    # The STDINT header file test is required because MinGW under Windows
    # doesn't define HAVE_STDINT_H even though it does have it.
    #
    # We need to add it to the global compiler definitions as config.h is not
    # included in pyport.h which is where the problem ocurrs without this
    # fix.
    check_include_file( "stdint.h" HAVE_STDINT_H )

    if( HAVE_STDINT_H )
        add_definitions( -DHAVE_STDINT_H )
    endif()

    # no place is this used, and "HAVE_STRINGS_H", if present in config.h then
    # conflicts with /usr/include/python2.6/Python.h.  Please rename the macro if
    # re-introduce this.
    # check_include_file("strings.h" HAVE_STRINGS_H)

    check_symbol_exists( strcasecmp "string.h" HAVE_STRCASECMP )
    check_symbol_exists( strcasecmp "strings.h" HAVE_STRCASECMP )
    check_symbol_exists( strncasecmp "string.h" HAVE_STRNCASECMP )
    check_symbol_exists( strncasecmp "strings.h" HAVE_STRNCASECMP )
    check_symbol_exists( strtok_r "string.h" HAVE_STRTOKR )

    check_cxx_symbol_exists( strcasecmp "string.h" HAVE_STRCASECMP )
    check_cxx_symbol_exists( strncasecmp "string.h" HAVE_STRNCASECMP )

    # Some platforms define malloc and free in malloc.h instead of stdlib.h.
    check_symbol_exists( malloc "stdlib.h" MALLOC_IN_STDLIB_H )

    # Check for functions in math.h.
    check_include_file( "math.h" HAVE_MATH_H )

    # Check for functions in C++ cmath.
    check_include_file_cxx( cmath HAVE_CXX_CMATH )
    check_cxx_symbol_exists( asinh cmath HAVE_CMATH_ASINH )
    check_cxx_symbol_exists( acosh cmath HAVE_CMATH_ACOSH )
    check_cxx_symbol_exists( atanh cmath HAVE_CMATH_ATANH )

    # CMakes check_cxx_symbol_exists() doesn't work for templates so we must create a
    # small program to verify isinf() exists in cmath.
    check_cxx_source_compiles( "#include <cmath>\nint main(int argc, char** argv)\n{\n  (void)argv;\n  std::isinf(1.0);  (void)argc;\n  return 0;\n}\n"  HAVE_CMATH_ISINF )

    #check_symbol_exists( clock_gettime "time.h" HAVE_CLOCK_GETTIME ) non-standard library, does not work
    check_library_exists( rt clock_gettime "" HAVE_CLOCK_GETTIME )

    # HAVE_GETTIMEOFDAY is already in use within 2.9 wxWidgets, so use HAVE_GETTIMEOFDAY_FUNC
    check_symbol_exists( gettimeofday "sys/time.h" HAVE_GETTIMEOFDAY_FUNC )

    # Check for Posix getc_unlocked() for improved performance over getc().  Fall back to
    # getc() on platforms where getc_unlocked() doesn't exist.
    check_symbol_exists( getc_unlocked "stdio.h" HAVE_FGETC_NOLOCK )

endmacro( perform_feature_checks )
