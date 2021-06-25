if( KICAD_USE_3DCONNEXION )
    add_library(pcbnew_navlib STATIC
	    "nl_pcbnew_plugin.cpp"
	    "nl_pcbnew_plugin_impl.cpp"
    )

    # Find the 3DxWare SDK component 3DxWare::NlClient
    # find_package(TDxWare_SDK 4.0 REQUIRED COMPONENTS 3DxWare::Navlib)
    target_compile_definitions(pcbnew_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_DEFINITIONS>
    )
    target_compile_options(pcbnew_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_OPTIONS>
    )
    target_include_directories(pcbnew_navlib PRIVATE
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_INCLUDE_DIRECTORIES>
        $<TARGET_PROPERTY:pcbnew_kiface_objects,INCLUDE_DIRECTORIES>
    )
    target_link_libraries(pcbnew_navlib
        $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_LINK_LIBRARIES>
        3DxWare::Navlib
    )
else()
    add_library(pcbnew_navlib STATIC
	    "nl_pcbnew_plugin.cpp"
    )
endif(KICAD_USE_3DCONNEXION)
