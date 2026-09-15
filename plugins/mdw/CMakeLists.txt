# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright (C) 2026 KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software: you can redistribute it and/or modify it
# under the terms of the GNU General Public License as published by the
# Free Software Foundation, either version 3 of the License, or (at your
# option) any later version.
#
# This program is distributed in the hope that it will be useful, but
# WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

# In-tree Managed Design Workspace backends, each a standalone executable on the MDW runtime

# Build aTarget beside aManifest in <build>/plugins/mdw/aSubdir, a folder a user copies into their plugins/mdw.
# INSTALL ON also ships it, the executable beside KiCad's own and the manifest in the stock plugins/mdw.
function( kicad_add_mdw_backend aTarget aSubdir aManifest )
    cmake_parse_arguments( ARG "" "INSTALL" "" ${ARGN} )

    set( folder ${CMAKE_BINARY_DIR}/plugins/mdw/${aSubdir} )
    set( manifest ${CMAKE_CURRENT_SOURCE_DIR}/${aManifest} )

    set_target_properties( ${aTarget} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${folder} )
    configure_file( ${manifest} ${folder}/${aManifest} COPYONLY )

    # A copied folder must carry the DLLs it links, kiapi among them; with none, copy_if_different would fail
    if( WIN32 AND NOT ARG_INSTALL )
        set( dlls $<TARGET_RUNTIME_DLLS:${aTarget}> )

        add_custom_command( TARGET ${aTarget} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E $<IF:$<BOOL:${dlls}>,copy_if_different,true> ${dlls} ${folder}
            COMMAND_EXPAND_LISTS
            )
    endif()

    if( NOT ARG_INSTALL )
        return()
    endif()

    # macOS and Windows find a build tree's DLLs and frameworks only beside the KiCad executables, and
    # look for the manifest where an install keeps it unless KICAD_RUN_FROM_BUILD_DIR is set
    if( APPLE )
        set_target_properties( ${aTarget} PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY ${OSX_BUNDLE_BUILD_BIN_DIR}
            INSTALL_RPATH "@executable_path/../Frameworks"
            )
        configure_file( ${manifest} ${OSX_BUNDLE_BUILD_DIR}/${OSX_BUNDLE_SUP_DIR}/plugins/mdw/${aManifest} COPYONLY )
    elseif( MSVC )
        set_target_properties( ${aTarget} PROPERTIES RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/kicad )
        configure_file( ${manifest} ${CMAKE_BINARY_DIR}/kicad/scripting/plugins/mdw/${aManifest} COPYONLY )
    endif()

    # The bundle carries the macOS executable
    if( NOT APPLE )
        install( TARGETS ${aTarget}
            DESTINATION ${KICAD_BIN}
            COMPONENT binary
            )
    endif()

    # Where PATHS::GetStockPluginsPath() looks
    if( WIN32 )
        set( manifestDir ${KICAD_BIN}/scripting/plugins/mdw )
    else()
        set( manifestDir ${KICAD_PLUGINS}/mdw )
    endif()

    install( FILES ${manifest}
        DESTINATION ${manifestDir}
        COMPONENT resources
        )
endfunction()

add_subdirectory( git )
