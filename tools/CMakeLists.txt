

add_executable( container_test EXCLUDE_FROM_ALL container_test.cpp )
target_link_libraries( container_test common polygon bitmaps ${wxWidgets_LIBRARIES} )


