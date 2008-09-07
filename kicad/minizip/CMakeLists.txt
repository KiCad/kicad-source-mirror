find_package(ZLIB QUIET)
if(ZLIB_FOUND)
    message(STATUS "Check for installed zlib -- found")
else(ZLIB_FOUND)
    message(STATUS "Check for installed zlib -- not found")
    message(STATUS "Use wxWidgets zlib")
    # zlib is not installed, and in this case wxWidgets creates its own zlib library
    # include files are in ${wxWidgets_ROOT_DIR}/src/zlib
    # and the corresponding library is libwxzlib-<version>.a (like libwxzlib-2.8.a)
    # and we try to use it
    set(ZLIB_INCLUDE_DIR ${wxWidgets_ROOT_DIR}/src/zlib)
    set(ZLIB_LIBRARIES ${wxWidgets_ROOT_DIR}/lib/libwxzlib-2.8.a)
endif(ZLIB_FOUND)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}
                    ${ZLIB_INCLUDE_DIR})

set(MINIZIP_SRCS
    ioapi.c
    minizip.c
    zip.c)

add_executable(minizip ${MINIZIP_SRCS})

target_link_libraries(minizip ${ZLIB_LIBRARIES})

install(TARGETS minizip
        RUNTIME DESTINATION ${KICAD_BIN}
        COMPONENT binary)
