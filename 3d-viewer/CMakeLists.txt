#add_definitions(-DPRINT_STATISTICS_3D_VIEWER)
add_definitions(-DPCBNEW)

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
    ${INC_AFTER}
    )

# directories
set( DIR_DLG 3d_cache/dialogs )
set( DIR_RAY 3d_rendering/3d_render_raytracing )
set( DIR_RAY_ACC ${DIR_RAY}/accelerators )
set( DIR_RAY_2D ${DIR_RAY}/shapes2D )
set( DIR_RAY_3D ${DIR_RAY}/shapes3D )
set( DIR_3D_PLUGINS ${CMAKE_SOURCE_DIR}/plugins/ldr )

set(3D-VIEWER_SRCS
    ${DIR_3D_PLUGINS}/pluginldr.cpp
    ${DIR_3D_PLUGINS}/3d/pluginldr3D.cpp
    3d_cache/3d_cache.cpp
    3d_cache/3d_plugin_manager.cpp
    ${DIR_DLG}/3d_cache_dialogs.cpp
    ${DIR_DLG}/dlg_select_3dmodel_base.cpp
    ${DIR_DLG}/dlg_select_3dmodel.cpp
    ${DIR_DLG}/panel_prev_3d_base.cpp
    ${DIR_DLG}/panel_prev_3d.cpp
    3d_canvas/board_adapter.cpp
    3d_canvas/create_layer_items.cpp
    3d_canvas/create_3Dgraphic_brd_items.cpp
    3d_canvas/create_layer_poly.cpp
    3d_canvas/eda_3d_canvas.cpp
    3d_canvas/eda_3d_canvas_pivot.cpp
    3d_model_viewer/3d_model_viewer.cpp
    3d_rendering/legacy/3d_model.cpp
    3d_rendering/legacy/ogl_legacy_utils.cpp
    3d_rendering/legacy/create_scene.cpp
    3d_rendering/legacy/render_3d_legacy.cpp
    3d_rendering/legacy/layer_triangles.cpp
    ${DIR_RAY_ACC}/accelerator_3d.cpp
    ${DIR_RAY_ACC}/bvh_packet_traversal.cpp
    ${DIR_RAY_ACC}/bvh_pbrt.cpp
    ${DIR_RAY_ACC}/container_3d.cpp
    ${DIR_RAY_ACC}/container_2d.cpp
    ${DIR_RAY}/PerlinNoise.cpp
    ${DIR_RAY}/create_scene.cpp
    ${DIR_RAY}/render_3d_raytrace.cpp
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
    3d_rendering/camera.cpp
    3d_rendering/color_rgb.cpp
    3d_rendering/image.cpp
    3d_rendering/post_shader.cpp
    3d_rendering/post_shader_ssao.cpp
    3d_rendering/track_ball.cpp
    3d_rendering/test_cases.cpp
    3d_rendering/trackball.cpp
    3d_viewer/3d_menubar.cpp
    3d_viewer/3d_toolbar.cpp
    3d_viewer/dialogs/dialog_3D_view_option.cpp
    3d_viewer/dialogs/dialog_3D_view_option_base.cpp
    3d_viewer/tools/3d_actions.cpp
    3d_viewer/tools/3d_conditions.cpp
    3d_viewer/tools/3d_controller.cpp
    3d_viewer/eda_3d_viewer.cpp
    3d_viewer/3d_viewer_settings.cpp
    common_ogl/ogl_attr_list.cpp
    common_ogl/ogl_utils.cpp
    3d_fastmath.cpp
    3d_math.cpp
    )

add_library(3d-viewer STATIC ${3D-VIEWER_SRCS})
add_dependencies( 3d-viewer pcbcommon )

target_link_libraries( 3d-viewer
    PRIVATE
        gal
        kimath
        nlohmann_json
        ${Boost_LIBRARIES}
        ${wxWidgets_LIBRARIES}
        ${OPENGL_LIBRARIES}
        kicad_3dsg )

add_subdirectory( 3d_cache )
