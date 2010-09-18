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
    find_path(ZLIB_INCLUDE_DIR zlib.h PATHS ${wxWidgets_ROOT_DIR}/../src/zlib/ ${wxWidgets_ROOT_DIR}/src/zlib/ DOC "location of zlib include files")
	find_file(ZLIB_LIBRARIES NAMES ${wxWidgets_LIB_DIR}/libwxzlib-2.8.a ZLIB_LIBRARIES NAMES ${wxWidgets_LIB_DIR}/libwxzlib-2.9.a libwxzlib.a PATHS ${wxWidgets_ROOT_DIR}/lib/ PATH_SUFFIXES gcc_dll DOC "location of wxzlib library file")
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
        DESTINATION ${KICAD_BIN}
        COMPONENT binary)
