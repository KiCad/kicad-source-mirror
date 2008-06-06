add_executable(boolonly boolonly.cpp)

if(WIN32)
    add_definitions(-D_MSWVC_)
else(WIN32)
    add_definitions(-D__UNIX__)
endif(WIN32)

include_directories(${kbool_SOURCE_DIR}/..)

target_link_libraries(boolonly kbool)
