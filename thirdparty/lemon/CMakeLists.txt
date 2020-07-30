add_executable( lemon
    lemon.c
    )

# The location of the template file for lemon is passed as a property on the lemon target
define_property( TARGET
    PROPERTY lemon_template
    BRIEF_DOCS "Location of the template file for the lemon parser"
    FULL_DOCS "Location of the template file for the lemon parser"
    )

set_property( TARGET lemon
    PROPERTY lemon_template ${CMAKE_CURRENT_SOURCE_DIR}/lempar.c
    )
