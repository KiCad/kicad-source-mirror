FIND_PACKAGE(ZLIB)
IF(ZLIB_FOUND)
	MESSAGE(STATUS "Check for installed ZLIB -- found")
ELSE(ZLIB_FOUND)
	MESSAGE(STATUS "Check for installed zlib -- not found")
	MESSAGE(FATAL_ERROR "zlib was not found - it is required to build KiCad")
ENDIF(ZLIB_FOUND)

SET(MINIZIP_SRCS
	ioapi.c
	minizip.c
	zip.c)

ADD_EXECUTABLE(minizip ${MINIZIP_SRCS} )

TARGET_LINK_LIBRARIES(minizip ${ZLIB_LIBRARIES})

INSTALL(TARGETS minizip RUNTIME DESTINATION bin)
