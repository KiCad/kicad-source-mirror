
include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${INC_AFTER}
    )

set( WEBVIEW_SRCS
    html_link_parser.cpp
    webviewer.cpp
)

add_library( webviewer STATIC ${WEBVIEW_SRCS})

add_dependencies( webviewer lib-dependencies )

