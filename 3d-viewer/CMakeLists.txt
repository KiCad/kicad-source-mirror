add_definitions(-DPCBNEW)

include_directories(BEFORE ${INC_BEFORE})
include_directories(
    textures
    ../pcbnew
    ../polygon
    3d_canvas
    3d_cache
    3d_rendering
    3d_viewer
    ${CMAKE_SOURCE_DIR}/include/gal/opengl
    ${GLEW_INCLUDE_DIR}
    ${GLM_INCLUDE_DIR}
    ${CMAKE_SOURCE_DIR}
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
    dialogs/dialog_3D_view_option_base.cpp
    dialogs/dialog_3D_view_option.cpp
    3d_aux.cpp
    3d_canvas.cpp
    3d_class.cpp
    3d_draw.cpp
    3d_draw_board_body.cpp
    3d_draw_basic_functions.cpp
    3d_draw_helper_functions.cpp
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
    CImage.cpp
    ${DIR_3D_PLUGINS}/pluginldr.cpp
    ${DIR_3D_PLUGINS}/3d/pluginldr3D.cpp
    3d_cache/3d_cache_wrapper.cpp
    3d_cache/3d_cache.cpp
    3d_cache/3d_plugin_manager.cpp
    3d_cache/3d_filename_resolver.cpp
    ${DIR_DLG}/3d_cache_dialogs.cpp
    ${DIR_DLG}/dlg_3d_pathconfig_base.cpp
    ${DIR_DLG}/dlg_3d_pathconfig.cpp
    ${DIR_DLG}/dlg_select_3dmodel.cpp
    ${DIR_DLG}/panel_prev_model.cpp
    3d_model_viewer/c3d_model_viewer.cpp
    3d_rendering/3d_render_ogl_legacy/c_ogl_3dmodel.cpp
    3d_rendering/3d_render_ogl_legacy/ogl_legacy_utils.cpp
    3d_rendering/3d_render_ogl_legacy/c3d_render_createscene_ogl_legacy.cpp
    3d_rendering/3d_render_ogl_legacy/c3d_render_ogl_legacy.cpp
    3d_rendering/3d_render_ogl_legacy/clayer_triangles.cpp
    ${DIR_RAY}/ray.cpp
    ${DIR_RAY_2D}/cbbox2d.cpp
    ${DIR_RAY_2D}/cobject2d.cpp
    ${DIR_RAY_2D}/croundsegment2d.cpp
    ${DIR_RAY_3D}/cbbox.cpp
    ${DIR_RAY_3D}/cbbox_ray.cpp
    3d_rendering/ccamera.cpp
    3d_rendering/cimage.cpp
    3d_rendering/ctrack_ball.cpp
    3d_rendering/trackball.cpp
    common_ogl/cogl_att_list.cpp
    common_ogl/ogl_utils.cpp
    )

add_library(3d-viewer STATIC ${3D-VIEWER_SRCS})
add_dependencies( 3d-viewer pcbcommon )

option( GLM_ENABLE_SIMD_SSE2 "Enable SSE2 optimizations" OFF )
option( GLM_ENABLE_SIMD_SSE3 "Enable SSE3 optimizations" OFF )
option( GLM_ENABLE_SIMD_AVX "Enable AVX optimizations" OFF )
option( GLM_ENABLE_SIMD_AVX2 "Enable AVX2 optimizations" OFF )
option( GLM_FORCE_PURE "Force 'pure' instructions" OFF )

if(GLM_FORCE_PURE)
    add_definitions(-DGLM_FORCE_PURE)

    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-mfpmath=387)
    endif()
elseif(GLM_ENABLE_SIMD_AVX2)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-mavx2)
    elseif(GLM_USE_INTEL)
        add_definitions(/QxAVX2)
    elseif(MSVC)
        add_definitions(/arch:AVX2)
    endif()
elseif(GLM_ENABLE_SIMD_AVX)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-mavx)
    elseif(GLM_USE_INTEL)
        add_definitions(/QxAVX)
    elseif(MSVC)
        add_definitions(/arch:AVX)
    endif()
elseif(GLM_ENABLE_SIMD_SSE3)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-msse3)
    elseif(GLM_USE_INTEL)
        add_definitions(/QxSSE3)
    elseif(MSVC)
        add_definitions(/arch:SSE2) # VC doesn't support /arch:SSE3
    endif()
elseif(GLM_ENABLE_SIMD_SSE2)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-msse2)
    elseif(GLM_USE_INTEL)
        add_definitions(/QxSSE2)
    elseif(MSVC)
        if(NOT CMAKE_CL_64)
            add_definitions(/arch:SSE2)
        endif()
    endif()
endif()


option( GLM_ENABLE_FAST_MATH "Enable fast math optimizations" OFF )

if(GLM_ENABLE_FAST_MATH)
    if(CMAKE_COMPILER_IS_GNUCXX)
        add_definitions(-ffast-math)
    endif()

    if(MSVC)
        add_definitions(/fp:fast)
    endif()
elseif(NOT GLM_ENABLE_FAST_MATH)
    if(MSVC)
        add_definitions(/fp:precise)
    endif()
endif()

target_link_libraries( 3d-viewer ${Boost_} ${wxWidgets_LIBRARIES} ${OPENGL_LIBRARIES} kicad_3dsg )

add_subdirectory( 3d_cache )
