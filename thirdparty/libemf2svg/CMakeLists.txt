find_package( PNG REQUIRED )
find_package( Iconv REQUIRED )

set( LIBEMF2SVG_SOURCES
    src/lib/emf2svg.c
    src/lib/emf2svg_clip_utils.c
    src/lib/emf2svg_img_utils.c
    src/lib/emf2svg_print.c
    src/lib/emf2svg_rec_bitmap.c
    src/lib/emf2svg_rec_clipping.c
    src/lib/emf2svg_rec_comment.c
    src/lib/emf2svg_rec_control.c
    src/lib/emf2svg_rec_drawing.c
    src/lib/emf2svg_rec_object_creation.c
    src/lib/emf2svg_rec_object_manipulation.c
    src/lib/emf2svg_rec_path.c
    src/lib/emf2svg_rec_state_record.c
    src/lib/emf2svg_rec_transform.c
    src/lib/emf2svg_utils.c
    src/lib/pmf2svg.c
    src/lib/pmf2svg_print.c
    vendor/libuemf/uemf.c
    vendor/libuemf/uemf_endian.c
    vendor/libuemf/uemf_safe.c
    vendor/libuemf/uemf_utf.c
    vendor/libuemf/upmf.c
    )

if( WIN32 )
    set( EXPORT_MACROS "#define FMEM_API" )
    configure_file( vendor/fmem/include/fmem.h.in fmem.h @ONLY )
    list( APPEND LIBEMF2SVG_SOURCES vendor/fmem/src/fmem-winapi-tmpfile.c )
endif()

add_library( emf2svg STATIC ${LIBEMF2SVG_SOURCES} )
set_target_properties( emf2svg PROPERTIES POSITION_INDEPENDENT_CODE ON )
target_include_directories( emf2svg
    PUBLIC inc
    PRIVATE vendor/libuemf ${CMAKE_CURRENT_BINARY_DIR}
    )
target_link_libraries( emf2svg PRIVATE PNG::PNG Iconv::Iconv Fontconfig::Fontconfig Freetype::Freetype )
target_compile_definitions( emf2svg PUBLIC EMF2SVG_STATIC PRIVATE _GNU_SOURCE )

if( APPLE )
    target_compile_definitions( emf2svg PRIVATE DARWIN )
endif()

if( CMAKE_C_BYTE_ORDER STREQUAL "BIG_ENDIAN" )
    target_compile_definitions( emf2svg PRIVATE WORDS_BIGENDIAN )
endif()
