set(NANOSVG_SRCS
    nanosvg.cpp
    nanosvgrast.cpp
    )

add_library(nanosvg STATIC ${NANOSVG_SRCS})

target_include_directories( nanosvg
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

target_link_libraries( nanosvg PRIVATE Freetype::Freetype Fontconfig::Fontconfig HarfBuzz::HarfBuzz )
