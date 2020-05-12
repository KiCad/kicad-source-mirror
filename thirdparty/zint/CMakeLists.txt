# copied from zint/backend, but modified a bit
set(zint_COMMON_SRCS backend/common.c backend/library.c backend/render.c backend/large.c backend/reedsol.c backend/gs1.c backend/eci.c)
set(zint_ONEDIM_SRCS backend/code.c backend/code128.c backend/2of5.c backend/upcean.c backend/telepen.c backend/medical.c backend/plessey.c backend/rss.c)
set(zint_POSTAL_SRCS backend/postal.c backend/auspost.c backend/imail.c backend/mailmark.c)
set(zint_TWODIM_SRCS backend/code16k.c backend/codablock.c backend/dmatrix.c backend/pdf417.c backend/qr.c backend/maxicode.c backend/composite.c backend/aztec.c backend/code49.c backend/code1.c backend/gridmtx.c backend/hanxin.c backend/dotcode.c)
set(zint_OUTPUT_SRCS backend/vector.c backend/render.c backend/ps.c backend/svg.c backend/emf.c backend/bmp.c backend/pcx.c backend/gif.c backend/png.c backend/tif.c backend/raster.c)
set(zint_SRCS ${zint_OUTPUT_SRCS} ${zint_COMMON_SRCS} ${zint_ONEDIM_SRCS} ${zint_POSTAL_SRCS} ${zint_TWODIM_SRCS})

add_definitions (-DNO_PNG)  # in fact, we do not care about output formats

add_library(zint STATIC ${zint_SRCS})

target_include_directories( zint
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )