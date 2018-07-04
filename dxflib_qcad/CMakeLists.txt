set(LIBDXF_SRCS
    dl_dxf.cpp
    dl_writer_ascii.cpp
    tinyspline_lib/tinyspline.c
    tinyspline_lib/tinysplinecpp.cpp
    )

add_library(lib_dxf STATIC ${LIBDXF_SRCS})
