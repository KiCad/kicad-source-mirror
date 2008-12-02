macro(create_svn_version_header)
    # Include Subversion support to automagically create version header file.
    find_package(Subversion)

    if(Subversion_FOUND)
        # Copied from the CMake module FindSubversion.cmake.  The default
        # version prevents generating the output files when the "svn info"
        # command fails.  Just fall back to using "build_version.h" for
        # the version strings.
        set(_Subversion_SAVED_LC_ALL "$ENV{LC_ALL}")
        set(ENV{LC_ALL} C)

        execute_process(
            COMMAND ${Subversion_SVN_EXECUTABLE} info ${PROJECT_SOURCE_DIR}
            OUTPUT_VARIABLE Kicad_WC_INFO
            ERROR_VARIABLE _svn_error
            RESULT_VARIABLE _svn_result
            OUTPUT_STRIP_TRAILING_WHITESPACE)

        if(NOT ${_svn_result} EQUAL 0)
            message(STATUS
                    "Using <build_version.h> for version string.")
        else(NOT ${_svn_result} EQUAL 0)
            string(REGEX REPLACE "^(.*\n)?URL: ([^\n]+).*"
                   "\\2" Kicad_WC_URL "${Kicad_WC_INFO}")
            string(REGEX REPLACE "^(.*\n)?Revision: ([^\n]+).*"
                   "\\2" Kicad_WC_REVISION "${Kicad_WC_INFO}")
            string(REGEX REPLACE "^(.*\n)?Last Changed Author: ([^\n]+).*"
                   "\\2" Kicad_WC_LAST_CHANGED_AUTHOR "${Kicad_WC_INFO}")
            string(REGEX REPLACE "^(.*\n)?Last Changed Rev: ([^\n]+).*"
                   "\\2" Kicad_WC_LAST_CHANGED_REV "${Kicad_WC_INFO}")
            string(REGEX REPLACE "^(.*\n)?Last Changed Date: ([^\n]+).*"
                   "\\2" Kicad_WC_LAST_CHANGED_DATE "${Kicad_WC_INFO}")
        endif(NOT ${_svn_result} EQUAL 0)

        set(ENV{LC_ALL} ${_Subversion_SAVED_LC_ALL})
    endif(Subversion_FOUND)

    # Check to make sure 'svn info' command did not fail.  Otherwise fallback
    # to vesion strings defined in "<kicad-src-dir>/include/build_version.h".
    if(Kicad_WC_LAST_CHANGED_DATE)
        string(REGEX REPLACE "^([0-9]+)\\-([0-9]+)\\-([0-9]+).*" "\\1\\2\\3"
               _kicad_svn_date ${Kicad_WC_LAST_CHANGED_DATE})
        set(KICAD_SVN_VERSION
            "(${_kicad_svn_date} SVN-R${Kicad_WC_LAST_CHANGED_REV})")
        set(KICAD_ABOUT_VERSION
            "SVN-R${Kicad_WC_LAST_CHANGED_REV} (${_kicad_svn_date})")

        # Definition to conditionally use date and revision returned from the
        # Subversion info command instead of hand coded date and revision in
        # "include/build_version.h".  If subversion is not found then the date
        # and version information must be manually edited.
        # Directive means SVN build, program version and build version will
        # reflect this.
        add_definitions(-DHAVE_SVN_VERSION)

        # Generate config.h.
        configure_file(${CMAKE_SOURCE_DIR}/CMakeModules/config.h.cmake
                       ${CMAKE_BINARY_DIR}/config.h)

        message(STATUS "Kicad SVN version: ${KICAD_SVN_VERSION}")
        message(STATUS "Kicad about version: ${KICAD_ABOUT_VERSION}")
    endif(Kicad_WC_LAST_CHANGED_DATE)
endmacro(create_svn_version_header)
