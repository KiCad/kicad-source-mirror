cmake_minimum_required(VERSION 3.5)

set(GLAD_SOURCES
	${CMAKE_CURRENT_SOURCE_DIR}/src/gl.c
)

if(WIN32)
	list(APPEND GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/wgl.c)
elseif(UNIX AND NOT APPLE)
	if(wxWidgets_HAS_GLX OR wxWidgets_HAS_EGL)
		# wx 3.3.2+ macros
		if(wxWidgets_HAS_EGL)
			list(APPEND GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/egl.c)
		endif()

		if(wxWidgets_HAS_GLX)
			list(APPEND GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/glx.c)
		endif()
	else()
		# Older wx macro (runtime GLX/EGL switch not supported)
		if(wxWidgets_GLCANVAS_EGL)
			list(APPEND GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/egl.c)
		else()
			list(APPEND GLAD_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/glx.c)
		endif()
	endif()
endif()

add_library(glad ${GLAD_SOURCES})

# Avoid linking Xlib directly, we don't pass a null display anyway.
target_compile_definitions(glad PRIVATE GLAD_GLX_NO_X11)

target_include_directories(glad PUBLIC
	$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
	$<INSTALL_INTERFACE:include>
)

if (WIN32)
	target_link_libraries(glad PUBLIC opengl32)
endif()