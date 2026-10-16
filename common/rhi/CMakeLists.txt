set( RHI_SRCS
    gpu_compute.cpp
    rhi_compute.cpp
    rhi_bind_group.cpp
    rhi_compositor.cpp
    rhi_device_proxy.cpp
    rhi_gal_pipelines.cpp
    rhi_platform_device.cpp
    rhi_projection.cpp
    render_thread.cpp
    swapchain_controller.cpp
)

if( APPLE )
    list( APPEND RHI_SRCS rhi_metal_device.mm )
    set( RHI_HAS_METAL TRUE CACHE INTERNAL "Metal backend available" )
else()
    # The device loads the Vulkan loader at run time, so only the headers are needed
    find_package( VulkanHeaders CONFIG QUIET )

    if( NOT VulkanHeaders_FOUND )
        find_package( Vulkan QUIET )
    endif()

    if( VulkanHeaders_FOUND OR Vulkan_FOUND )
        list( APPEND RHI_SRCS rhi_vulkan_device.cpp rhi_vma_impl.cpp )
        set( RHI_HAS_VULKAN TRUE CACHE INTERNAL "Vulkan backend available" )
    endif()
endif()

add_library( rhi STATIC ${RHI_SRCS} )

target_include_directories( rhi PUBLIC
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries( rhi PUBLIC core PRIVATE kiplatform magic_enum )

if( RHI_HAS_VULKAN )
    target_compile_definitions( rhi PUBLIC RHI_HAS_VULKAN )
    find_package( X11 QUIET )

    if( VulkanHeaders_FOUND )
        target_link_libraries( rhi PRIVATE Vulkan::Headers )
    else()
        target_include_directories( rhi PRIVATE ${Vulkan_INCLUDE_DIRS} )
    endif()

    target_include_directories( rhi PRIVATE ${CMAKE_SOURCE_DIR}/thirdparty/vma )
    target_link_libraries( rhi PRIVATE ${CMAKE_DL_LIBS} )

    # This file only compiles the vendored allocator, whose warnings are not ours to fix.
    if( MSVC )
        set_source_files_properties( rhi_vma_impl.cpp PROPERTIES COMPILE_OPTIONS "/W0" )
    else()
        set_source_files_properties( rhi_vma_impl.cpp PROPERTIES COMPILE_OPTIONS "-w" )
    endif()

    # The X11 surface path connects through xcb, so it needs both libraries
    if( TARGET X11::X11 AND TARGET X11::xcb )
        target_compile_definitions( rhi PRIVATE RHI_VULKAN_HAS_X11 )
        target_link_libraries( rhi PRIVATE X11::X11 X11::xcb )
    endif()

    if( KICAD_WAYLAND )
        find_package( Wayland 1.20 COMPONENTS Client REQUIRED )
        target_compile_definitions( rhi PRIVATE RHI_VULKAN_HAS_WAYLAND )
        target_link_libraries( rhi PRIVATE Wayland::Client )
    endif()
endif()

if( RHI_HAS_METAL )
    target_compile_definitions( rhi PUBLIC RHI_HAS_METAL )
    target_link_libraries( rhi PRIVATE
        "-framework Metal"
        "-framework QuartzCore"
        "-framework AppKit"
    )

    # ARC spares the Metal device manual releases on every failure path and in the destructor.
    set_source_files_properties( rhi_metal_device.mm
        PROPERTIES
            LANGUAGE OBJCXX
            COMPILE_FLAGS "-fobjc-arc"
    )
endif()

file( GLOB RHI_SHADERS CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.vert
                                        ${CMAKE_CURRENT_SOURCE_DIR}/shaders/*.frag )

kicad_add_shaders( TARGET rhi SHADERS ${RHI_SHADERS} )
