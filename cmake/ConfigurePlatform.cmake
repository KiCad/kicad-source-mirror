if( MSVC )
    cmake_policy( SET CMP0091 NEW )

    # CMake does not set CMAKE_SYSTEM_PROCESSOR correctly for MSVC
    # and it will always return the host instead of the target arch
    if("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM64")
        set( KICAD_BUILD_ARCH "arm64" )
        set( KICAD_BUILD_ARCH_ARM64 1 )
    elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "ARM")
        set( KICAD_BUILD_ARCH "arm" )
        set( KICAD_BUILD_ARCH_ARM 1 )
    elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "X86")
        set( KICAD_BUILD_ARCH "x86" )
        set( KICAD_BUILD_ARCH_X86 1 )
    elseif("${MSVC_C_ARCHITECTURE_ID}" STREQUAL "x64")
        set( KICAD_BUILD_ARCH "x64" )
        set( KICAD_BUILD_ARCH_X64 1 )
    else()
        message(FATAL_ERROR "Failed to determine the MSVC target architecture: ${MSVC_C_ARCHITECTURE_ID}")
    endif()

    add_compile_definitions( KICAD_BUILD_ARCH=${KICAD_BUILD_ARCH} )

    # CMake does not set CMAKE_SYSTEM_PROCESSOR correctly for MSVC
    # and it will always return the host instead of the target arch
    if("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "ARM64")
        set( KICAD_HOST_ARCH "arm64" )
        set( KICAD_HOST_ARCH_ARM64 1 )
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "ARM")
        set( KICAD_HOST_ARCH "arm" )
        set( KICAD_HOST_ARCH_ARM 1 )
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "X86")
        set( KICAD_HOST_ARCH "x86" )
        set( KICAD_HOST_ARCH_X86 1 )
    elseif("${CMAKE_HOST_SYSTEM_PROCESSOR}" STREQUAL "AMD64")
        set( KICAD_HOST_ARCH "x64" )
        set( KICAD_HOST_ARCH_X64 1 )
    else()
        message(FATAL_ERROR "Failed to determine the host architecture: ${CMAKE_SYSTEM_PROCESSOR}")
    endif()
else()
    if ( NOT CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set( KICAD_BUILD_ARCH "x86" )
        set( KICAD_BUILD_ARCH_X86 1 )
    elseif( CMAKE_SIZEOF_VOID_P EQUAL 8 )
        set( KICAD_BUILD_ARCH "x64" )
        set( KICAD_BUILD_ARCH_X64 1 )
    endif()
endif()


if( MSVC )
    # This is a workaround borrowed from https://github.com/dotnet/runtime/blob/main/eng/native/configurecompiler.cmake
    # CMake currently cannot handle the fact that "armasm" is used in combination with standard MSVC "cl" for ARM targets
    # So this is basically a hack (incombination with the MSVCAssemblyHelper.cmake) to make this all work
    if(KICAD_BUILD_ARCH_ARM)
        message( "Configuring ARM assembler" )
        # Explicitly specify the assembler to be used for Arm32 compile
        file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}\\bin\\HostX86\\arm\\armasm.exe" CMAKE_ASM_COMPILER)

        set(CMAKE_ASM_MASM_COMPILER ${CMAKE_ASM_COMPILER})
        message("CMAKE_ASM_MASM_COMPILER explicitly set to: ${CMAKE_ASM_MASM_COMPILER}")

        # Enable generic assembly compilation to avoid CMake generate VS proj files that explicitly
        # use ml[64].exe as the assembler.
        enable_language(ASM)
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreaded         "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDLL      "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebug    "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL "")
        set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -g <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")

    elseif(KICAD_BUILD_ARCH_ARM64)
        message( "Configuring ARM64 assembler" )

        # Explicitly specify the assembler to be used for Arm64 compile
        file(TO_CMAKE_PATH "$ENV{VCToolsInstallDir}\\bin\\HostX86\\arm64\\armasm64.exe" CMAKE_ASM_COMPILER)

        set(CMAKE_ASM_MASM_COMPILER ${CMAKE_ASM_COMPILER})
        message("CMAKE_ASM_MASM_COMPILER explicitly set to: ${CMAKE_ASM_MASM_COMPILER}")

        # Enable generic assembly compilation to avoid CMake generate VS proj files that explicitly
        # use ml[64].exe as the assembler.
        enable_language(ASM)
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreaded         "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDLL      "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebug    "")
        set(CMAKE_ASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL "")
        set(CMAKE_ASM_COMPILE_OBJECT "<CMAKE_ASM_COMPILER> -g <INCLUDES> <FLAGS> -o <OBJECT> <SOURCE>")
    else()
        message( "Configuring MASM assembler" )
        if(KICAD_BUILD_ARCH_X86)
            set( CMAKE_ASM_MASM_FLAGS "${CMAKE_ASM_MASM_FLAGS} /safeseh" )
        endif()
        enable_language(ASM_MASM)

        set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreaded         "")
        set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDLL      "")
        set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebug    "")
        set(CMAKE_ASM_MASM_COMPILE_OPTIONS_MSVC_RUNTIME_LIBRARY_MultiThreadedDebugDLL "")
    endif()
endif()