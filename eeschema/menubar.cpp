/**
 * @file eeschema/menubar.cpp
 * @brief (Re)Create the main menubar for the schematic frame
 */
#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "bitmaps.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "hotkeys.h"
#include "macros.h"

#include "help_common_strings.h"

/**
 * @brief (Re)Create the menubar for the schematic frame
 */
void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    // Create and try to get the current menubar
    wxString    text;
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    if( ! menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove(0);

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // New
    item = new wxMenuItem( fileMenu,
                           ID_NEW_PROJECT,
                           _( "&New\tCtrl+N" ),
                           _( "New schematic project" ) );
    SET_BITMAP( new_xpm );
    fileMenu->Append( item );

    // Open
    item = new wxMenuItem( fileMenu,
                           ID_LOAD_PROJECT,
                           _( "&Open\tCtrl+O" ),
                           _( "Open an existing schematic project" ) );
    SET_BITMAP( open_document_xpm );
    fileMenu->Append( item );

    // Open Recent submenu
    wxMenu* openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, openRecentMenu,
                                        -1, _( "Open &Recent" ),
                                        _( "Open a recent opened schematic project" ),
                                        open_project_xpm );
    // Separator
    fileMenu->AppendSeparator();

    // Save schematic project
    item = new wxMenuItem( fileMenu,
                           ID_SAVE_PROJECT,
                           _( "&Save Whole Schematic Project\tCtrl+S" ),
                           _( "Save all sheets in the schematic project" ) );
    SET_BITMAP( save_project_xpm );
    fileMenu->Append( item );

    // Save current sheet
    item = new wxMenuItem( fileMenu,
                           ID_SAVE_ONE_SHEET,
                           _( "Save &Current Sheet Only" ),
                           _( "Save only current schematic sheet" ) );
    SET_BITMAP( save_xpm );
    fileMenu->Append( item );

    // Save current sheet as
    item = new wxMenuItem( fileMenu,
                           ID_SAVE_ONE_SHEET_AS,
                           _( "Save Current Sheet &as" ),
                           _( "Save current schematic sheet as..." ) );
    SET_BITMAP( save_as_xpm );
    fileMenu->Append( item );

    // Separator
    fileMenu->AppendSeparator();

    // Print
    item = new wxMenuItem( fileMenu,
                           wxID_PRINT,
                           _( "P&rint" ),
                           _( "Print schematic" ) );
    SET_BITMAP( print_button );
    fileMenu->Append( item );

    // Plot submenu
    wxMenu* choice_plot_fmt = new wxMenu;

    // Plot PostScript
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_PS,
                           _( "Plot PostScript" ),
                           _( "Plot schematic sheet in PostScript format" ) );
    SET_BITMAP( plot_PS_xpm );
    choice_plot_fmt->Append( item );

    // Plot HPGL
    item = new wxMenuItem( choice_plot_fmt,
                           ID_GEN_PLOT_HPGL,
                           _( "Plot HPGL" ),
                           _( "Plot schematic sheet in HPGL format" ) );
    SET_BITMAP( plot_HPG_xpm );
    choice_plot_fmt->Append( item );

    // Plot SVG
    item = new wxMenuItem( choice_plot_fmt,
                           ID_GEN_PLOT_SVG,
                           _( "Plot SVG" ),
                           _( "Plot schematic sheet in SVG format" ) );
    SET_BITMAP( plot_xpm );
    choice_plot_fmt->Append( item );

    // Plot DXF
    item = new wxMenuItem( choice_plot_fmt,
                           ID_GEN_PLOT_DXF,
                           _( "Plot DXF" ),
                           _( "Plot schematic sheet in DXF format" ) );
    SET_BITMAP( plot_xpm );
    choice_plot_fmt->Append( item );

    // Plot to Clipboard (Windows only)
#ifdef __WINDOWS__

    item = new wxMenuItem( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                           _( "Plot to Clipboard" ),
                           _( "Export drawings to clipboard" ) );
    SET_BITMAP( copy_button );
    choice_plot_fmt->Append( item );

#endif // __WINDOWS__

    // Plot submenu
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, choice_plot_fmt,
                                        ID_GEN_PLOT, _( "&Plot" ),
                                        _( "Plot schematic sheet in HPGL, PostScript or SVG format" ),
                                        plot_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    item = new wxMenuItem( fileMenu,
                           wxID_EXIT,
                           _( "&Quit" ),
                           _( "Quit EESchema" ) );
    SET_BITMAP( exit_xpm );
    fileMenu->Append( item );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "Undo" ), s_Schematic_Hokeys_Descr, HK_UNDO );

    item = new wxMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, wxITEM_NORMAL );
    SET_BITMAP( undo_xpm );
    editMenu->Append( item );

    // Redo
    text = AddHotkeyName( _( "Redo" ), s_Schematic_Hokeys_Descr, HK_REDO );

    item = new wxMenuItem( editMenu, wxID_REDO, text, HELP_REDO, wxITEM_NORMAL );
    SET_BITMAP( redo_xpm );
    editMenu->Append( item );

    // Delete
    editMenu->AppendSeparator();
    item = new wxMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                           _( "Delete" ), HELP_DELETE_ITEMS, wxITEM_NORMAL );
    SET_BITMAP( delete_body_xpm );
    editMenu->Append( item );

    // Separator
    editMenu->AppendSeparator();

    // Find
    text = AddHotkeyName( _( "&Find" ), s_Schematic_Hokeys_Descr, HK_FIND_ITEM );
    item = new wxMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND, wxITEM_NORMAL );
    SET_BITMAP( find_xpm );
    editMenu->Append( item );

    // Separator
    editMenu->AppendSeparator();

    // Backannotate
    item = new wxMenuItem( editMenu,
                           ID_BACKANNO_ITEMS,
                           _( "Backannotate" ),
                           _( "Back annotated footprint fields" ),
                           wxITEM_NORMAL );
    SET_BITMAP( backanno_xpm );
    editMenu->Append( item );

    // Menu View:
    wxMenu* viewMenu = new wxMenu;

    /**
     * Important Note for ZOOM IN and ZOOM OUT commands from menubar:
     * we cannot add hotkey shortcut here, because the hotkey HK_ZOOM_IN and HK_ZOOM_OUT
     * events(default = WXK_F1 and WXK_F2) are *NOT* equivalent to this menu command:
     * zoom in and out from hotkeys are equivalent to the pop up menu zoom
     * From here, zooming is made around the screen center
     * From hotkeys, zooming is made around the mouse cursor position
     * (obviously not possible from the toolbar or menubar command)
     *
     * in others words HK_ZOOM_IN and HK_ZOOM_OUT *are NOT* accelerators
     * for Zoom in and Zoom out sub menus
     * SO WE ADD THE NAME OF THE CORRESPONDING HOTKEY AS A COMMENT, NOT AS A SHORTCUT
     * using in AddHotkeyName call the option "false" (not a shortcut)
     */

    // Zoom in
    text = AddHotkeyName( _( "Zoom In" ), s_Schematic_Hokeys_Descr,
                          ID_ZOOM_IN, false );  // add comment, not a shortcut
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, wxITEM_NORMAL );
    SET_BITMAP( zoom_in_xpm );
    viewMenu->Append( item );

    // Zoom out
    text = AddHotkeyName( _( "Zoom Out" ), s_Schematic_Hokeys_Descr,
                          ID_ZOOM_OUT, false );  // add comment, not a shortcut
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, wxITEM_NORMAL );
    SET_BITMAP( zoom_out_xpm );
    viewMenu->Append( item );

    // Fit on screen
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, wxITEM_NORMAL );
    SET_BITMAP( zoom_fit_in_page_xpm );
    viewMenu->Append( item );

    // Separator
    viewMenu->AppendSeparator();

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, wxITEM_NORMAL );
    SET_BITMAP( zoom_redraw_xpm );
    viewMenu->Append( item );

    // Menu place:
    // @todo unify IDs
    wxMenu* placeMenu = new wxMenu;

    // Component
    text = AddHotkeyName( _( "Component" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_COMPONENT, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_SCH_PLACE_COMPONENT, text,
                           HELP_PLACE_COMPONENTS, wxITEM_NORMAL );
    SET_BITMAP( add_component_xpm );
    placeMenu->Append( item );

    // Power port
    text = AddHotkeyName( _( "Power port" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_POWER, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_PLACE_POWER_BUTT, text,
                           HELP_PLACE_POWERPORT, wxITEM_NORMAL );
    SET_BITMAP( add_power_xpm );
    placeMenu->Append( item );

    // Wire
    text = AddHotkeyName( _( "Wire" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_WIRE, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_WIRE_BUTT, text,
                           HELP_PLACE_WIRE, wxITEM_NORMAL );
    SET_BITMAP( add_line_xpm );
    placeMenu->Append( item );

    // Bus
    text = AddHotkeyName( _( "Bus" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_BUS, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_BUS_BUTT, text,
                           HELP_PLACE_BUS, wxITEM_NORMAL );
    SET_BITMAP( add_bus_xpm );
    placeMenu->Append( item );

    // Wire to Bus entry
    text = AddHotkeyName( _( "Wire to bus entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_WIRE_ENTRY, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT, text,
                           HELP_PLACE_WIRE2BUS_ENTRY, wxITEM_NORMAL );
    SET_BITMAP( add_line2bus_xpm );
    placeMenu->Append( item );

    // Bus to Bus entry
    text = AddHotkeyName( _( "Bus to bus entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_BUS_ENTRY, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT, text,
                           HELP_PLACE_BUS2BUS_ENTRY, wxITEM_NORMAL );
    SET_BITMAP( add_bus2bus_xpm );
    placeMenu->Append( item );

    // No connect flag
    text = AddHotkeyName( _( "No connect flag" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NOCONN_FLAG, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG, wxITEM_NORMAL );
    SET_BITMAP( noconn_button );
    placeMenu->Append( item );

    // Net name
    text = AddHotkeyName( _( "Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_LABEL, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_LABEL_BUTT, text,
                           HELP_PLACE_NETLABEL, wxITEM_NORMAL  );
    SET_BITMAP( add_line_label_xpm );
    placeMenu->Append( item );

    // Global label
    text = AddHotkeyName( _( "Global label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GLABEL, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_GLABEL_BUTT, text,
                           HELP_PLACE_GLOBALLABEL, wxITEM_NORMAL );
    SET_BITMAP( add_glabel_xpm );
    placeMenu->Append( item );

    // Junction
    text = AddHotkeyName( _( "Junction" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_JUNCTION, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_JUNCTION_BUTT, text,
                           HELP_PLACE_JUNCTION, wxITEM_NORMAL );
    SET_BITMAP( add_junction_xpm );
    placeMenu->Append( item );

    // Separator
    placeMenu->AppendSeparator();

    // Hierarchical label
    text = AddHotkeyName( _( "Hierarchical label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, false );    // add comment, not a shortcut
    text = AddHotkeyName( _( "Hierarchical label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_HIERLABEL_BUTT,
                            text, HELP_PLACE_HIER_LABEL,
                            add_hierarchical_label_xpm );


    // Hierarchical sheet
    text = AddHotkeyName( _( "Hierarchical sheet" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HIER_SHEET, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT, text,
                           HELP_PLACE_SHEET, wxITEM_NORMAL );
    SET_BITMAP( add_hierarchical_subsheet_xpm );
    placeMenu->Append( item );

    // Import hierarchical sheet
    item = new wxMenuItem( placeMenu,
                           ID_IMPORT_HLABEL_BUTT,
                           _( "Import Hierarchical Label" ),
                           HELP_IMPORT_SHEETPIN, wxITEM_NORMAL );
    SET_BITMAP( import_hierarchical_label_xpm );
    placeMenu->Append( item );

    // Add hierarchical Pin to Sheet
    item = new wxMenuItem( placeMenu,
                           ID_SHEET_PIN_BUTT,
                           _( "Add Hierarchical Pin to Sheet" ),
                           HELP_PLACE_SHEETPIN, wxITEM_NORMAL );
    SET_BITMAP( add_hierar_pin_xpm );
    placeMenu->Append( item );

    // Separator
    placeMenu->AppendSeparator();

    // Graphic line or polygon
    text = AddHotkeyName( _( "Graphic polyline" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_POLYLINE, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_LINE_COMMENT_BUTT, text,
                           HELP_PLACE_GRAPHICLINES, wxITEM_NORMAL );
    SET_BITMAP( add_dashed_line_xpm );
    placeMenu->Append( item );

    // Graphic text
    text = AddHotkeyName( _( "Graphic text" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_TEXT, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT, text,
                           HELP_PLACE_GRAPHICTEXTS, wxITEM_NORMAL );
    SET_BITMAP( add_text_xpm );
    placeMenu->Append( item );


    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_REQ,
                           _( "&Library" ),
                           _( "Library preferences" ) );
    SET_BITMAP( library_xpm );
    preferencesMenu->Append( item );

    // Colors
    item = new wxMenuItem( preferencesMenu,
                           ID_COLORS_SETUP,
                           _( "&Colors" ),
                           _( "Color preferences" ) );
    SET_BITMAP( palette_xpm );
    preferencesMenu->Append( item );

    // Options (Preferences on WXMAC)
    item = new wxMenuItem( preferencesMenu,
                           wxID_PREFERENCES,
#ifdef __WXMAC__
                           _( "&Preferences..." ),
#else
                           _( "&Options" ),
#endif // __WXMAC__
                           _( "EESchema preferences" ) );
    SET_BITMAP( preference_xpm );
    preferencesMenu->Append( item );

    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    // Save preferences
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_SAVE,
                           _( "&Save preferences" ),
                           _( "Save application preferences" ) );
    SET_BITMAP( save_setup_xpm );
    preferencesMenu->Append( item );

    // Read preferences
    item = new wxMenuItem( preferencesMenu,
                           ID_CONFIG_READ,
                           _( "&Read preferences" ),
                           _( "Read application preferences" ) );
    SET_BITMAP( read_setup_xpm );
    preferencesMenu->Append( item );

    // Help Menu:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    item = new wxMenuItem( helpMenu,
                           ID_GENERAL_HELP,
                           _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    SET_BITMAP( online_help_xpm );
    helpMenu->Append( item );

    // About EESchema
    item = new wxMenuItem( helpMenu,
                           wxID_ABOUT,
                           _( "&About EESchema" ),
                           _( "About EESchema schematic designer" ) );
    SET_BITMAP( info_xpm );
    helpMenu->Append( item );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu,   _( "&File" ) );
    menuBar->Append( editMenu,   _( "&Edit" ) );
    menuBar->Append( viewMenu,   _( "&View" ) );
    menuBar->Append( placeMenu,  _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu,   _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
