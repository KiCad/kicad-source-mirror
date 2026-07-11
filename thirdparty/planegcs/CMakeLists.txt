# planegcs: 2D geometric constraint solver vendored from FreeCAD's Sketcher.
# LGPL-2.1-or-later; kept isolated from the permissive libs/kimath. The KiCad
# adapter that consumes it lives in pcbnew/constraints/. See README.kicad.md.

set( PLANEGCS_SRCS
    Constraints.cpp
    GCS.cpp
    Geo.cpp
    SubSystem.cpp
    qp_eq.cpp
    )

add_library( planegcs STATIC ${PLANEGCS_SRCS} )

target_include_directories( planegcs
    PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    )

# Eigen is a header-only implementation detail of planegcs, vendored under
# thirdparty/eigen. planegcs uses Boost.Graph and Boost.Math headers, already
# required by KiCad.
target_link_libraries( planegcs PUBLIC eigen Boost::headers )

target_compile_features( planegcs PUBLIC cxx_std_20 )
