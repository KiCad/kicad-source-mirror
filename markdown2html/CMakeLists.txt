# .cpp files are compiled with extra ${WSHADOW_FLAGS}
if( COMPILER_SUPPORTS_WSHADOW )
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WSHADOW_FLAGS}")
endif()

set( USE_MADDY_CONVERTER False )

include_directories( BEFORE ${INC_BEFORE} ${INC_AFTER} )

# Set to True to use maddy converter, False to use sundowm converter
if( USE_MADDY_CONVERTER )
    add_definitions( -DUSE_MADDY_CONVERTER )
    add_library ( markdown_lib STATIC
                  markdown2html.cpp
                )
else()

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
endif()
