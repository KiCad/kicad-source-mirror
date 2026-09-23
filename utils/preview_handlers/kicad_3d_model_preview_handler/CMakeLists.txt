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

if (COMPILER_SUPPORTS_WARNINGS)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

include_directories(
    "${CMAKE_SOURCE_DIR}/utils/preview_handlers/kicad_3d_model_preview_handler"
    "${CMAKE_BINARY_DIR}"
)
include_directories( BEFORE ${INC_BEFORE} )

set(KICAD_3D_PREVIEW_HANDLER_FILES
    kicad_3d_model_previewer.rc
    kicad_3d_model_to_occ.cpp
    kicad_3d_preview_handler.cpp
    kicad_3d_preview_class_factory.cpp
    kicad_3d_preview_register.cpp
    dllmain.cpp
)

add_library(kicad_3d_model_previewer SHARED ${KICAD_3D_PREVIEW_HANDLER_FILES})

find_library( OCC_OPENGL_LIBRARY TKOpenGl HINTS "${OCC_LIBRARY_DIR}" NO_DEFAULT_PATH REQUIRED )

set_target_properties(kicad_3d_model_previewer PROPERTIES LINK_FLAGS "/DEF:${CMAKE_SOURCE_DIR}/utils/preview_handlers/kicad_3d_model_preview_handler/exports.def")

# The .rc pulls in the generated kicad_build_version.h, which only a full build happened to
# produce first
add_dependencies(kicad_3d_model_previewer version_header)

target_link_libraries(kicad_3d_model_previewer
    kicad_3d_import
    ${OCC_LIBRARIES}
    ${OCC_OPENGL_LIBRARY}
    Version
)

# The shell loads this handler into its own surrogate process, whose application directory is
# System32, so LOAD_WITH_ALTERED_SEARCH_PATH can only resolve KiCad's own DLLs from the directory
# holding the handler.  An install puts them all in one bin directory; the build tree does not.
get_filename_component( KICAD_3D_PREVIEW_VCPKG_BIN_DIR "${wxWidgets_LIB_DIR}/../bin" ABSOLUTE )

add_custom_command(TARGET kicad_3d_model_previewer POST_BUILD
    COMMAND ${CMAKE_COMMAND}
        "-DHANDLER=$<TARGET_FILE:kicad_3d_model_previewer>"
        "-DKICAD_LIBS=$<TARGET_FILE:kicad_3d_import>$<SEMICOLON>$<TARGET_FILE:kicad_3dsg>"
        "-DDEST=$<TARGET_FILE_DIR:kicad_3d_model_previewer>"
        "-DSEARCH_DIRS=${KICAD_3D_PREVIEW_VCPKG_BIN_DIR}$<SEMICOLON>$<TARGET_FILE_DIR:kicad_3d_import>$<SEMICOLON>$<TARGET_FILE_DIR:kicad_3dsg>"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/stage_build_tree_deps.cmake"
    VERBATIM
)

# The packager deploys the vcpkg DLLs and the MSVC runtime.  Resolving them here instead fails
# the install on any host that cannot find one, such as a builder without VCRUNTIME140_1.dll
install(TARGETS kicad_3d_model_previewer
    RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary
)

if( KICAD_WIN32_INSTALL_PDBS )
    install( FILES $<TARGET_PDB_FILE:kicad_3d_model_previewer> DESTINATION ${KICAD_BIN} )
endif()
