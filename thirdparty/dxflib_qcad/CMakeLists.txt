set(DXFLIB_QCAD_SRCS
    dl_dxf.cpp
    dl_writer_ascii.cpp
    )

add_library(dxflib_qcad STATIC ${DXFLIB_QCAD_SRCS})

target_include_directories( dxflib_qcad
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )
