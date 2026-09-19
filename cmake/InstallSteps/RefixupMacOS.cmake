# RefixupMacOS.cmake

# Now that we don't use BundleUtilities and instead use GET_RUNTIME_DEPENDENCIES,
# the binaries that are built all have absolute path library load commands.
# What we need to do here is update all the paths for _the runtime dependencies
# that we installed into the bundle_ to live in @rpath, with Python getting
# special treatment in that it lives in @rpath/Frameworks.

# Make sure GLOB_RECURSE doesn't follow symlinks
cmake_policy( PUSH )
cmake_policy( SET CMP0009 NEW )

function( refix_kicad_bundle target )
    # target should be the path to the kicad.app directory

    string( TIMESTAMP start_time )

    file( GLOB_RECURSE items ${target}/*.dylib ${target}/*.so ${target}/*.kiface )
    file( GLOB appex_binaries ${target}/Contents/PlugIns/*.appex/Contents/MacOS/* )
    file( GLOB framework_directories ${target}/Contents/Frameworks/*.framework )
    set( framework_binaries )

    foreach( framework ${framework_directories} )
        get_filename_component( framework_name "${framework}" NAME_WE )
        file( GLOB framework_executable "${framework}/Versions/*/${framework_name}" )

        foreach( executable ${framework_executable} )
            file( REAL_PATH "${executable}" real_executable )
            list( APPEND framework_binaries "${real_executable}" )
        endforeach()
    endforeach()

    list( REMOVE_DUPLICATES framework_binaries )

    list( APPEND items ${framework_binaries} )

    foreach( item ${items} )
        message( "Refixing prereqs for '${item}'" )
        refix_prereqs( ${item} )

        if( "${item}" MATCHES "/Contents/Frameworks/[^/]+\\.dylib$" )
            delete_all_rpaths( ${item} )
            add_loader_rpath( ${item} )
        elseif( "${item}" MATCHES "/Contents/PlugIns/3d/[^/]+\\.so$" )
            delete_all_rpaths( ${item} )
            refix_rpaths( ${item} "@loader_path" )
        endif()
    endforeach( )

    foreach( binary ${appex_binaries} )
        if( NOT IS_DIRECTORY "${binary}" )
            message( "Refixing nested component '${binary}'" )
            refix_prereqs( ${binary} )
            delete_all_rpaths( ${binary} )
            refix_rpaths( ${binary} )
        endif()
    endforeach()

    foreach( binary ${framework_binaries} )
        if( NOT IS_DIRECTORY "${binary}" )
            message( "Refixing nested framework '${binary}'" )
            refix_prereqs( ${binary} )
            delete_all_rpaths( ${binary} )
            refix_rpaths( ${binary} "@loader_path" )
        endif()
    endforeach( )

    # For binaries, we need to fix the prereqs and the rpaths
    file( GLOB subdirs ${target}/Contents/Applications/*.app )
    foreach( subdir ${subdirs} )
        file( GLOB binaries ${subdir}/Contents/MacOS/* )
        foreach( binary ${binaries} )
            message( "Refixing rpaths and prereqs for '${binary}'" )
            #refix_rpaths( ${binary} )
            refix_prereqs( ${binary} )
        endforeach( )
    endforeach( )

    file( GLOB binaries ${target}/Contents/MacOS/* )
    foreach( binary ${binaries} )
        message( "Refixing prereqs for '${binary}'" )
        refix_prereqs( ${binary} )
        if( ${binary} MATCHES "kicad-cli" )
            message( "Refixing rppath for '${binary}'" )
            delete_all_rpaths( ${binary} )
            refix_rpaths( ${binary} )
        endif()
    endforeach( )

    message( "Removing Python pyc files" )
    file( GLOB_RECURSE pycs ${target}/*.pyc )
    file( REMOVE ${pycs}  )

    string( TIMESTAMP end_time )
    # message( "Refixing start time: ${start_time}\nRefixing end time: ${end_time}" )
endfunction( )


function( delete_all_rpaths BINARY_PATH )
    message(STATUS "Inspecting RPATHs for: ${BINARY_PATH}")

    execute_process(
            COMMAND otool -l ${BINARY_PATH}
            OUTPUT_VARIABLE OTOOL_OUTPUT
            RESULT_VARIABLE OTOOL_RESULT
            ERROR_QUIET
    )
    if(NOT "${OTOOL_RESULT}" STREQUAL "0")
        message(WARNING "Failed to run otool on ${BINARY_PATH}. Skipping.")
        return()
    endif()

    # Parse the output to find lines containing 'path <value> (offset'
    # otool output format for RPATH usually looks like:
    #     path @rpath/libs (offset 12)
    string(REGEX MATCHALL "path [^\n]+ \\(offset" RAW_MATCHES "${OTOOL_OUTPUT}")

    # Iterate over matches and remove them
    foreach(MATCH ${RAW_MATCHES})
        # Clean the string: remove "path " prefix and " (offset" suffix
        string(REGEX REPLACE "^path[ \t]*" "" RPATH_VAL "${MATCH}")
        string(REGEX REPLACE "[ \t]*\\(offset$" "" RPATH_VAL "${RPATH_VAL}")

        # Strip any remaining whitespace
        string(STRIP "${RPATH_VAL}" RPATH_VAL)

        message(STATUS "Deleting RPATH: '${RPATH_VAL}'")

        # 5. Execute install_name_tool to delete the specific RPATH
        execute_process(
                COMMAND install_name_tool -delete_rpath "${RPATH_VAL}" "${BINARY_PATH}"
                RESULT_VARIABLE INT_RESULT
                ERROR_VARIABLE INT_ERR
        )

        if(NOT "${INT_RESULT}" STREQUAL "0")
            message(WARNING "Error removing rpath '${RPATH_VAL}': ${INT_ERR}")
        endif()
    endforeach()
endfunction()

function( add_loader_rpath binary )
    execute_process(
            COMMAND install_name_tool -add_rpath "@loader_path" "${binary}"
            RESULT_VARIABLE add_rpath_rv
            ERROR_VARIABLE add_rpath_ev
    )

    if( NOT add_rpath_rv STREQUAL "0" )
        message( FATAL_ERROR "adding loader rpath failed: ${add_rpath_rv}\n${add_rpath_ev}" )
    endif()
endfunction()

function( refix_rpaths binary )
    get_filename_component( executable_path ${binary} DIRECTORY )

    set( rpath_base "@executable_path" )

    if( ARGC GREATER 1 )
        set( rpath_base "${ARGV1}" )
    endif()

    set( desired_rpaths )
    file( RELATIVE_PATH relative_kicad_framework_path ${executable_path} ${target}/Contents/Frameworks )
    string( REGEX REPLACE "/+$" "" relative_kicad_framework_path "${relative_kicad_framework_path}" ) # remove trailing slash
    list( APPEND desired_rpaths "${rpath_base}/${relative_kicad_framework_path}" )

    foreach( desired_rpath ${desired_rpaths} )
        execute_process(
                COMMAND install_name_tool -add_rpath ${desired_rpath} ${binary}
                RESULT_VARIABLE add_rpath_rv
                OUTPUT_VARIABLE add_rpath_ov
                ERROR_VARIABLE add_rpath_ev
        )
        if( NOT add_rpath_rv STREQUAL "0" )
            message( FATAL_ERROR "adding rpath failed: ${add_rpath_rv}\n${add_rpath_ev}" )
        endif( )
    endforeach( )
endfunction( )

function( refix_prereqs target )
    # file(GET_RUNTIME_DEPENDENCIES) does not seem to work properly on libraries, it returns empty
    # results.  So, to figure out which ones we can remap to rpath, we make use of ${items}, which
    # happens to contain all the shared libs we found in the bundle.  This is a big hack, because
    # we're not actually checking that these shared libs live *in* the rpath, but in practice it
    # should work.  If this stops being the case, we can always add more logic...

    execute_process(
            COMMAND otool -L ${target}
            RESULT_VARIABLE gp_rv
            OUTPUT_VARIABLE gp_cmd_ov
            ERROR_VARIABLE gp_ev
    )

    if( NOT gp_rv STREQUAL "0" )
        message( FATAL_ERROR "otool failed: ${gp_rv}\n${gp_ev}" )
    else()
        message( DEBUG "otool -L ${target} returned: ${gp_cmd_ov}" )
    endif( )

    string( REPLACE ";" "\\;" candidates "${gp_cmd_ov}" )
    string( REPLACE "\n" "${eol_char};" candidates "${candidates}" )
    # check for install id and remove it from list, since otool -L can include a
    # reference to itself
    set( gp_install_id )
    execute_process(
            COMMAND otool -D ${target}
            RESULT_VARIABLE otool_rv
            OUTPUT_VARIABLE gp_install_id_ov
            ERROR_VARIABLE otool_ev
    )
    if( NOT otool_rv STREQUAL "0" )
        message( FATAL_ERROR "otool -D failed: ${otool_rv}\n${otool_ev}" )
    endif()
    # second line is install name
    string( REGEX REPLACE ".*:\n" "" gp_install_id "${gp_install_id_ov}" )
    if( gp_install_id )
        # trim
        string( REGEX MATCH "[^\n ].*[^\n ]" gp_install_id "${gp_install_id}" )
    endif( )

    set( changes "" )

    set( otool_regex "^\t([^\t]+) \\(compatibility version ([0-9]+.[0-9]+.[0-9]+), current version ([0-9]+.[0-9]+.[0-9]+)\\)${eol_char}$" )

    foreach( candidate ${candidates} )
        if( "${candidate}" MATCHES "${otool_regex}" )
            string( REGEX REPLACE "${otool_regex}" "\\1" raw_prereq "${candidate}" )

            message( DEBUG "processing ${raw_prereq}")
            get_filename_component( prereq_name ${raw_prereq} NAME )
            message( DEBUG "    prereq name: ${prereq_name}" )
            set( changed_prereq "" )

            foreach( item ${items} )
                get_filename_component( item_name ${item} NAME )
                if( "${item_name}" STREQUAL "${prereq_name}" )
                    message( DEBUG "    found match at ${item}" )

                    if( item MATCHES "^.*/Contents/PlugIns/.*" )
                        string( REGEX REPLACE "^.*/Contents/PlugIns/(.*)$"
                                "@rpath/../PlugIns/\\1"
                                changed_prereq
                                "${item}" )
                    else()
                        set( changed_prereq "@rpath/${item_name}" )
                    endif()
                endif()
            endforeach()

            # install_runtime_deps excludes Python, so it is never among the bundled items and
            # must not be treated as an unresolved dependency here either
            if( prereq_name STREQUAL "Python" )
                message( DEBUG "    Python is provided by the host; left as is" )
                continue()
            endif()

            if( raw_prereq MATCHES "^@rpath.*" )
                if( "${changed_prereq}" STREQUAL "" )
                    message( FATAL_ERROR "Unresolved @rpath dependency '${raw_prereq}' in '${target}'" )
                endif()

                continue()
            endif()

            if( "${changed_prereq}" STREQUAL "" )
                if( raw_prereq MATCHES "^/" AND NOT raw_prereq MATCHES "^/(System/Library|usr/lib)/" )
                    message( FATAL_ERROR "Unresolved non-system dependency '${raw_prereq}' in '${target}'" )
                endif()

                message( DEBUG "    not found in bundle; assumed to be a system library" )
                continue()
            endif()

            # Because of the above continue()s, we know we changed the prereq if we're here

            if( raw_prereq STREQUAL gp_install_id )
                set( cmd install_name_tool -id ${changed_prereq} "${target}" )
                message( DEBUG "     updating install id: ${cmd}" )
                execute_process( COMMAND ${cmd} RESULT_VARIABLE install_name_tool_result )
                if( NOT install_name_tool_result EQUAL 0 )
                    string( REPLACE ";" "' '" msg "'${cmd}'" )
                    message( FATAL_ERROR "Command failed setting install id:\n ${msg}" )
                endif( )
                continue( )
            endif( )

            if ( NOT raw_prereq STREQUAL changed_prereq )
                # we know we need to change this prereq
                set( changes ${changes} "-change" "${raw_prereq}" "${changed_prereq}" )
            endif( )
        endif( )
    endforeach( )

    if( changes )
        set( cmd install_name_tool ${changes} "${target}" )
        message( DEBUG "changing prereqs: ${changes}" )
        execute_process( COMMAND ${cmd} RESULT_VARIABLE install_name_tool_result )
        if( NOT install_name_tool_result EQUAL 0 )
            string( REPLACE ";" "' '" msg "'${cmd}'" )
            message( FATAL_ERROR "Command failed:\n ${msg}" )
        endif( )
    endif( )
endfunction( )

cmake_policy( POP )
