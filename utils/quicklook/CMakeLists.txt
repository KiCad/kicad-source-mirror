# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or (at your option)
# any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, you may find one here:
# http://www.gnu.org/licenses/old-licenses/gpl-3.0.html

# The enclosing utils/CMakeLists.txt adds this directory only on Apple hosts after the shared
# importer target is available.
if( NOT APPLE )
    return()
endif()

foreach( required
        KICAD_PREVIEW_BUNDLE_ID
        KICAD_THUMBNAIL_BUNDLE_ID
        KICAD_VERSION
        KICAD_VERSION_FULL )
    if( NOT DEFINED ${required} OR "${${required}}" STREQUAL "" )
        message( FATAL_ERROR "Quick Look requires ${required}" )
    endif()
endforeach()

set( KIQL_SHARED_SRCS
    kiql_import_queue.mm
    kiql_model_scene.mm
    kiql_preview_view.mm
    )

# App extensions are ordinary Mach-O executables in a bundle whose entry point is NSExtensionMain,
# so they build under the same generator as the rest of KiCad.
function( kicad_add_quicklook_extension target bundle_name plist entitlements )
    add_executable( ${target} MACOSX_BUNDLE ${KIQL_SHARED_SRCS} ${ARGN} )

    set_target_properties( ${target} PROPERTIES
        BUNDLE TRUE
        BUNDLE_EXTENSION appex
        OUTPUT_NAME ${bundle_name}
        MACOSX_BUNDLE_INFO_PLIST ${plist}
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
        )

    target_compile_options( ${target} PRIVATE -fobjc-arc -fapplication-extension )
    target_link_options( ${target} PRIVATE -e _NSExtensionMain -fapplication-extension )

    target_include_directories( ${target} PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${GLM_INCLUDE_DIR}
        )

    target_link_libraries( ${target} PRIVATE
        kicad_3d_import
        "-framework AppKit"
        "-framework Cocoa"
        "-framework Foundation"
        "-framework Metal"
        "-framework Quartz"
        "-framework QuickLookThumbnailing"
        "-framework SceneKit"
        )

    set_property( TARGET ${target} PROPERTY KIQL_ENTITLEMENTS "${entitlements}" )
endfunction()

kicad_add_quicklook_extension( kicad_model_preview_appex KiCadModelPreview
    "${CMAKE_CURRENT_SOURCE_DIR}/Preview/Info.plist.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/Preview/Preview.entitlements"
    Preview/kiql_preview_view_controller.mm )

kicad_add_quicklook_extension( kicad_model_thumbnail_appex KiCadModelThumbnail
    "${CMAKE_CURRENT_SOURCE_DIR}/Thumbnail/Info.plist.in"
    "${CMAKE_CURRENT_SOURCE_DIR}/Thumbnail/Thumbnail.entitlements"
    Thumbnail/kiql_thumbnail_provider.mm )

add_custom_target( kicad_model_quicklook )
add_dependencies( kicad_model_quicklook kicad_model_preview_appex kicad_model_thumbnail_appex )

set( KICAD_QUICKLOOK_PREVIEW_APPEX "${CMAKE_CURRENT_BINARY_DIR}/KiCadModelPreview.appex"
    CACHE INTERNAL "Built Quick Look preview extension" )
set( KICAD_QUICKLOOK_THUMBNAIL_APPEX "${CMAKE_CURRENT_BINARY_DIR}/KiCadModelThumbnail.appex"
    CACHE INTERNAL "Built Quick Look thumbnail extension" )
