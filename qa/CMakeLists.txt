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

# Shared QA helper libraries
add_subdirectory( qa_utils )
add_subdirectory( unit_test_utils )

# Unit tests
add_subdirectory( common )
add_subdirectory( pcbnew )
add_subdirectory( shape_poly_set_refactor )

# Utility/test programs
add_subdirectory( pcb_parse_input )

# add_subdirectory( pcb_test_window )
# add_subdirectory( polygon_triangulation )
# add_subdirectory( polygon_generator )
