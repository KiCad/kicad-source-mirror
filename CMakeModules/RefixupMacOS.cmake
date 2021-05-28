# RefixupMacOS.cmake

# Adjust rpaths and dependency information on macOS
# after fixup_bundle.

# Some of this comes from GetPrerequisites.cmake.

# This is not intended to make an install completely
# redistributable and relocatable.

# TODO: Check style things

function( refix_kicad_bundle target )
    # target should be the path to the kicad.app directory

    string( TIMESTAMP start_time )

    cleanup_python( ${target} )

    file( GLOB_RECURSE items ${target}/*.dylib ${target}/*.so ${target}/*.kiface )

    foreach( item ${items} )
        message( "Refixing '${item}'" )
        refix_prereqs( ${item} )
    endforeach( )

    # For binaries, we need to fix the prereqs and the rpaths
    file( GLOB subdirs ${target}/Contents/Applications/*.app )
    foreach( subdir ${subdirs} )
        file( GLOB binaries ${subdir}/Contents/MacOS/* )
        foreach( binary ${binaries} )
            message( "Refixing '${binary}'" )
            refix_rpaths( ${binary} )
            refix_prereqs( ${binary} )
        endforeach( )
    endforeach( )

    file( GLOB pythonbinbinaries ${target}/Contents/Frameworks/Python.framework/Versions/3.*/bin/python3 )
    foreach( pythonbinbinary ${pythonbinbinaries} )
        message( "Refixing '${pythonbinbinary}'" )
        refix_rpaths( ${pythonbinbinary} )
        refix_prereqs( ${pythonbinbinary} )
    endforeach()

    file( GLOB pythonresbinaries ${target}/Contents/Frameworks/Python.framework/Versions/3.*/Resources/Python.app/Contents/MacOS/Python )
    foreach( pythonresbinary ${pythonresbinaries} )
        message( "Refixing '${pythonresbinary}'" )
        refix_rpaths( ${pythonresbinary} )
        refix_prereqs( ${pythonresbinary} )
    endforeach()

    file( GLOB binaries ${target}/Contents/MacOS/* )
    foreach( binary ${binaries} )
        message( "Refixing '${binary}'" )
        refix_rpaths( ${binary} )
        refix_prereqs( ${binary} )
    endforeach( )

    string( TIMESTAMP end_time )
    # message( "Refixing start time: ${start_time}\nRefixing end time: ${end_time}" )
endfunction( )

function( cleanup_python bundle)
    # Remove extra Python
    file( REMOVE_RECURSE ${bundle}/Contents/MacOS/Python )
    # Make sure Python's Current is a symlink to 3.x
    file( REMOVE_RECURSE ${bundle}/Contents/Frameworks/Python.framework/Versions/Current )
    file( GLOB python_version LIST_DIRECTORIES true  RELATIVE ${bundle}/Contents/Frameworks/Python.framework/Versions ${bundle}/Contents/Frameworks/Python.framework/Versions/3* )
    execute_process( COMMAND ln -s ${python_version} ${bundle}/Contents/Frameworks/Python.framework/Versions/Current )
endfunction()

function( refix_rpaths binary )
    get_filename_component( executable_path ${binary} DIRECTORY )

    set( desired_rpaths )
    file( RELATIVE_PATH relative_kicad_framework_path ${executable_path} ${target}/Contents/Frameworks )
    string( REGEX REPLACE "/+$" "" relative_kicad_framework_path "${relative_kicad_framework_path}" ) # remove trailing slash
    file( RELATIVE_PATH relative_python_framework_path ${executable_path} ${target}/Contents/Frameworks/Python.framework )
    string( REGEX REPLACE "/+$" "" relative_python_framework_path "${relative_python_framework_path}" ) # remove trailing slash
    list( APPEND desired_rpaths "@executable_path/${relative_kicad_framework_path}" "@executable_path/${relative_python_framework_path}" )

    get_item_rpaths( ${binary} old_rpaths )
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
    # Replace '@executable_path/../Frameworks/' in dependencies with '@rpath/'
    execute_process(
            COMMAND otool -L ${target}
            RESULT_VARIABLE gp_rv
            OUTPUT_VARIABLE gp_cmd_ov
            ERROR_VARIABLE gp_ev
    )

    if( NOT gp_rv STREQUAL "0" )
        message( FATAL_ERROR "otool failed: ${gp_rv}\n${gp_ev}" )
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
        if( "${candidate}" MATCHES "${gp_regex}" )
            string( REGEX REPLACE "${otool_regex}" "\\1" raw_prereq "${candidate}" )

            if ( raw_prereq MATCHES "^@executable_path/\\.\\./\\.\\./.*/Contents/MacOS/Python$" )
                set( changed_prereq "@rpath/Versions/Current/Python" )
            elseif ( raw_prereq MATCHES "^@executable_path/\\.\\./Frameworks/" )
                string( REPLACE "@executable_path/../Frameworks/"
                        "@rpath/" changed_prereq
                        "${raw_prereq}" )
            elseif ( raw_prereq MATCHES "^@executable_path/\\.\\./PlugIns/" )
                string( REPLACE "@executable_path/../PlugIns/"
                        "@rpath/../PlugIns/" changed_prereq
                        "${raw_prereq}" )
            else( )
                continue( )
            endif( )

            # Because of the above continue( ) in the else, we know we changed the prereq if we're here

            if( raw_prereq STREQUAL gp_install_id )
                set( cmd install_name_tool -id ${changed_prereq} "${target}" )
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
        execute_process( COMMAND ${cmd} RESULT_VARIABLE install_name_tool_result )
        if( NOT install_name_tool_result EQUAL 0 )
            string( REPLACE ";" "' '" msg "'${cmd}'" )
            message( FATAL_ERROR "Command failed:\n ${msg}" )
        endif( )
    endif( )
endfunction( )