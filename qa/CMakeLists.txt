if( KICAD_SCRIPTING_MODULES )

    # Test that runs the QA tests through scripting
    add_test( NAME qa_python
        COMMAND ${PYTHON_EXECUTABLE} test.py
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
    )

    set_property( TEST qa_python
        PROPERTY ENVIRONMENT "PYTHONPATH=${CMAKE_BINARY_DIR}/pcbnew${PYTHON_QA_PATH}"
    )

endif()

add_subdirectory( geometry )
add_subdirectory( shape_poly_set_refactor )
add_subdirectory( pcb_test_window )
add_subdirectory( polygon_triangulation )
add_subdirectory( polygon_generator )