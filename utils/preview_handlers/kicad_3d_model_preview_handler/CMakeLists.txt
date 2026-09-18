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

target_link_libraries(kicad_3d_model_previewer
    kicad_3d_import
    ${OCC_LIBRARIES}
    ${OCC_OPENGL_LIBRARY}
    Version
)

install(TARGETS kicad_3d_model_previewer
    RUNTIME_DEPENDENCY_SET kicad_3d_preview_runtime_deps
    RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary
)

install( RUNTIME_DEPENDENCY_SET kicad_3d_preview_runtime_deps
    DIRECTORIES "$<TARGET_FILE_DIR:kicad_3d_model_previewer>"
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
    RUNTIME DESTINATION ${KICAD_BIN}
    COMPONENT binary
)
