# Eigen is a header-only template library. Expose it SYSTEM so its headers do
# not raise warnings in, or leak their own diagnostics into, the consumers that
# include <Eigen/...>. Only planegcs uses it today.
add_library( eigen INTERFACE )

target_include_directories( eigen SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )
