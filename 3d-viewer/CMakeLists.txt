add_definitions(-DPCBNEW)

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${Boost_INCLUDE_DIR}
    ../pcbnew
    ../polygon
    ${INC_AFTER}
    )

set(3D-VIEWER_SRCS
    3d_aux.cpp
    3d_canvas.cpp
    3d_class.cpp
    3d_draw.cpp
    3d_draw_basic_functions.cpp
    3d_frame.cpp
    3d_read_mesh.cpp
    3d_toolbar.cpp
    info3d_visu.cpp
    trackball.cpp
    )

add_library(3d-viewer STATIC ${3D-VIEWER_SRCS})
