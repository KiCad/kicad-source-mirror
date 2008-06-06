if(WIN32)
    add_definitions(-D_MSWVC_)
else(WIN32)
    add_definitions(-D__UNIX__)
endif(WIN32)

include_directories(${kbool_SOURCE_DIR}/include)

set(KBOOL_SRCS
    booleng.cpp
    graph.cpp
    graphlst.cpp
    line.cpp
    link.cpp
    lpoint.cpp
    node.cpp
    record.cpp
    scanbeam.cpp)

add_library(kbool ${KBOOL_SRCS})
