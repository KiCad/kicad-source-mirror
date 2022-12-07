set(TINYSPLINE_LIB_SRCS
    tinyspline.c
    tinysplinecxx.cxx
    parson.c
    )

add_library(tinyspline_lib STATIC ${TINYSPLINE_LIB_SRCS})

target_include_directories( tinyspline_lib
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
