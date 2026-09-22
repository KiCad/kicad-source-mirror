# Stages the preview handler's runtime DLLs beside it so it can be registered from a build tree.
#
# vcpkg's applocal step runs against the handler alone, before the KiCad DLLs are copied in, so
# whatever those pull in of their own is missing.  kicad_3d_import wants wxWidgets and its
# transitive expat/pcre2, none of which the handler itself imports.

file( MAKE_DIRECTORY "${DEST}" )

foreach( lib IN LISTS KICAD_LIBS )
    get_filename_component( _name "${lib}" NAME )
    file( COPY_FILE "${lib}" "${DEST}/${_name}" ONLY_IF_DIFFERENT )
endforeach()

file( GET_RUNTIME_DEPENDENCIES
    LIBRARIES ${HANDLER} ${KICAD_LIBS}
    RESOLVED_DEPENDENCIES_VAR _resolved
    UNRESOLVED_DEPENDENCIES_VAR _unresolved
    CONFLICTING_DEPENDENCIES_PREFIX _conflicting
    DIRECTORIES ${SEARCH_DIRS}
    PRE_EXCLUDE_REGEXES "api-ms-.*" "ext-ms-.*"
    POST_EXCLUDE_REGEXES ".*[Ww][Ii][Nn][Dd][Oo][Ww][Ss][/\\\\][Ss][Yy][Ss][Tt][Ee][Mm]32[/\\\\].*"
)

# vcpkg deploys the same DLL into several of the search directories, so a name reachable more
# than once is one artefact seen twice rather than a real ambiguity
foreach( _name IN LISTS _conflicting_FILENAMES )
    list( GET _conflicting_${_name} 0 _first )
    list( APPEND _resolved "${_first}" )
endforeach()

foreach( dep IN LISTS _resolved )
    get_filename_component( _name "${dep}" NAME )
    file( COPY_FILE "${dep}" "${DEST}/${_name}" ONLY_IF_DIFFERENT )
endforeach()

# Unresolved names are the API sets and system DLLs the regexes above did not catch, so they are
# reported rather than treated as an error
if( _unresolved )
    message( STATUS "3D preview handler: unresolved runtime dependencies: ${_unresolved}" )
endif()
