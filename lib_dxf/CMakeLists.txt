include_directories(intern)

set(LIBDXF_SRCS
    libdxfrw.cpp
    intern/dxfwriter.cpp
	intern/drw_textcodec.cpp
	intern/dxfreader.cpp
	drw_objects.cpp
	drw_entities.cpp
    )

add_library(lib_dxf STATIC ${LIBDXF_SRCS})
