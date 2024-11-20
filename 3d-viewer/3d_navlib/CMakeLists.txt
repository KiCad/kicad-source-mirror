    add_library(3d-viewer_navlib STATIC
	    "nl_3d_viewer_plugin.cpp"
	    "nl_3d_viewer_plugin_impl.cpp"
        "nl_footprint_properties_plugin.cpp"
        "nl_footprint_properties_plugin_impl.cpp"
    )

    # 3d-viewer_navlib depends on make_lexer outputs in common
    add_dependencies( 3d-viewer_navlib pcbcommon )

    # Find the 3DxWare SDK component 3DxWare::NlClient
    # find_package(TDxWare_SDK 4.0 REQUIRED COMPONENTS 3DxWare::Navlib)
    target_compile_definitions(3d-viewer_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_DEFINITIONS>
    )
    target_compile_options(3d-viewer_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_OPTIONS>
    )
    target_include_directories(3d-viewer_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:pcbnew_kiface_objects,INCLUDE_DIRECTORIES>
    )
    target_link_libraries(3d-viewer_navlib
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_LINK_LIBRARIES>
        3DxWare::Navlib
    )

