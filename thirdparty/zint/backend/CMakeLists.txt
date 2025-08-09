# Copyright (C) 2008 by BogDan Vatra < bogdan@licentia.eu >
# Copyright (C) 2009-2025 Robin Stuart <rstuart114@gmail.com>
# vim: set ts=4 sw=4 et :

cmake_minimum_required(VERSION 3.10)
project(zint)

if(ZINT_USE_PNG)
    find_package(PNG)
endif()

set(zint_COMMON_SRCS common.c eci.c filemem.c general_field.c gs1.c large.c library.c reedsol.c)
set(zint_ONEDIM_SRCS 2of5.c 2of5inter.c 2of5inter_based.c bc412.c channel.c codabar.c code.c code11.c code128.c
                     code128_based.c dxfilmedge.c medical.c plessey.c rss.c telepen.c upcean.c)
set(zint_POSTAL_SRCS auspost.c imail.c mailmark.c postal.c)
set(zint_TWODIM_SRCS aztec.c codablock.c code1.c code16k.c code49.c composite.c dmatrix.c dotcode.c gridmtx.c
                     hanxin.c maxicode.c pdf417.c qr.c ultra.c)
if(ZINT_USE_PNG AND PNG_FOUND)
    set(zint_OUTPUT_SRCS bmp.c emf.c gif.c output.c pcx.c png.c ps.c raster.c svg.c tif.c vector.c)
else()
    set(zint_OUTPUT_SRCS bmp.c emf.c gif.c output.c pcx.c ps.c raster.c svg.c tif.c vector.c)
endif()
set(zint_SRCS ${zint_OUTPUT_SRCS} ${zint_COMMON_SRCS} ${zint_ONEDIM_SRCS} ${zint_POSTAL_SRCS} ${zint_TWODIM_SRCS})

if(ZINT_SHARED)
    add_library(zint SHARED ${zint_SRCS})

    if(WIN32)
        target_sources(${PROJECT_NAME} PRIVATE libzint.rc)
    endif()
endif()

if(ZINT_STATIC)
    add_library(zint-static STATIC ${zint_SRCS})
    if(WIN32)
        set_target_properties(zint-static PROPERTIES OUTPUT_NAME zint-static)
    else()
        set_target_properties(zint-static PROPERTIES OUTPUT_NAME zint)
    endif()
endif()

function(zint_target_link_libraries library)
    if(ZINT_SHARED)
        target_link_libraries(zint ${library})
    endif()
    if(ZINT_STATIC)
        target_link_libraries(zint-static ${library})
    endif()
endfunction()

function(zint_target_compile_definitions scope definition)
    if(ZINT_SHARED)
        target_compile_definitions(zint ${scope} ${definition})
    endif()
    if(ZINT_STATIC)
        target_compile_definitions(zint-static ${scope} ${definition})
    endif()
endfunction()

function(zint_target_compile_options scope option)
    if(ZINT_SHARED)
        target_compile_options(zint ${scope} ${option})
    endif()
    if(ZINT_STATIC)
        target_compile_options(zint-static ${scope} ${option})
    endif()
endfunction()

function(zint_target_include_directories)
    if(ZINT_SHARED)
        target_include_directories(zint ${ARGN})
    endif()
    if(ZINT_STATIC)
        target_include_directories(zint-static ${ARGN})
    endif()
endfunction()

if(ZINT_SHARED)
    set_target_properties(zint PROPERTIES   SOVERSION "${ZINT_VERSION_MAJOR}.${ZINT_VERSION_MINOR}"
                                            VERSION ${ZINT_VERSION})
endif()

if(ZINT_USE_PNG AND PNG_FOUND)
    zint_target_link_libraries(PNG::PNG)
    message(STATUS "Using PNG")
else()
    zint_target_compile_definitions(PRIVATE ZINT_NO_PNG)
    message(STATUS "Not using PNG")
endif()

# Incompatible with ZINT_SANITIZE (and also with ZINT_USE_PNG unless libpng instrumented)
if(NOT ZINT_SANITIZE AND ZINT_SANITIZEM AND CMAKE_C_COMPILER_ID MATCHES "Clang")
    add_compile_options(-fsanitize=memory -fsanitize-memory-track-origins -fno-omit-frame-pointer -O2)
    link_libraries(-fsanitize=memory)
    zint_target_compile_definitions(PRIVATE ZINT_SANITIZEM)
endif()

if(ZINT_TEST)
    zint_target_compile_definitions(PUBLIC ZINT_TEST)
endif()

check_c_compiler_flag("-Wc90-c99-compat" C_COMPILER_FLAG_WC90_C99_COMPAT)
if(C_COMPILER_FLAG_WC90_C99_COMPAT)
    zint_target_compile_options(PRIVATE "-Wc90-c99-compat")
endif()

if(NOT MSVC)
    # Link with standard C math library.
    zint_target_link_libraries(m)
endif()

if(MSVC)
    if(ZINT_SHARED)
        target_compile_definitions(zint PRIVATE DLL_EXPORT)
    endif()
endif()

zint_target_include_directories(PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# Adapted from old (2008) KDE "SetPaths.cmake" to use GNUInstallDirs
set(INSTALL_TARGETS_DEFAULT_ARGS RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
                                 LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
                                 ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT Devel)

if(ZINT_SHARED)
    install(TARGETS zint EXPORT zint-targets ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()
if(ZINT_STATIC)
    install(TARGETS zint-static EXPORT zint-targets ${INSTALL_TARGETS_DEFAULT_ARGS})
endif()
install(EXPORT zint-targets NAMESPACE zint:: DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/zint")
install(FILES zint.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT Devel)

if(ZINT_TEST)
    add_subdirectory(tests)
endif()
