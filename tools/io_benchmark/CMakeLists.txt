
include_directories( BEFORE ${INC_BEFORE} )

set( IOBENCHMARK_SRCS
    io_benchmark.cpp
)

add_executable( io_benchmark
    ${IOBENCHMARK_SRCS}
)

target_link_libraries( io_benchmark
    common
    qa_utils
    ${wxWidgets_LIBRARIES}
)

