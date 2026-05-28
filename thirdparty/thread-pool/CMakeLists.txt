add_library( thread-pool INTERFACE )

target_include_directories( thread-pool INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

# Enable native extensions
target_compile_definitions( thread-pool INTERFACE BS_THREAD_POOL_NATIVE_EXTENSIONS=1 )

target_sources( thread-pool INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/bs_thread_pool.hpp
    )