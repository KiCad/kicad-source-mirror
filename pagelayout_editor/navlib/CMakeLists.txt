
add_library(pl_editor_navlib STATIC
    "nl_pl_editor_plugin.cpp"
    "nl_pl_editor_plugin_impl.cpp"
)

# pl_editor_navlib depends on make_lexer outputs in common
add_dependencies( pl_editor_navlib common )

# Find the 3DxWare SDK component 3DxWare::NlClient
# find_package(TDxWare_SDK 4.0 REQUIRED COMPONENTS 3DxWare::Navlib)
target_compile_definitions(pl_editor_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(pl_editor_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_OPTIONS>
)
target_include_directories(pl_editor_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:pl_editor_kiface,INCLUDE_DIRECTORIES>
)
target_link_libraries(pl_editor_navlib
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_LINK_LIBRARIES>
    3DxWare::Navlib
)
