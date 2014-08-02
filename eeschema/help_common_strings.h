/**
 * @file eeschema/help_common_strings.h
 * strings common to toolbars and menubar
 */

/**
 * These strings are used in menus and tools, that do the same command
 * But they are internationalized, and therefore must be created
 * at run time, on the fly.
 * So they cannot be static.
 *
 * Therefore they are defined by \#define, used inside menu constructors
 */

// Common to schematic editor and component editor
#define HELP_UNDO _( "Undo last command" )
#define HELP_REDO _( "Redo last command" )

#define HELP_ZOOM_IN     _( "Zoom in" )
#define HELP_ZOOM_OUT    _( "Zoom out" )
#define HELP_ZOOM_FIT    _( "Fit schematic sheet on screen" )
#define HELP_ZOOM_REDRAW _( "Redraw schematic view" )

#define HELP_DELETE_ITEMS         _( "Delete item" )

// Schematic editor:
#define HELP_FIND _( "Find components and text" )
#define HELP_REPLACE _( "Find and replace text in schematic items" )
#define HELP_PLACE_COMPONENTS     _( "Place component" )
#define HELP_PLACE_POWERPORT      _( "Place power port" )
#define HELP_PLACE_WIRE           _( "Place wire" )
#define HELP_PLACE_BUS            _( "Place bus" )
#define HELP_PLACE_WIRE2BUS_ENTRY _( "Place wire to bus entry" )
#define HELP_PLACE_BUS2BUS_ENTRY  _( "Place bus to bus entry" )
#define HELP_PLACE_NC_FLAG        _( "Place not-connected flag" )

#define HELP_PLACE_NETLABEL _( "Place net name - local label" )
#define HELP_PLACE_GLOBALLABEL \
    _(\
        "Place global label.\nWarning: inside global hierarchy , all global labels with same name are connected" )
#define HELP_PLACE_HIER_LABEL \
    _( "Place a hierarchical label. Label will be seen as a hierarchical pin in the sheet symbol" )

#define HELP_PLACE_JUNCTION     _( "Place junction" )
#define HELP_PLACE_SHEET        _( "Create hierarchical sheet" )
#define HELP_IMPORT_SHEETPIN    _( \
        "Place hierarchical pin imported from the corresponding hierarchical label" )
#define HELP_PLACE_SHEETPIN     _( "Place hierarchical pin in sheet" )
#define HELP_PLACE_GRAPHICLINES _( "Place graphic lines or polygons" )
#define HELP_PLACE_GRAPHICTEXTS _( "Place text" )

#define HELP_ANNOTATE _( "Annotate schematic components" )
#define HELP_RUN_LIB_EDITOR _( "Library Editor - Create/edit components" )
#define HELP_RUN_LIB_VIEWER _( "Library Browser - Browse components" )
#define HELP_GENERATE_BOM _( "Generate bill of materials and/or cross references" )
#define HELP_IMPORT_FOOTPRINTS \
    _( "Back-import component footprint fields via CvPcb .cmp file" )

// Component editor:
#define HELP_ADD_PIN _( "Add pins to component" )
#define HELP_ADD_BODYTEXT _( "Add text to component body" )
#define HELP_ADD_BODYRECT _( "Add graphic rectangle to component body" )
#define HELP_ADD_BODYCIRCLE _( "Add circles to component body" )
#define HELP_ADD_BODYARC _( "Add arcs to component body" )
#define HELP_ADD_BODYPOLYGON _( "Add lines and polygons to component body" )
#define HELP_PLACE_GRAPHICIMAGES _("Add bitmap image")
