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

# The backend runs without a GUI, so it takes wxBase alone
find_package( wxWidgets 3.2.0 COMPONENTS base REQUIRED )

# The manifest makes the backend long path aware, so a library member past MAX_PATH checks out
if( WIN32 )
    if( MINGW )
        # KICAD-MDW-GIT_RESOURCES variable is set by the macro.
        mingw_resource_compiler( kicad-mdw-git )
        set( KICAD_MDW_GIT_RESOURCES ${KICAD-MDW-GIT_RESOURCES} )
    else()
        set( KICAD_MDW_GIT_RESOURCES ${CMAKE_SOURCE_DIR}/resources/msw/kicad-mdw-git.rc )
    endif()
endif()

add_executable( kicad-mdw-git
    git_backend.cpp
    git_lifecycle.cpp
    git_shared_libraries.cpp
    kicad_mdw_git.cpp
    mdw_git_checkouts.cpp
    mdw_git_push_outcomes.cpp
    mdw_git_releases.cpp
    mdw_git_remote.cpp
    mdw_git_repo.cpp
    ${KICAD_MDW_GIT_RESOURCES}
    )

kicad_add_mdw_backend( kicad-mdw-git org.kicad.mdw.git org.kicad.mdw.git.kicad-mdw.json INSTALL ON )

target_link_libraries( kicad-mdw-git
    mdw_runtime
    kigit
    nlohmann_json
    ${LIBGIT2_LIBRARIES}
    ${NNG_LIBRARY}
    ${wxWidgets_LIBRARIES}
    )

target_include_directories( kicad-mdw-git PRIVATE
    ${NNG_INCLUDE_DIR}
    ${CMAKE_BINARY_DIR}
    )

# The resource file includes the generated version header
add_dependencies( kicad-mdw-git version_header )
