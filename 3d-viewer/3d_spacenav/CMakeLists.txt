if( UNIX AND NOT APPLE )
    add_library(3d-viewer_spacenav STATIC
        spnav_viewer_plugin.cpp
    )
    add_dependencies( 3d-viewer_spacenav pcbcommon )
    target_include_directories(3d-viewer_spacenav PRIVATE
        ${CMAKE_SOURCE_DIR}/common
        ${CMAKE_SOURCE_DIR}/3d-viewer
    )
    target_link_libraries(3d-viewer_spacenav
        PRIVATE
            kicommon
    )
endif()