find_package( PNG REQUIRED )
find_package( JPEG REQUIRED )

set( LIBWMF_SOURCES
    src/api.c
    src/bbuf.c
    src/font.c
    src/meta.c
    src/player.c
    src/recorder.c
    src/stream.c
    src/wmf.c
    src/xml.c
    src/ipa/foreign.c
    src/ipa/ipa.c
    src/ipa/xgd.c
    src/extra/gd/gd.c
    src/extra/gd/gd_clip.c
    src/extra/gd/gd_gd.c
    src/extra/gd/gd_gd2.c
    src/extra/gd/gd_io.c
    src/extra/gd/gd_io_dp.c
    src/extra/gd/gd_io_file.c
    src/extra/gd/gd_io_ss.c
    src/extra/gd/gd_jpeg.c
    src/extra/gd/gd_png.c
    src/extra/gd/gd_ss.c
    src/extra/gd/gd_wbmp.c
    src/extra/gd/gdcache.c
    src/extra/gd/gdfontg.c
    src/extra/gd/gdfontl.c
    src/extra/gd/gdfontmb.c
    src/extra/gd/gdfonts.c
    src/extra/gd/gdfontt.c
    src/extra/gd/gdft.c
    src/extra/gd/gdhelpers.c
    src/extra/gd/gdkanji.c
    src/extra/gd/gdtables.c
    src/extra/gd/gdxpm.c
    src/extra/gd/wbmp.c
    )

add_library( libwmf_orcad STATIC ${LIBWMF_SOURCES} )

target_include_directories( libwmf_orcad
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/extra/gd
        ${FREETYPE_INCLUDE_DIRS}
    )

target_compile_definitions( libwmf_orcad
    PUBLIC
        LIBWMF_STATIC=1
    PRIVATE
        HAVE_GD=1
        HAVE_LIBFREETYPE=1
        HAVE_LIBJPEG=1
        HAVE_LIBPNG=1
        PACKAGE="libwmf"
        VERSION="0.2.15"
        WMF_FONTDIR=""
        WMF_GS_FONTDIR=""
        WMF_GS_FONTMAP=""
        WMF_SYS_FONTMAP=""
        WMF_XTRA_FONTMAP=""
    )

if( UNIX )
    target_compile_definitions( libwmf_orcad PRIVATE HAVE_UNISTD_H=1 )
endif()

target_link_libraries( libwmf_orcad
    PUBLIC
        Freetype::Freetype
        JPEG::JPEG
        PNG::PNG
        ZLIB::ZLIB
    )

if( NOT WIN32 )
    target_link_libraries( libwmf_orcad PUBLIC m )
endif()

install( DIRECTORY fonts/ DESTINATION ${KICAD_DATA}/libwmf/fonts )

file( COPY fonts DESTINATION ${CMAKE_BINARY_DIR}/libwmf )
