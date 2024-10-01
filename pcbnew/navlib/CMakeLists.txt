
add_library(pcbnew_navlib STATIC
    "nl_pcbnew_plugin.cpp"
    "nl_pcbnew_plugin_impl.cpp"
)

# pcbnew_navlib depends on make_lexer outputs in common
add_dependencies( pcbnew_navlib pcbcommon )

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
    kiplatform # Needs kiplatform to check versioning
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_LINK_LIBRARIES>
    3DxWare::Navlib
)
