# Copies a dependency once under its real filename and preserves the requested
# basename as a bundle-local alias.
function( install_runtime_file file dest )
    file( REAL_PATH "${file}" _real_file )
    get_filename_component( _real_name "${_real_file}" NAME )
    get_filename_component( _file_name "${file}" NAME )
    file( MAKE_DIRECTORY "${dest}" )
    file( COPY_FILE "${_real_file}" "${dest}/${_real_name}" ONLY_IF_DIFFERENT )
    file( CHMOD "${dest}/${_real_name}"
          PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE )

    if( NOT _file_name STREQUAL _real_name )
        file( REMOVE "${dest}/${_file_name}" )
        file( CREATE_LINK "${_real_name}" "${dest}/${_file_name}" SYMBOLIC )
    endif()
endfunction()

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
        install_runtime_file( "${_file}" "${dest}" )
    endforeach()

#    list(LENGTH _u_deps _u_length)
#    if("${_u_length}" GREATER 0)
#        message(WARNING "Unresolved dependencies detected! ${_u_deps}")
#    endif()
endfunction()