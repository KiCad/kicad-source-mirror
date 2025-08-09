# copied from zint/backend, but modified a bit
set(zint_COMMON_SRCS backend/common.c backend/eci.c backend/filemem.c backend/general_field.c backend/gs1.c backend/large.c backend/library.c backend/reedsol.c)
set(zint_ONEDIM_SRCS backend/2of5.c backend/2of5inter.c backend/2of5inter_based.c backend/bc412.c backend/channel.c backend/codabar.c backend/code.c backend/code11.c backend/code128.c
                     backend/code128_based.c backend/dxfilmedge.c backend/medical.c backend/plessey.c backend/rss.c backend/telepen.c backend/upcean.c)
set(zint_POSTAL_SRCS backend/auspost.c backend/imail.c backend/mailmark.c backend/postal.c)
set(zint_TWODIM_SRCS backend/aztec.c backend/codablock.c backend/code1.c backend/code16k.c backend/code49.c backend/composite.c backend/dmatrix.c backend/dotcode.c backend/gridmtx.c
                     backend/hanxin.c backend/maxicode.c backend/pdf417.c backend/qr.c backend/ultra.c)

set(zint_OUTPUT_SRCS backend/output.c backend/ps.c backend/raster.c backend/vector.c)
set(zint_OUTPUT_SRCS ${zint_OUTPUT_SRCS} backend/bmp.c backend/gif.c backend/png.c backend/pcx.c backend/svg.c backend/tif.c backend/emf.c )
set(zint_SRCS ${zint_OUTPUT_SRCS} ${zint_COMMON_SRCS} ${zint_ONEDIM_SRCS} ${zint_POSTAL_SRCS} ${zint_TWODIM_SRCS})

add_definitions (-DZINT_NO_PNG)  # in fact, we do not care about output formats

add_library(zint STATIC ${zint_SRCS})

target_include_directories( zint
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        )