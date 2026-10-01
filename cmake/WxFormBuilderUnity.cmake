#  This program source code file is part of KICAD, a free EDA CAD application.
#
#  Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

# Number of wxFormBuilder sources compiled together in one unity translation unit
set( KICAD_WXFB_UNITY_BATCH_SIZE 16 )


# Function kicad_collect_targets
#
# Collects every buildsystem target defined in a directory and its subdirectories.
#
# Arguments:
#  - OUT_VAR is the variable receiving the list of targets
#  - DIR is the source directory to start from
function( kicad_collect_targets OUT_VAR DIR )
    get_property( _targets DIRECTORY ${DIR} PROPERTY BUILDSYSTEM_TARGETS )
    get_property( _subdirs DIRECTORY ${DIR} PROPERTY SUBDIRECTORIES )

    foreach( _subdir IN LISTS _subdirs )
        kicad_collect_targets( _sub_targets ${_subdir} )
        list( APPEND _targets ${_sub_targets} )
    endforeach()

    set( ${OUT_VAR} ${_targets} PARENT_SCOPE )
endfunction()


# Function kicad_unity_wxformbuilder_sources
#
# wxFormBuilder output only needs a few wxWidgets headers, so loading a target's precompiled
# header costs more than it saves.  The files are also free of file-local symbols, which makes
# them safe to compile in batches.  This takes them off the precompiled header and groups them
# into unity translation units.  It must be called after all targets have been defined.
#
# A source can opt out of batching with the SKIP_UNITY_BUILD_INCLUSION source property.
function( kicad_unity_wxformbuilder_sources )
    kicad_collect_targets( _all_targets ${CMAKE_SOURCE_DIR} )

    foreach( _target IN LISTS _all_targets )
        get_target_property( _type ${_target} TYPE )

        if( NOT _type MATCHES "^(EXECUTABLE|STATIC_LIBRARY|SHARED_LIBRARY|MODULE_LIBRARY|OBJECT_LIBRARY)$" )
            continue()
        endif()

        get_target_property( _sources ${_target} SOURCES )
        get_target_property( _source_dir ${_target} SOURCE_DIR )

        if( NOT _sources )
            continue()
        endif()

        set( _batchable )

        foreach( _source IN LISTS _sources )
            if( NOT _source MATCHES "_base\\.cpp$" OR _source MATCHES "\\$<" )
                continue()
            endif()

            get_filename_component( _path ${_source} ABSOLUTE BASE_DIR ${_source_dir} )

            if( NOT EXISTS ${_path} )
                continue()
            endif()

            # Hand-written *_base.cpp files exist too, so identify generated ones by their banner
            file( STRINGS ${_path} _banner LIMIT_COUNT 4 REGEX "generated with wxFormBuilder" )

            if( NOT _banner )
                continue()
            endif()

            set_source_files_properties( ${_path} TARGET_DIRECTORY ${_target}
                    PROPERTIES SKIP_PRECOMPILE_HEADERS ON )

            get_source_file_property( _skip_unity ${_path} TARGET_DIRECTORY ${_target}
                    SKIP_UNITY_BUILD_INCLUSION )

            if( NOT _skip_unity )
                list( APPEND _batchable ${_path} )
            endif()
        endforeach()

        list( REMOVE_DUPLICATES _batchable )
        list( LENGTH _batchable _count )

        if( _count LESS 2 )
            continue()
        endif()

        # Sort so that batch membership does not depend on the order sources were listed
        list( SORT _batchable )
        get_target_property( _binary_dir ${_target} BINARY_DIR )
        set( _index 0 )
        set( _batches )

        foreach( _path IN LISTS _batchable )
            math( EXPR _batch "${_index} / ${KICAD_WXFB_UNITY_BATCH_SIZE}" )
            math( EXPR _index "${_index} + 1" )

            set_source_files_properties( ${_path} TARGET_DIRECTORY ${_target}
                    PROPERTIES UNITY_GROUP "wxfb_${_batch}" )
            list( APPEND _batches ${_batch} )
        endforeach()

        list( REMOVE_DUPLICATES _batches )

        # CMake does not carry SKIP_PRECOMPILE_HEADERS over to the unity source it generates, so
        # set it on the path that CMake will use for each group
        foreach( _batch IN LISTS _batches )
            set_source_files_properties(
                    ${_binary_dir}/CMakeFiles/${_target}.dir/Unity/unity_wxfb_${_batch}_cxx.cxx
                    TARGET_DIRECTORY ${_target}
                    PROPERTIES SKIP_PRECOMPILE_HEADERS ON )
        endforeach()

        set_target_properties( ${_target} PROPERTIES
                UNITY_BUILD ON
                UNITY_BUILD_MODE GROUP )
    endforeach()
endfunction()
