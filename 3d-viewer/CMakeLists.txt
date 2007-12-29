ADD_DEFINITIONS(-DPCBNEW)

INCLUDE_DIRECTORIES(../pcbnew ../polygon)

SET(3D-VIEWER_SRCS
	3d_aux.cpp
	3d_canvas.cpp
	3d_class.cpp
	3d_draw.cpp
	3d_frame.cpp
	3d_read_mesh.cpp
	3d_toolbar.cpp
	trackball.cpp)

ADD_LIBRARY(3d-viewer ${3D-VIEWER_SRCS})
