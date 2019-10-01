project(markdown)

add_library ( markdown_lib STATIC
    markdown2html.cpp
    )

include_directories( BEFORE ${INC_BEFORE} ${INC_AFTER} )
