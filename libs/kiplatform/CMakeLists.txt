# Add the appropriate source files
if( APPLE )
    set( PLATFORM_SRCS
        osx/app.mm
        osx/environment.mm
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
        msw/app.cpp
        msw/environment.cpp
        msw/ui.cpp
        )
elseif( UNIX )
    set( PLATFORM_SRCS
        gtk/app.cpp
        gtk/environment.cpp
        gtk/ui.cpp
        )

    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)

    # Detect GTK3 and configure it
    set( PLATFORM_LIBS
        ${GTK3_LIBRARIES}
        )

    include_directories( SYSTEM ${GTK3_INCLUDE_DIRS} )
    link_directories( ${GTK3_LIBRARY_DIRS} )
    add_definitions( ${GTK3_CFLAGS_OTHER} )
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
