option( KICAD_SHOW_GTK_MESSAGES
    "Show all the GTK error messages in the terminal"
    OFF )

mark_as_advanced( KICAD_SHOW_GTK_MESSAGES )

set( PLATFORM_COMPILE_DEFS "" )

# Add port-specific files and definitions
if( KICAD_WX_PORT STREQUAL osx )
    set( PLATFORM_SRCS
        port/wxosx/ui.mm
        )
elseif( KICAD_WX_PORT STREQUAL msw )
    set( PLATFORM_SRCS
        port/wxmsw/ui.cpp
        )
elseif( KICAD_WX_PORT STREQUAL gtk )
    set( PLATFORM_SRCS
        port/wxgtk/ui.cpp
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

    # Add the definition to show the GTK error messages if desired (they are hidden
    # by us by default)
    if( KICAD_SHOW_GTK_MESSAGES )
        message( STATUS "Configuring KiCad not to hide any GTK error messages" )
        string( APPEND PLATFORM_COMPILE_DEFS "-DKICAD_SHOW_GTK_MESSAGES" )
    endif()
endif()


# Add OS-specific files and definitions
if( APPLE )
    list( APPEND PLATFORM_SRCS
        os/apple/app.mm
        os/apple/drivers.mm
        os/apple/environment.mm
        os/apple/io.mm
        os/apple/policy.mm
        os/apple/secrets.mm
        os/apple/sysinfo.cpp
        os/apple/printing.mm
        )

    set( PLATFORM_LIBS
        "-framework Cocoa"
        "-framework AppKit"
        "-framework CoreData"
        "-framework Foundation"
        "-framework PDFKit"
        )
elseif( WIN32 )
    list( APPEND PLATFORM_SRCS
        os/windows/app.cpp
        os/windows/drivers.cpp
        os/windows/environment.cpp
        os/windows/io.cpp
        os/windows/policy.cpp
        os/windows/secrets.cpp
        os/windows/sysinfo.cpp
        os/windows/printing.cpp
        )

    set( PLATFORM_LIBS
        "shlwapi"
        "winhttp"
        "wintrust"
        "imm32"
        "windowsapp"
        )
elseif( UNIX )
    list( APPEND PLATFORM_SRCS
        os/unix/app.cpp
        os/unix/drivers.cpp
        os/unix/environment.cpp
        os/unix/io.cpp
        os/unix/policy.cpp
        os/unix/secrets.cpp
        os/unix/sysinfo.cpp
        os/unix/printing.cpp
        )

    # Detect the secret library and configure it
    pkg_check_modules(secret REQUIRED libsecret-1)
    include_directories( SYSTEM ${secret_INCLUDE_DIRS} )
    list( APPEND PLATFORM_LIBS ${secret_LIBRARIES} )

    if( KICAD_WAYLAND )
        find_package(Wayland 1.20 COMPONENTS Client REQUIRED)

        list( APPEND PLATFORM_SRCS port/wxgtk/wayland-pointer-constraints-unstable-v1.c )
        list( APPEND PLATFORM_LIBS Wayland::Client  )
    endif()
endif()

include_directories(
    ${INC_AFTER}
    )

add_library( kiplatform STATIC
    ${PLATFORM_SRCS}
    )

target_compile_definitions( kiplatform PRIVATE
    ${PLATFORM_COMPILE_DEFS}
    )

target_include_directories( kiplatform PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    )

target_link_libraries( kiplatform
    core
    ${wxWidgets_LIBRARIES}
    ${PLATFORM_LIBS}
    )

if( APPLE )
    find_library(COREFOUNDATION_LIBRARY CoreFoundation REQUIRED)
    find_library(SECURITY_LIBRARY Security REQUIRED)

    target_link_libraries( kiplatform
            ${COREFOUNDATION_LIBRARY}
            ${SECURITY_LIBRARY}
    )
endif()

if( WIN32 )
add_dependencies( kiplatform
    version_header
    )
endif()
