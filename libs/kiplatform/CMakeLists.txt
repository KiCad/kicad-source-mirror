
# Add the appropriate source files
if( APPLE )
    set( PLATFORM_SRCS
        osx/ui.mm
        )

    set( PLATFORM_LIBS
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreData"
        "-framework Foundation"
        )
elseif( WIN32 )
    set( PLATFORM_SRCS
        msw/ui.cpp
        )
elseif( UNIX )
    set( PLATFORM_SRCS
        gtk/ui.cpp
        )
endif()


add_library( kiplatform STATIC
    ${PLATFORM_SRCS}
    )

target_include_directories( kiplatform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries( kiplatform
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    )
