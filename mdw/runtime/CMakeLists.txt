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

# The runtime links kiapi and nng only, never kicommon or wxWidgets, so wxString stays out of the protocol

set( MDW_RUNTIME_SRCS
    src/mdw_backend_server.cpp
    src/mdw_event_publisher.cpp
    src/mdw_file_io.cpp
    src/mdw_idempotency.cpp
    src/mdw_lifecycle.cpp
    src/mdw_paging.cpp
    src/mdw_poller.cpp
    src/mdw_run_backend.cpp
    src/mdw_staged_folder.cpp
    )

add_library( mdw_runtime STATIC
    ${MDW_RUNTIME_SRCS}
    )

target_link_libraries( mdw_runtime
    PUBLIC
        kiapi
        ${NNG_LIBRARY}
    )

target_include_directories( mdw_runtime
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${NNG_INCLUDE_DIR}
    )
