FIND_PACKAGE(ZLIB)
IF(ZLIB_FOUND)
	MESSAGE(STATUS "Check for installed ZLIB -- found")
ELSE(ZLIB_FOUND)
	MESSAGE(STATUS "Check for installed zlib -- not found")
    IF (NOT MINGW)
		MESSAGE(FATAL_ERROR
                 "zlib was not found - it is required to build KiCad")
	ELSE (NOT MINGW)
		# zlib is not installed, and in this case wxWidgets creates its own zlib library
		# include files are in ${wxWidgets_ROOT_DIR}/src/zlib
		#and the corresponding library is libwxzlib-<version>.a ( like libwxzlib-2.8.a)
		# and we try to use it
		INCLUDE_DIRECTORIES(${wxWidgets_ROOT_DIR}/src/zlib)
    ENDIF (NOT MINGW)
ENDIF(ZLIB_FOUND)

SET(MINIZIP_SRCS
	ioapi.c
	minizip.c
	zip.c)

ADD_EXECUTABLE(minizip ${MINIZIP_SRCS} )

TARGET_LINK_LIBRARIES(minizip ${ZLIB_LIBRARIES}  ${wxWidgets_LIBRARIES})

INSTALL(TARGETS minizip RUNTIME DESTINATION bin COMPONENT binary)
