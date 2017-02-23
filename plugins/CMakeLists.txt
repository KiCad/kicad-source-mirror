# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

include_directories(
        ${GLM_INCLUDE_DIR}
        ${CMAKE_SOURCE_DIR}/include
        ${CMAKE_SOURCE_DIR}/3d-viewer
)

add_subdirectory( 3d )
