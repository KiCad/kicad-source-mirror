add_definitions(-DPCBNEW)

include_directories(../pcbnew
                    ../polygon)

set(3D-VIEWER_SRCS
    3d_aux.cpp
    3d_canvas.cpp
    3d_class.cpp
    3d_draw.cpp
    3d_frame.cpp
    3d_read_mesh.cpp
    3d_toolbar.cpp
    trackball.cpp)

add_library(3d-viewer ${3D-VIEWER_SRCS})
