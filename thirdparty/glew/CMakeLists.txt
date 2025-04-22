add_library( glew STATIC )

# Mark the include directory as private so that it doesn't get used by other targets
# and is only used when building the actual library.
# The actual include directories will be added to the global include paths as
# system headers
target_include_directories( glew PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include" )

# Definitions for compiling GLEW staticly for EGL (extracted from the main GLEW CMakeLists.txt file)
add_compile_definitions( GLEW_STATIC )
add_compile_definitions( GLEW_EGL )
add_compile_definitions( GLEW_NO_GLU )

target_sources( glew PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/glew.c
    )

target_link_libraries( glew PUBLIC
    ${OPENGL_LIBRARIES}
    ${OPENGL_egl_LIBRARY}
    )
