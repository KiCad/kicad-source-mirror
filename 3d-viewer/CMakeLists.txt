#add_definitions(-DPRINT_STATISTICS_3D_VIEWER)
add_compile_definitions( PCBNEW )

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    ${CMAKE_CURRENT_BINARY_DIR}
    ../pcbnew
    3d_canvas
    3d_cache
    3d_rendering
    3d_viewer
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include/gal/opengl
    ${CMAKE_SOURCE_DIR}/common/dialogs
    ${INC_AFTER}
    )

# directories
set( DIR_RAY 3d_rendering/raytracing )
set( DIR_RAY_ACC ${DIR_RAY}/accelerators )
set( DIR_RAY_2D ${DIR_RAY}/shapes2D )
set( DIR_RAY_3D ${DIR_RAY}/shapes3D )
set( DIR_3D_PLUGINS ${CMAKE_SOURCE_DIR}/plugins/ldr )

set(3D-VIEWER_SRCS
    ${DIR_3D_PLUGINS}/pluginldr.cpp
    ${DIR_3D_PLUGINS}/3d/pluginldr3D.cpp
    3d_cache/3d_cache.cpp
    3d_cache/3d_plugin_manager.cpp
    3d_canvas/board_adapter.cpp
    3d_canvas/create_layer_items.cpp
    3d_canvas/create_3Dgraphic_brd_items.cpp
    3d_canvas/eda_3d_canvas.cpp
    3d_canvas/eda_3d_canvas_pivot.cpp
    3d_model_viewer/eda_3d_model_viewer.cpp
    3d_rendering/opengl/3d_model.cpp
    3d_rendering/opengl/opengl_utils.cpp
    3d_rendering/opengl/create_scene.cpp
    3d_rendering/opengl/render_3d_opengl.cpp
    3d_rendering/opengl/layer_triangles.cpp
    3d_rendering/opengl/3d_spheres_gizmo.cpp
    ${DIR_RAY_ACC}/accelerator_3d.cpp
    ${DIR_RAY_ACC}/bvh_packet_traversal.cpp
    ${DIR_RAY_ACC}/bvh_pbrt.cpp
    ${DIR_RAY_ACC}/container_3d.cpp
    ${DIR_RAY_ACC}/container_2d.cpp
    ${DIR_RAY}/PerlinNoise.cpp
    ${DIR_RAY}/create_scene.cpp
    ${DIR_RAY}/render_3d_raytrace_base.cpp
    ${DIR_RAY}/render_3d_raytrace_gl.cpp
    ${DIR_RAY}/render_3d_raytrace_ram.cpp
    ${DIR_RAY}/frustum.cpp
    ${DIR_RAY}/material.cpp
    ${DIR_RAY}/mortoncodes.cpp
    ${DIR_RAY}/ray.cpp
    ${DIR_RAY}/raypacket.cpp
    ${DIR_RAY_2D}/bbox_2d.cpp
    ${DIR_RAY_2D}/filled_circle_2d.cpp
    ${DIR_RAY_2D}/layer_item_2d.cpp
    ${DIR_RAY_2D}/object_2d.cpp
    ${DIR_RAY_2D}/polygon_2d.cpp
    ${DIR_RAY_2D}/4pt_polygon_2d.cpp
    ${DIR_RAY_2D}/ring_2d.cpp
    ${DIR_RAY_2D}/round_segment_2d.cpp
    ${DIR_RAY_2D}/triangle_2d.cpp
    ${DIR_RAY_3D}/bbox_3d.cpp
    ${DIR_RAY_3D}/bbox_3d_ray.cpp
    ${DIR_RAY_3D}/cylinder_3d.cpp
    ${DIR_RAY_3D}/dummy_block_3d.cpp
    ${DIR_RAY_3D}/layer_item_3d.cpp
    ${DIR_RAY_3D}/object_3d.cpp
    ${DIR_RAY_3D}/plane_3d.cpp
    ${DIR_RAY_3D}/round_segment_3d.cpp
    ${DIR_RAY_3D}/triangle_3d.cpp
    3d_rendering/buffers_debug.cpp
    3d_rendering/render_3d_base.cpp
    3d_rendering/color_rgba.cpp
    3d_rendering/image.cpp
    3d_rendering/post_shader.cpp
    3d_rendering/post_shader_ssao.cpp
    3d_rendering/track_ball.cpp
    3d_rendering/test_cases.cpp
    3d_rendering/trackball.cpp
    3d_viewer/3d_menubar.cpp
    3d_viewer/toolbars_3d.cpp
    3d_viewer/tools/eda_3d_actions.cpp
    3d_viewer/tools/eda_3d_conditions.cpp
    3d_viewer/tools/eda_3d_controller.cpp
    3d_viewer/eda_3d_viewer_frame.cpp
    3d_viewer/eda_3d_viewer_settings.cpp
    common_ogl/ogl_attr_list.cpp
    common_ogl/ogl_utils.cpp
    3d_fastmath.cpp
    3d_math.cpp
    dialogs/appearance_controls_3D.cpp
    dialogs/appearance_controls_3D_base.cpp
    dialogs/dialog_select_3d_model_base.cpp
    dialogs/dialog_select_3d_model.cpp
    dialogs/panel_preview_3d_model_base.cpp
    dialogs/panel_preview_3d_model.cpp
    dialogs/dialog_export_3d_image.cpp
    dialogs/panel_3D_display_options.cpp
    dialogs/panel_3D_display_options_base.cpp
    dialogs/panel_3D_opengl_options.cpp
    dialogs/panel_3D_opengl_options_base.cpp
    dialogs/panel_3D_raytracing_options.cpp
    dialogs/panel_3D_raytracing_options_base.cpp
    )

add_library(3d-viewer STATIC ${3D-VIEWER_SRCS})
add_dependencies( 3d-viewer pcbcommon )

target_link_libraries( 3d-viewer
    PRIVATE
        gal
        kimath
        core
        common
        nlohmann_json
        Boost::headers
        ${wxWidgets_LIBRARIES}
        ${OPENGL_LIBRARIES}
        kicad_3dsg )

add_subdirectory( 3d_cache )


if( UNIX AND NOT APPLE )
    message( STATUS "Including libspnav support in 3d-viewer" )
    add_subdirectory( 3d_spacenav )
    target_link_libraries( 3d-viewer PRIVATE 3d-viewer_spacenav )
else()
    message( STATUS "Including 3Dconnexion SpaceMouse navigation support in 3d-viewer" )
    add_subdirectory( 3d_navlib )
    target_link_libraries( 3d-viewer PRIVATE 3d-viewer_navlib)
endif()
