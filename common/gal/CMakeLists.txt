
set( GAL_SRCS
    # Common part
	../callback_gal.cpp
    ../draw_panel_gal.cpp
    ../gl_context_mgr.cpp
    ../newstroke_font.cpp
    ../painter.cpp
    color4d.cpp
    cursors.cpp
    dpi_scaling.cpp
    gal_display_options.cpp
    graphics_abstraction_layer.cpp
    hidpi_gl_canvas.cpp

    ../view/view_controls.cpp
    ../view/view_overlay.cpp
    ../view/wx_view_controls.cpp
    ../view/zoom_controller.cpp

    # OpenGL GAL
    opengl/opengl_gal.cpp
    opengl/gl_resources.cpp
    opengl/gl_builtin_shaders.cpp
    opengl/shader.cpp
    opengl/vertex_item.cpp
    opengl/vertex_container.cpp
    opengl/cached_container.cpp
    opengl/cached_container_gpu.cpp
    opengl/cached_container_ram.cpp
    opengl/noncached_container.cpp
    opengl/vertex_manager.cpp
    opengl/gpu_manager.cpp
    opengl/antialiasing.cpp
    opengl/opengl_compositor.cpp
    opengl/utils.cpp

    # Cairo GAL
    cairo/cairo_gal.cpp
    cairo/cairo_compositor.cpp
    cairo/cairo_print.cpp
    )

add_library( gal STATIC ${GAL_SRCS} )

target_link_libraries( gal
    common          # This is needed until the circular dependency is removed
    kimath
    nlohmann_json
    ${GLEW_LIBRARIES}
    ${CAIRO_LIBRARIES}
    ${PIXMAN_LIBRARIES}
    ${OPENGL_LIBRARIES}
    ${GDI_PLUS_LIBRARIES}
	# outline font support
	${FREETYPE_LIBRARIES}
	${HarfBuzz_LIBRARIES}
	${Fontconfig_LIBRARIES}
)

