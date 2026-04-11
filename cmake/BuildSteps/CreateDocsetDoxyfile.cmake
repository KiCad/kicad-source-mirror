#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2020 KiCad Developers, see AUTHORS.txt for contributors.
#
#  This program is free software; you can redistribute it and/or
#  modify it under the terms of the GNU General Public License
#  as published by the Free Software Foundation; either version 2
#  of the License, or (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program.  If not, see <https://www.gnu.org/licenses/>.

function(get_kicad_doc_version RESULT_NAME)

    include( ${KICAD_CMAKE_MODULE_PATH}/CreateGitVersionHeader.cmake )
    create_git_version_header(${CMAKE_SOURCE_DIR})

    # Now we have KICAD_VERSION, but it's got () around it
    string(REPLACE "(" "" KICAD_VERSION ${KICAD_VERSION})
    string(REPLACE ")" "" KICAD_VERSION ${KICAD_VERSION})

    set (${RESULT_NAME} ${KICAD_VERSION} PARENT_SCOPE)

endfunction()

get_kicad_doc_version(KICAD_DOC_VERSION)

# copy and modify the "normal" Doxyfile
cmake_path(GET DOCSET_DOXYFILE PARENT_PATH OUTPUT_DIR)
file(MAKE_DIRECTORY ${OUTPUT_DIR})
file(COPY_FILE ${SRC_DOXYFILE} ${DOCSET_DOXYFILE})
file(APPEND ${DOCSET_DOXYFILE} "

# Added for DocSet generation
OUTPUT_DIRECTORY        = ${OUTPUT_DIRECTORY}
PROJECT_NAME            = ${DOCSET_BUNDLE_ID}
PROJECT_NUMBER          = ${KICAD_DOC_VERSION}
GENERATE_DOCSET         = YES
DOCSET_FEEDNAME         = ${DOCSET_BUNDLE_ID}
DOCSET_BUNDLE_ID        = ${DOCSET_BUNDLE_ID}
DISABLE_INDEX           = YES
GENERATE_TREEVIEW       = NO
SEARCHENGINE            = NO
GENERATE_TAGFILE        = ${DOXY_TAG_FILE}
# Doxygen-awesome doesn't look great without the index
HTML_EXTRA_STYLESHEET   = "
    )
