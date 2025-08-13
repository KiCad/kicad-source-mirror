# SPDX-FileCopyrightText: 2015 Alex Richardson <arichardson.kde@gmail.com>
#
# SPDX-License-Identifier: BSD-3-Clause

#[=======================================================================[.rst:
FindPoppler
-----------

Try to find Poppler.

This is a component-based find module, which makes use of the COMPONENTS
and OPTIONAL_COMPONENTS arguments to find_module.  The following components
are available::

  Core  Cpp  Qt5  Qt4  Glib

If no components are specified, this module will act as though all components
were passed to OPTIONAL_COMPONENTS.

This module will define the following variables, independently of the
components searched for or found:

``Poppler_FOUND``
    TRUE if (the requested version of) Poppler is available
``Poppler_VERSION``
    Found Poppler version
``Poppler_TARGETS``
    A list of all targets imported by this module (note that there may be more
    than the components that were requested)
``Poppler_LIBRARIES``
    This can be passed to target_link_libraries() instead of the imported
    targets
``Poppler_INCLUDE_DIRS``
    This should be passed to target_include_directories() if the targets are
    not used for linking
``Poppler_DEFINITIONS``
    This should be passed to target_compile_options() if the targets are not
    used for linking

For each searched-for components, ``Poppler_<component>_FOUND`` will be set to
TRUE if the corresponding Poppler library was found, and FALSE otherwise.  If
``Poppler_<component>_FOUND`` is TRUE, the imported target
``Poppler::<component>`` will be defined.  This module will also attempt to
determine ``Poppler_*_VERSION`` variables for each imported target, although
``Poppler_VERSION`` should normally be sufficient.

In general we recommend using the imported targets, as they are easier to use
and provide more control.  Bear in mind, however, that if any target is in the
link interface of an exported library, it must be made available by the
package config file.

Since 5.19
#]=======================================================================]

include(${CMAKE_CURRENT_LIST_DIR}/ECMFindModuleHelpers.cmake)

ecm_find_package_version_check(Poppler)

set(Poppler_known_components
    Cpp
    Qt4
    Qt5
    Qt6
    Glib
)
foreach(_comp ${Poppler_known_components})
    string(TOLOWER "${_comp}" _lc_comp)
    set(Poppler_${_comp}_component_deps "Core")
    set(Poppler_${_comp}_pkg_config "poppler-${_lc_comp}")
    set(Poppler_${_comp}_lib "poppler-${_lc_comp}")
    set(Poppler_${_comp}_header_subdir "poppler/${_lc_comp}")
endforeach()
set(Poppler_known_components Core ${Poppler_known_components})

set(Poppler_Core_component_deps "")
set(Poppler_Core_pkg_config "poppler")
# poppler-config.h header is only installed with --enable-xpdf-headers
# fall back to using any header from a submodule with a path to make it work in that case too
set(Poppler_Core_header "poppler-config.h" "cpp/poppler-version.h" "qt6/poppler-qt6.h" "qt5/poppler-qt5.h" "qt4/poppler-qt4.h" "glib/poppler.h")
set(Poppler_Core_header_subdir "poppler")
set(Poppler_Core_lib "poppler")

set(Poppler_Cpp_header "poppler-version.h")
set(Poppler_Qt6_header "poppler-qt6.h")
set(Poppler_Qt5_header "poppler-qt5.h")
set(Poppler_Qt4_header "poppler-qt4.h")
set(Poppler_Glib_header "poppler.h")

ecm_find_package_parse_components(Poppler
    RESULT_VAR Poppler_components
    KNOWN_COMPONENTS ${Poppler_known_components}
)
ecm_find_package_handle_library_components(Poppler
    COMPONENTS ${Poppler_components}
)

# If pkg-config didn't provide us with version information,
# try to extract it from poppler-version.h or poppler-config.h
if(NOT Poppler_VERSION)
    find_file(Poppler_VERSION_HEADER
        NAMES "poppler-config.h" "cpp/poppler-version.h"
        HINTS ${Poppler_INCLUDE_DIRS}
        PATH_SUFFIXES ${Poppler_Core_header_subdir}
    )
    mark_as_advanced(Poppler_VERSION_HEADER)
    if(Poppler_VERSION_HEADER)
        file(READ ${Poppler_VERSION_HEADER} _poppler_version_header_contents)
        string(REGEX REPLACE
            "^.*[ \t]+POPPLER_VERSION[ \t]+\"([0-9d.]*)\".*$"
            "\\1"
            Poppler_VERSION
            "${_poppler_version_header_contents}"
        )
        unset(_poppler_version_header_contents)
    endif()
endif()

find_package_handle_standard_args(Poppler
    FOUND_VAR
        Poppler_FOUND
    REQUIRED_VARS
        Poppler_LIBRARIES
    VERSION_VAR
        Poppler_VERSION
    HANDLE_COMPONENTS
)

include(FeatureSummary)
set_package_properties(Poppler PROPERTIES
    DESCRIPTION "A PDF rendering library"
    URL "https://poppler.freedesktop.org/"
)
