/**
 * @file pcbnew/help_common_strings.h
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

#define HELP_UNDO _( "Undo last edition" )
#define HELP_REDO _( "Redo the last undo command" )
#define HELP_FIND _( "Find components and text in current loaded board" )

#define HELP_ZOOM_IN     _( "Zoom in" )
#define HELP_ZOOM_OUT    _( "Zoom out" )
#define HELP_ZOOM_FIT    _( "Zoom to fit the board on the screen" )
#define HELP_ZOOM_REDRAW _( "Redraw the screen of the board" )

#define HELP_SHOW_HIDE_LAYERMANAGER _( "Show/hide the layers manager toolbar" )

#define HELP_SHOW_HIDE_MICROWAVE_TOOLS _( "Show/hide the toolbar for microwave tools\nThis is a experimental feature (under development)" )
