if( KICAD_SCRIPTING_MODULES )

    if( APPLE AND ( KICAD_BUILD_STATIC OR KICAD_BUILD_DYNAMIC ) )
        set( PYTHON_QA_PATH :${LIBWXPYTHON_ROOT}/wxPython/lib/python2.6/site-packages ) 
    endif()

    # build target that runs the QA tests through scripting 
    add_custom_target( qa
        COMMAND PYTHONPATH=${CMAKE_BINARY_DIR}/pcbnew${PYTHON_QA_PATH} ${PYTHON_EXECUTABLE} test.py

        COMMENT "running qa"
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )

endif()
