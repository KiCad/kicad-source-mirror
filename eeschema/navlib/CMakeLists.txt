
add_library(eeschema_navlib STATIC
    "nl_schematic_plugin.cpp"
    "nl_schematic_plugin_impl.cpp"
)

# eeschema_navlib depends on make_lexer outputs in common
add_dependencies( eeschema_navlib common )

# Find the 3DxWare SDK component 3DxWare::NlClient
# find_package(TDxWare_SDK 4.0 REQUIRED COMPONENTS 3DxWare::Navlib)
target_compile_definitions(eeschema_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_DEFINITIONS>
)
target_compile_options(eeschema_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_COMPILE_OPTIONS>
)
target_include_directories(eeschema_navlib PRIVATE
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_INCLUDE_DIRECTORIES>
    $<TARGET_PROPERTY:eeschema_kiface_objects,INCLUDE_DIRECTORIES>
)
target_link_libraries(eeschema_navlib
    kiplatform # Needs kiplatform to check versioning
    $<TARGET_PROPERTY:3DxWare::Navlib,INTERFACE_LINK_LIBRARIES>
    3DxWare::Navlib
)
