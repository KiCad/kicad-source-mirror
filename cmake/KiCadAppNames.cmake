# Derive the Linux/Unix desktop application identifiers shared by the resource
# metadata generation and the runtime Wayland app_id (see PGM_BASE::InitPgm).
#
# We use two different app IDs here for legacy reasons. The org.kicad.KiCad ID is
# from FlatHub Flatpaks, while the org.kicad.kicad ID is used everywhere else.
# Having these two be separate causes problems in Gnome Software because they will
# each appear as their own app, so KiCad gets two entries. To work around this, we
# include a provides statement in the metainfo for the other app ID to link the two
# IDs together.

# Default values for regular builds
set( KICAD_REVERSE_DOMAIN "org.kicad" )
set( KICAD_APP_NAME "${KICAD_REVERSE_DOMAIN}.kicad" )
set( KICAD_PROVIDES_APP_ID "${KICAD_REVERSE_DOMAIN}.KiCad" )
set( KICAD_APP_PREFIX "${KICAD_REVERSE_DOMAIN}" )
set( KICAD_ICON_PREFIX "" )
set( KICAD_DESKTOP_FILE_ICON_PREFIX "" )
set( KICAD_DESKTOP_FILE_ICON_KICAD "kicad" )
set( KICAD_MIME_FILE_PREFIX "kicad" )
set( KICAD_MIME_ICON_PREFIX "" )

# Override default values from above if we are building a flatpak
if( KICAD_BUILD_FLATPAK )
    set( KICAD_APP_NAME "${KICAD_REVERSE_DOMAIN}.KiCad" )
    set( KICAD_PROVIDES_APP_ID "${KICAD_REVERSE_DOMAIN}.kicad" )
    if( KICAD_BUILD_NIGHTLY_FLATPAK )
        set( KICAD_APP_NAME "${KICAD_APP_NAME}.Nightly" )
        set( KICAD_PROVIDES_APP_ID "${KICAD_PROVIDES_APP_ID}.Nightly" )
    endif()
    set( KICAD_APP_PREFIX "${KICAD_APP_NAME}" )
    set( KICAD_ICON_PREFIX "${KICAD_APP_NAME}" )
    set( KICAD_DESKTOP_FILE_ICON_PREFIX "${KICAD_APP_PREFIX}." )
    set( KICAD_DESKTOP_FILE_ICON_KICAD "${KICAD_APP_NAME}" )
    set( KICAD_MIME_FILE_PREFIX "${KICAD_APP_PREFIX}" )
    set( KICAD_MIME_ICON_PREFIX "${KICAD_APP_PREFIX}." )
endif()
