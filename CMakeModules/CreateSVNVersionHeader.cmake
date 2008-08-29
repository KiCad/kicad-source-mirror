macro(create_svn_version_header)
    # Include Subversion support to automagically create version header file.
    find_package(SubversionCVS)
    if(Subversion_FOUND)
        Subversion_WC_INFO(${PROJECT_SOURCE_DIR} Kicad)
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
    endif(Subversion_FOUND)
endmacro(create_svn_version_header)
