# This program source code file is part of KiCad, a free EDA CAD application.
#
# Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

# A backend whose challenge names the realm the test sets, or which is too old to check its server
add_executable( qa_mdw_realm_backend
    mdw_realm_backend.cpp
    )

target_link_libraries( qa_mdw_realm_backend
    mdw_runtime
    )

# Drives the real kicad-mdw-git process through the supervisor
add_executable( qa_mdw_git
    test_mdw_backend_registry.cpp
    test_mdw_callback_thread.cpp
    test_mdw_disconnect_here.cpp
    test_mdw_framework_lifecycle.cpp
    test_mdw_git_auth.cpp
    test_mdw_git_baseline.cpp
    test_mdw_git_claims.cpp
    test_mdw_git_credentials.cpp
    test_mdw_git_library_requirements.cpp
    test_mdw_git_library_rows.cpp
    test_mdw_git_library_sync.cpp
    test_mdw_git_permissions.cpp
    test_mdw_git_release.cpp
    test_mdw_git_revision_copy.cpp
    test_mdw_git_workspace_listing.cpp
    test_mdw_reply_checks.cpp
    ${CMAKE_SOURCE_DIR}/plugins/mdw/git/mdw_git_checkouts.cpp
    ${CMAKE_SOURCE_DIR}/plugins/mdw/git/mdw_git_remote.cpp
    ${CMAKE_SOURCE_DIR}/plugins/mdw/git/mdw_git_repo.cpp
    )

add_dependencies( qa_mdw_git kicad-mdw-git qa_mdw_realm_backend )

target_link_libraries( qa_mdw_git
    qa_utils
    kicommon
    mdw_runtime
    kiapi
    ${LIBGIT2_LIBRARIES}
    ${wxWidgets_LIBRARIES}
    )

target_include_directories( qa_mdw_git PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/common
    ${CMAKE_SOURCE_DIR}/plugins/mdw/git
    ${CMAKE_SOURCE_DIR}/thirdparty/nlohmann_json
    ${CURL_INCLUDE_DIRS}
    )

target_compile_definitions( qa_mdw_git PRIVATE
    MDW_GIT_JSON_FROM_KICOMMON
    MDW_GIT_EXECUTABLE="$<TARGET_FILE:kicad-mdw-git>"
    MDW_REALM_EXECUTABLE="$<TARGET_FILE:qa_mdw_realm_backend>"
    )

# Generous for slow Windows Debug builds
kicad_add_boost_test( qa_mdw_git qa_mdw_git NO_CATCH_SYSTEM_ERRORS TIMEOUT 900 )

setup_qa_env( qa_mdw_git )
