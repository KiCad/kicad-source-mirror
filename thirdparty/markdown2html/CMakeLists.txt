# Add all the warnings to the files
if( COMPILER_SUPPORTS_WARNINGS )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARN_FLAGS_CXX}")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${WARN_FLAGS_C}")
endif()

include_directories( BEFORE ${INC_BEFORE} ${INC_AFTER} )

set( SUNDOWN_SRC
    md_parser/markdown.c
    md_parser/stack.c
    md_parser/buffer.c
    md_parser/autolink.c
    )

set( SUNDOWN_SRC_HTML
    html_formatter/html.c
    html_formatter/html_smartypants.c
    html_formatter/houdini_html_e.c
    html_formatter/houdini_href_e.c
    )

add_library(markdown_lib STATIC
            markdown2html.cpp
            ${SUNDOWN_SRC}
            ${SUNDOWN_SRC_HTML}
            )

target_include_directories( markdown_lib PRIVATE md_parser html_formatter )
