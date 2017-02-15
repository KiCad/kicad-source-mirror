
include_directories( BEFORE ${INC_BEFORE} )

set( IOBENCHMARK_SRCS
    io_benchmark.cpp
    stdstream_line_reader.cpp
)

add_executable( io_benchmark
    ${IOBENCHMARK_SRCS}
)

target_link_libraries( io_benchmark
    common
    ${wxWidgets_LIBRARIES}
)

