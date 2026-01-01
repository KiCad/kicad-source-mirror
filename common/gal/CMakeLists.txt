

set( FONT_SRCS
    ../font/font.cpp
    ../font/glyph.cpp
    ../font/stroke_font.cpp
	../font/outline_font.cpp
	../font/outline_decomposer.cpp
    ../font/text_attributes.cpp
	)

set( GAL_SRCS
    # Common part
    ../callback_gal.cpp
    painter.cpp
    cursors.cpp
    gal_display_options.cpp
    graphics_abstraction_layer.cpp
    hidpi_gl_canvas.cpp
    hidpi_gl_3D_canvas.cpp

    ../view/view.cpp
    ../view/view_controls.cpp
    ../view/view_group.cpp
    ../view/view_overlay.cpp
    ../view/zoom_controller.cpp
    ../view/view_item.cpp

    ${FONT_SRCS}

    3d/camera.cpp

    # OpenGL GAL
    opengl/opengl_gal.cpp
    opengl/gl_resources.cpp

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

add_library( gal SHARED ${GAL_SRCS} )

if( WIN32 )
    # we need the gdiplus library for cairo printing on windows
    set( GDI_PLUS_LIBRARIES gdiplus )
endif()

target_link_libraries( gal
    kicommon
    kimath
    kiplatform
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

target_compile_definitions( gal PRIVATE GAL_DLL=1 )

install( TARGETS gal
    RUNTIME DESTINATION ${KICAD_LIB}
    LIBRARY DESTINATION ${KICAD_LIB}
    COMPONENT binary
    )

include( ${KICAD_CMAKE_MODULE_PATH}/KiCadVersion.cmake )
include( ${KICAD_CMAKE_MODULE_PATH}/CreateGitVersionHeader.cmake )
create_git_version_header(${CMAKE_SOURCE_DIR})

# Extract the major and minor build version as a string
string( REGEX MATCH
        "([0-9]+)\\.([0-9]+)\\.([0-9]+)"
        KICAD_MAJOR_MINOR_PATCH_VERSION
        "${KICAD_VERSION}"
    )

set_target_properties( gal PROPERTIES
    OUTPUT_NAME     kigal
    SOVERSION      ${KICAD_MAJOR_MINOR_PATCH_VERSION}
    )

if( APPLE )
    # puts library into the main kicad.app bundle in build tree
    set_target_properties( gal PROPERTIES
        LIBRARY_OUTPUT_DIRECTORY "${OSX_BUNDLE_BUILD_LIB_DIR}"
        INSTALL_NAME_DIR "${OSX_BUNDLE_BUILD_LIB_DIR}"
    )
    set_target_properties( gal PROPERTIES INSTALL_RPATH
            "@executable_path/../Frameworks" )
    set_target_properties( gal PROPERTIES BUILD_WITH_INSTALL_RPATH 1 )
endif()

function( add_shader outTarget inFile shaderName )
    set(outCppName "${shaderName}.cpp")
    set(outHeaderName "${shaderName}.h")

    add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${outCppName}
                ${CMAKE_BINARY_DIR}/include/gal/shaders/${outHeaderName}
        COMMAND ${CMAKE_COMMAND}
            -DSOURCE_FILE="${CMAKE_CURRENT_SOURCE_DIR}/shaders/${inFile}"
            -DOUT_CPP_DIR="${CMAKE_CURRENT_BINARY_DIR}/"
            -DOUT_HEADER_DIR="${CMAKE_BINARY_DIR}/include/gal/shaders/"
            -DOUT_CPP_FILENAME="${outCppName}"
            -DOUT_HEADER_FILENAME="${outHeaderName}"
            -DOUT_VAR_NAME="${shaderName}"
            -P ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateShaderCpp.cmake
        DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/${inFile}
                ${KICAD_CMAKE_MODULE_PATH}/BuildSteps/CreateShaderCpp.cmake
        )

    target_sources( ${outTarget} PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/${outCppName} )
    target_include_directories( ${outTarget} PUBLIC ${CMAKE_BINARY_DIR}/include/gal/shaders/ )
endfunction()

add_shader( gal kicad_frag.glsl glsl_kicad_frag )
add_shader( gal kicad_vert.glsl glsl_kicad_vert )
add_shader( gal smaa_base.glsl glsl_smaa_base )
add_shader( gal smaa_pass_1_frag_color.glsl glsl_smaa_pass_1_frag_color )
add_shader( gal smaa_pass_1_frag_luma.glsl glsl_smaa_pass_1_frag_luma )
add_shader( gal smaa_pass_1_vert.glsl glsl_smaa_pass_1_vert )
add_shader( gal smaa_pass_2_frag.glsl glsl_smaa_pass_2_frag )
add_shader( gal smaa_pass_2_vert.glsl glsl_smaa_pass_2_vert )
add_shader( gal smaa_pass_3_frag.glsl glsl_smaa_pass_3_frag )
add_shader( gal smaa_pass_3_vert.glsl glsl_smaa_pass_3_vert )
add_shader( gal xor_diff_frag.glsl glsl_xor_diff_frag )
add_shader( gal xor_diff_vert.glsl glsl_xor_diff_vert )