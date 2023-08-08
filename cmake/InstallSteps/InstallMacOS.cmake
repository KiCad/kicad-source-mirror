# Copies the runtime dependencies for a given target into the bundle

function( install_runtime_deps exe libs dest )
    # set(CMAKE_MESSAGE_LOG_LEVEL DEBUG)
    message( DEBUG "install_runtime_deps ${exe}\n  libs: ${libs}\n  dest: ${dest}" )

    file( GET_RUNTIME_DEPENDENCIES
        LIBRARIES ${libs}
        EXECUTABLES ${exe}
        RESOLVED_DEPENDENCIES_VAR _r_deps
        UNRESOLVED_DEPENDENCIES_VAR _u_deps
        POST_EXCLUDE_FILES Python
    )

    if( "${dest}" STREQUAL "" )
        set( dest "${OSX_BUNDLE_INSTALL_LIB_DIR}" )
        message( DEBUG ".... Updated dest to ${dest}" )
    endif()

    foreach( _file ${_r_deps} )
        message( DEBUG ".... install dep ${_file}" )
        file(INSTALL
            DESTINATION "${dest}"
            TYPE SHARED_LIBRARY
            FOLLOW_SYMLINK_CHAIN
            FILES "${_file}"
        )
    endforeach()

#    list(LENGTH _u_deps _u_length)
#    if("${_u_length}" GREATER 0)
#        message(WARNING "Unresolved dependencies detected! ${_u_deps}")
#    endif()
endfunction()