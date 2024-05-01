#
#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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
#  along with this program; if not, you may find one here:
#  http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
#  or you may search the http://www.gnu.org website for the version 2 license,
#  or you may write to the Free Software Foundation, Inc.,
#  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA

# Build file for docset generation.
#
# Docsets are generated from the Doxygen docs by this process:
# * Modify the existing doxygen file for a docset-friendly output
# * Run doxygen to generate normal docygen output
# * Run a makefile made by doxygen to start the docset
# * Run doxytag2zealdb to generate the docset index
# * Make a couple of changes to the Plist file and add icons

find_program(DOXYTAG2ZEALDB doxytag2zealdb)
find_program(SED sed)

if(DOXYGEN_FOUND AND DOXYTAG2ZEALDB AND SED)

    # The DocSet's bundle ID, which is used for most of the ID's
    set(DOCSET_BUNDLE_ID KiCad)

    # The source for the doxygen config
    set(SRC_DOXYFILE ${CMAKE_SOURCE_DIR}/doxygen/Doxyfile)

    # A new doxyfile with the original, plus some extra config
    set(DOCSET_DOXYFILE ${CMAKE_CURRENT_BINARY_DIR}/doxygen/Doxyfile)

    # Various pieces of the docset
    set(DOCSET_LOC ${CMAKE_CURRENT_BINARY_DIR}/doxygen/html/${DOCSET_BUNDLE_ID}.docset)
    set(DOXY_MAKEFILE ${CMAKE_CURRENT_BINARY_DIR}/doxygen/html/Makefile)
    set(DOXY_TAG_FILE ${CMAKE_CURRENT_BINARY_DIR}/${DOCSET_BUNDLE_ID}.tag)
    set(DOCSET_PLIST ${DOCSET_LOC}/Contents/Info.plist)
    set(DOCSET_DSIDX ${DOCSET_LOC}/Contents/Resources/docSet.dsidx)

    #icon files
    set(DOCSET_SRC_ICON16 ${CMAKE_CURRENT_SOURCE_DIR}/icon-16.png)
    set(DOCSET_ICON16 ${DOCSET_LOC}/icon.png)

    add_custom_command(
        COMMAND ${CMAKE_COMMAND}
            -DSRC_DOXYFILE=${SRC_DOXYFILE}
            -DDOCSET_BUNDLE_ID=${DOCSET_BUNDLE_ID}
            -DDOXY_TAG_FILE=${DOXY_TAG_FILE}
            -DDOCSET_BUNDLE_ID=${DOCSET_BUNDLE_ID}
            -DDOCSET_DOXYFILE=${DOCSET_DOXYFILE}
            -DOUTPUT_DIRECTORY="${CMAKE_CURRENT_BINARY_DIR}/doxygen"
            -DKICAD_CMAKE_MODULE_PATH=${KICAD_CMAKE_MODULE_PATH}
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateDocsetDoxyfile.cmake
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/
        OUTPUT ${DOCSET_DOXYFILE}
        DEPENDS version_header
                ${SRC_DOXYFILE}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateDocsetDoxyfile.cmake
        COMMENT "Modifying doxyfile for docset creation"
        )

    add_custom_command(
        COMMAND ${DOXYGEN_EXECUTABLE} ${DOCSET_DOXYFILE}
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/doxygen
        OUTPUT ${DOXY_TAG_FILE} ${DOXY_MAKEFILE}
        DEPENDS ${DOCSET_DOXYFILE}
        COMMENT "Generating Doxygen for DocSet"
    )

    # Generate the skeleton of the docset
    # And modify the plist: DocSetPlatformFamily is used for the prefix in Zeal,
    add_custom_command(
        COMMAND make || true
        COMMAND ${SED} -i "/<key>DocSetPlatformFamily<\\/key>/!b;n;s/doxygen/${DOCSET_BUNDLE_ID}/" ${DOCSET_PLIST}
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen/html
        DEPENDS ${DOXY_MAKEFILE}
        OUTPUT ${DOCSET_PLIST}
        COMMENT "Running doxygen-generated makefile"
        VERBATIM
    )

    add_custom_command(
        COMMAND ${DOXYTAG2ZEALDB} --tag ${DOXY_TAG_FILE}
            --db ${DOCSET_DSIDX}
            --include-parent-scopes
            --include-function-signatures
        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/doxygen/html
        DEPENDS ${DOCSET_PLIST} ${DOXY_TAG_FILE}
        OUTPUT ${DOCSET_DSIDX}
        COMMENT "Generating docset index"
    )

    add_custom_command(
        COMMAND ${CMAKE_COMMAND} -E copy ${DOCSET_SRC_ICON16} ${DOCSET_ICON16}
        DEPENDS ${DOCSET_DSIDX} ${DOCSET_SRC_ICON16}
        OUTPUT ${DOCSET_ICON16}
        COMMENT "Copying docset icons"
    )

    add_custom_target(docset
        DEPENDS ${DOCSET_ICON16}
    )

endif()
