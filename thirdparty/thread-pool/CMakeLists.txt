add_library( thread-pool INTERFACE )

target_include_directories( thread-pool SYSTEM INTERFACE ${CMAKE_CURRENT_SOURCE_DIR} )

target_sources( thread-pool INTERFACE
    ${CMAKE_CURRENT_SOURCE_DIR}/bs_thread_pool.hpp
    )