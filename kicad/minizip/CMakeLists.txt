# TODO:
# * Add include for wxzlib

SET(MINIZIP_SRCS
	ioapi.c
	minizip.c
	zip.c)

ADD_EXECUTABLE(minizip ${MINIZIP_SRCS})

INSTALL(TARGETS minizip RUNTIME DESTINATION bin)
