IF(WIN32)
    ADD_DEFINITIONS( -D_MSWVC_ )
ELSE(WIN32)
    ADD_DEFINITIONS( -D__UNIX__  )
ENDIF(WIN32)

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

ADD_LIBRARY( kbool ${KBOOL_SRCS})

