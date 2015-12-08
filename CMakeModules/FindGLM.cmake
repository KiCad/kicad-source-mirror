if( ${CMAKE_MAJOR_VERSION} STREQUAL "2" AND ${CMAKE_MINOR_VERSION} STREQUAL "8"
    AND ( ${CMAKE_PATCH_VERSION} STREQUAL "2" OR ${CMAKE_PATCH_VERSION} STREQUAL "7"
    OR ${CMAKE_PATCH_VERSION} STREQUAL "10" ) )
    
    message( FATAL_ERROR "\nThis version of CMake is known to not work\n"
        "Known bad versions: 2.8.2, 2.8.7, 2.8.10\n" )
    
endif()


find_path( GLM_INCLUDE_DIR glm.hpp PATH_SUFFIXES glm )


if( NOT ${GLM_INCLUDE_DIR} STREQUAL "" )
   
    # attempt to extract the GLM Version information from setup.hpp
    find_file( GLM_SETUP setup.hpp PATHS ${GLM_INCLUDE_DIR} PATH_SUFFIXES core detail NO_DEFAULT_PATH )

    if( GLM_SETUP )
        # extract the "#define GLM_VERSION*" lines
        file( STRINGS ${GLM_SETUP} _version REGEX "^#define.*GLM_VERSION.*" )
        
        foreach( SVAR ${_version} )
            string( REGEX MATCH GLM_VERSION_[M,A,J,O,R,I,N,P,T,C,H,E,V,I,S]* _VARNAME ${SVAR} )
            string( REGEX MATCH [0-9]+ _VALUE ${SVAR} )
            
            if( NOT ${_VARNAME} STREQUAL "" AND NOT ${_VALUE} STREQUAL "" )
                set( _${_VARNAME} ${_VALUE} )
            endif()
            
        endforeach()
        
        #ensure that NOT GLM_VERSION* will evaluate to '0'
        if( NOT _GLM_VERSION_MAJOR )
            set( _GLM_VERSION_MAJOR 0 )
        endif()

        if( NOT _GLM_VERSION_MINOR )
            set( _GLM_VERSION_MINOR 0 )
        endif()

        if( NOT _GLM_VERSION_PATCH )
            set( _GLM_VERSION_PATCH 0 )
        endif()

        if( NOT _GLM_VERSION_REVISION )
            set( _GLM_VERSION_REVISION 0 )
        endif()

        set( GLM_VERSION ${_GLM_VERSION_MAJOR}.${_GLM_VERSION_MINOR}.${_GLM_VERSION_PATCH}.${_GLM_VERSION_REVISION} )
        unset( GLM_SETUP CACHE )
        
    endif( GLM_SETUP )
    
endif( NOT ${GLM_INCLUDE_DIR} STREQUAL "" )


include( FindPackageHandleStandardArgs )
FIND_PACKAGE_HANDLE_STANDARD_ARGS( GLM
    REQUIRED_VARS GLM_INCLUDE_DIR
    VERSION_VAR GLM_VERSION )

    
mark_as_advanced( GLM_INCLUDE_DIR )
set( GLM_VERSION_MAJOR ${_GLM_VERSION_MAJOR} CACHE INTERNAL "" )
set( GLM_VERSION_MINOR ${_GLM_VERSION_MINOR} CACHE INTERNAL "" )
set( GLM_VERSION_PATCH ${_GLM_VERSION_PATCH} CACHE INTERNAL "" )
set( GLM_VERSION_TWEAK ${_GLM_VERSION_REVISION} CACHE INTERNAL "" )
