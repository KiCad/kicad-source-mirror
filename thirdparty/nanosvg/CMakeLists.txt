set(NANOSVG_SRCS
    nanosvg.cpp
    )

add_library(nanosvg STATIC ${NANOSVG_SRCS})

target_include_directories( nanosvg
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
