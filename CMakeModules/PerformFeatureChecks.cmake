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

macro(perform_feature_checks)

    include(CheckIncludeFile)

    check_include_file("malloc.h" HAVE_MALLOC_H)

    # FIXME: Visual C++ does not support the "not" keyword natively.  It is
    #        defined as a macro in <iso646.h>.  There should be a cmake macro
    #        to check if compiler supports the not keyword natively.  If not,
    #        then check for <iso646.h> and include it.  Although it doesn't
    #        appear to cause any problems with other compilers, that doesn't
    #        mean won't fail somewhere down the line.
    check_include_file("iso646.h" HAVE_ISO646_H)

    check_include_file("strings.h" HAVE_STRINGS_H)

    include(CheckSymbolExists)
    check_symbol_exists(strcasecmp "string.h" HAVE_STRCASECMP)
    check_symbol_exists(strcasecmp "strings.h" HAVE_STRCASECMP)
    check_symbol_exists(strncasecmp "string.h" HAVE_STRNCASECMP)
    check_symbol_exists(strncasecmp "strings.h" HAVE_STRNCASECMP)

    # Some platforms define malloc and free in malloc.h instead of stdlib.h.
    check_symbol_exists(malloc "stdlib.h" MALLOC_IN_STDLIB_H)

    # Use ISO C++ conformant names to disable Visual C++ warnings.
    check_symbol_exists(_stricmp "string.h" HAVE_ISO_STRICMP)
    check_symbol_exists(_strnicmp "string.h" HAVE_ISO_STRNICMP)
    check_symbol_exists(_snprintf "stdio.h" HAVE_ISO_SNPRINTF)
    check_symbol_exists(_hypot "math.h" HAVE_ISO_HYPOT)

    # Generate config.h.
    configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/config.h.cmake
                   ${CMAKE_BINARY_DIR}/config.h)

endmacro(perform_feature_checks)
