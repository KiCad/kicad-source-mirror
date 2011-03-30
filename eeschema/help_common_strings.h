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
#define HELP_UNDO _( "Undo last edition" )
#define HELP_REDO _( "Redo the last undo command" )

#define HELP_ZOOM_IN     _( "Zoom in" )
#define HELP_ZOOM_OUT    _( "Zoom out" )
#define HELP_ZOOM_FIT    _( "Fit the schematic sheet on the screen" )
#define HELP_ZOOM_REDRAW _( "Redraw the schematic view" )

#define HELP_DELETE_ITEMS         _( "Delete items" )

// Schematic editor:
#define HELP_FIND _( "Find components and texts" )

#define HELP_PLACE_COMPONENTS     _( "Place a component" )
#define HELP_PLACE_POWERPORT      _( "Place a power port" )
#define HELP_PLACE_WIRE           _( "Place a wire" )
#define HELP_PLACE_BUS            _( "Place a bus" )
#define HELP_PLACE_WIRE2BUS_ENTRY _( "Place a wire to bus entry" )
#define HELP_PLACE_BUS2BUS_ENTRY  _( "Place a bus to bus entry" )
#define HELP_PLACE_NC_FLAG        _( "Place a no connect flag" )

#define HELP_PLACE_NETLABEL _( "Place a net name (local label)" )
#define HELP_PLACE_GLOBALLABEL \
    _(\
        "Place a global label.\nWarning: all global labels with the same name are connected in whole hierarchy" )
#define HELP_PLACE_HIER_LABEL \
    _( "Place a hierarchical label. This label will be seen as a hierarchical pin in the sheet symbol" )

#define HELP_PLACE_JUNCTION     _( "Place a junction" )
#define HELP_PLACE_SHEET        _( "Create a hierarchical sheet" )
#define HELP_IMPORT_SHEETPIN    _( \
        "Place a hierarchical pin imported from the corresponding hierarchical label in sheet" )
#define HELP_PLACE_SHEETPIN     _( "Place a hierarchical pin in sheet" )
#define HELP_PLACE_GRAPHICLINES _( "Place graphic lines or polygons" )
#define HELP_PLACE_GRAPHICTEXTS _( "Place graphic text (comment)" )


// Component editor:
#define HELP_ADD_PIN _( "Add pins to the component" )
#define HELP_ADD_BODYTEXT _( "Add graphic texts to the component body" )
#define HELP_ADD_BODYRECT _( "Add graphic rectangles to the component body" )
#define HELP_ADD_BODYCIRCLE _( "Add circles to the component body" )
#define HELP_ADD_BODYARC _( "Add arcs to the component body" )
#define HELP_ADD_BODYPOLYGON _( "Add lines and polygons to the component body" )
