# unset CMAKE_CXX_FLAGS because it's contaminated with too many options
set( CMAKE_CXX_FLAGS "" )
add_subdirectory( sg )
