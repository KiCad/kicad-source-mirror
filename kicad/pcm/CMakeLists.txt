# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

include_directories( BEFORE ${INC_BEFORE} )

add_compile_definitions( PCM )

set ( PCM_DLG_SRCS
    dialogs/dialog_manage_repositories_base.cpp
    dialogs/dialog_manage_repositories.cpp
    dialogs/dialog_pcm_base.cpp
    dialogs/dialog_pcm.cpp
    dialogs/dialog_pcm_progress_base.cpp
    dialogs/dialog_pcm_progress.cpp
    dialogs/panel_package_base.cpp
    dialogs/panel_package.cpp
    dialogs/panel_packages_view_base.cpp
    dialogs/panel_packages_view.cpp
)

set( PCM_SRCS
    ${PCM_DLG_SRCS}
    pcm.cpp
    pcm_data.cpp
    pcm_task_manager.cpp
)

add_library( pcm STATIC
    ${PCM_SRCS}
)

target_include_directories(
    pcm PUBLIC dialogs
)

target_link_libraries( pcm
    ${wxWidgets_LIBRARIES}
    common
    picosha2
)

# Copy the schema to the build directory when building outside the source tree
# to allow for proper running of kicad from the build directory.
if( NOT (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_CURRENT_BINARY_DIR} ) )
    file( GLOB SCHEMA_FILES ${CMAKE_CURRENT_SOURCE_DIR}/schemas/*.json )

    add_custom_target( schema_build_copy ALL
        COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_CURRENT_SOURCE_DIR}/schemas ${CMAKE_BINARY_DIR}/schemas
        DEPENDS ${SCHEMA_FILES}
        COMMENT "Copying schema files into build directory"
        )
endif()

INSTALL( DIRECTORY
    schemas
    DESTINATION ${KICAD_DATA}
    COMPONENT Runtime
)
