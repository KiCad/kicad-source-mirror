add_definitions(-DPCBNEW)

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    textures
    ../pcbnew
    ../polygon
    ${INC_AFTER}
    )

set(3D-VIEWER_SRCS
    dialogs/dialog_3D_view_option_base.cpp
    dialogs/dialog_3D_view_option.cpp
    3d_aux.cpp
    3d_canvas.cpp
    3d_class.cpp
    3d_draw.cpp
    3d_draw_basic_functions.cpp
    3d_frame.cpp
    3d_material.cpp
    3d_mesh_model.cpp
    3d_read_mesh.cpp
    3d_toolbar.cpp
    info3d_visu.cpp
    trackball.cpp
    vrmlmodelparser.cpp
    vrml_aux.cpp
    vrml_v1_modelparser.cpp
    vrml_v2_modelparser.cpp
    x3dmodelparser.cpp
    )

add_library(3d-viewer STATIC ${3D-VIEWER_SRCS})
