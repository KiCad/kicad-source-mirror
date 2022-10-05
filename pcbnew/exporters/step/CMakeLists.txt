include_directories( SYSTEM
    ${OCE_INCLUDE_DIRS}
    ${OCC_INCLUDE_DIR}
)

set( KS2_LIB_FILES
    kicad2step.cpp
    pcb/3d_resolver.cpp
    pcb/base.cpp
    pcb/kicadmodel.cpp
    pcb/kicadfootprint.cpp
    pcb/kicadpad.cpp
    pcb/kicadpcb.cpp
    pcb/kicadcurve.cpp
    pcb/oce_utils.cpp
)

# Break the library out for re-use by both kicad2step and any qa that needs it
# In future, this could move for re-use by other programs needing s-expr support (?)
add_library( kicad2step_lib STATIC
    ${KS2_LIB_FILES}
)

target_include_directories( kicad2step_lib PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include # for core
    ${Boost_INCLUDE_DIR}        # see commit 03bce554
    ${CMAKE_SOURCE_DIR}/libs/kimath/include
    ${INC_AFTER}
)

target_link_libraries( kicad2step_lib
    sexpr
    common
    ${wxWidgets_LIBRARIES}
    ${OCC_LIBRARIES}
    ${ZLIB_LIBRARIES}
    kimath
)


target_include_directories( kicad2step_lib PRIVATE
    $<TARGET_PROPERTY:gzip-hpp,INTERFACE_INCLUDE_DIRECTORIES>
    )