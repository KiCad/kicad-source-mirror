/**
 * @file eeschema/menubar.cpp
 * @brief (Re)Create the main menubar for the schematic frame
 */
#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "appl_wxstruct.h"
#include "wxEeschemaStruct.h"

#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "hotkeys.h"

#include "help_common_strings.h"

/**
 * @brief (Re)Create the menubar for the schematic frame
 */
void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    // Create and try to get the current menubar
    wxString   text;
    wxMenuBar* menuBar = GetMenuBar();

    if( !menuBar )
        menuBar = new wxMenuBar();

    // Delete all existing menus so they can be rebuilt.
    // This allows language changes of the menu text on the fly.
    menuBar->Freeze();
    while( menuBar->GetMenuCount() )
        delete menuBar->Remove( 0 );

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;

    // New
    AddMenuItem( fileMenu,
                ID_NEW_PROJECT,
                _( "&New\tCtrl+N" ),
                _( "New schematic project" ),
                KiBitmap( new_xpm ) );

    // Open
    AddMenuItem( fileMenu,
                ID_LOAD_PROJECT,
                _( "&Open\tCtrl+O" ),
                _( "Open an existing schematic project" ),
                KiBitmap( open_document_xpm ) );

    // Open Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    AddMenuItem( fileMenu, openRecentMenu,
                wxID_ANY, _( "Open &Recent" ),
                _( "Open a recent opened schematic project" ),
                KiBitmap( open_project_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Save schematic project
    AddMenuItem( fileMenu,
                ID_SAVE_PROJECT,
                _( "&Save Whole Schematic Project\tCtrl+S" ),
                _( "Save all sheets in the schematic project" ),
                KiBitmap( save_project_xpm ) );

    // Save current sheet
    AddMenuItem( fileMenu,
                ID_SAVE_ONE_SHEET,
                _( "Save &Current Sheet Only" ),
                _( "Save only current schematic sheet" ),
                KiBitmap( save_xpm ) );

    // Save current sheet as
    AddMenuItem( fileMenu,
                ID_SAVE_ONE_SHEET_AS,
                _( "Save Current Sheet &as" ),
                _( "Save current schematic sheet as..." ),
                KiBitmap( save_as_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Page settings
    AddMenuItem( fileMenu,
                ID_SHEET_SET,
                _( "P&age Settings" ),
                _( "Settigns for page size and information" ),
                KiBitmap( sheetset_xpm ) );

    // Print
    AddMenuItem( fileMenu,
                wxID_PRINT,
                _( "P&rint" ),
                _( "Print schematic" ),
                KiBitmap( print_button_xpm ) );

    // Plot submenu
    wxMenu* choice_plot_fmt = new wxMenu;

    // Plot PostScript
    AddMenuItem( choice_plot_fmt, ID_GEN_PLOT_PS,
                _( "Plot PostScript" ),
                _( "Plot schematic sheet in PostScript format" ),
                KiBitmap( plot_ps_xpm ) );

    // Plot HPGL
    AddMenuItem( choice_plot_fmt,
                ID_GEN_PLOT_HPGL,
                _( "Plot HPGL" ),
                _( "Plot schematic sheet in HPGL format" ),
                KiBitmap( plot_hpg_xpm ) );

    // Plot SVG
    AddMenuItem( choice_plot_fmt,
                ID_GEN_PLOT_SVG,
                _( "Plot SVG" ),
                _( "Plot schematic sheet in SVG format" ),
                KiBitmap( plot_xpm ) );

    // Plot DXF
    AddMenuItem( choice_plot_fmt,
                ID_GEN_PLOT_DXF,
                _( "Plot DXF" ),
                _( "Plot schematic sheet in DXF format" ),
                KiBitmap( plot_xpm ) );

    // Plot to Clipboard (Windows only)
#ifdef __WINDOWS__

    AddMenuItem( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                _( "Plot to Clipboard" ),
                _( "Export drawings to clipboard" ),
                KiBitmap( copy_button_xpm ) );

#endif // __WINDOWS__

    // Plot submenu
    AddMenuItem( fileMenu, choice_plot_fmt,
                ID_GEN_PLOT, _( "&Plot" ),
                _( "Plot schematic sheet in HPGL, PostScript or SVG format" ),
                KiBitmap( plot_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    AddMenuItem( fileMenu,
                wxID_EXIT,
                _( "&Quit" ),
                _( "Quit Eeschema" ),
                KiBitmap( exit_xpm ) );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "Undo" ), s_Schematic_Hokeys_Descr, HK_UNDO );

    AddMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "Redo" ), s_Schematic_Hokeys_Descr, HK_REDO );

    AddMenuItem( editMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    // Delete
    editMenu->AppendSeparator();
    AddMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                _( "Delete" ), HELP_DELETE_ITEMS,
                KiBitmap( delete_body_xpm ) );

    // Find
    editMenu->AppendSeparator();
    text = AddHotkeyName( _( "&Find" ), s_Schematic_Hokeys_Descr, HK_FIND_ITEM );
    AddMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND, KiBitmap( find_xpm ) );

    // Backannotate
    editMenu->AppendSeparator();
    AddMenuItem( editMenu,
                ID_BACKANNO_ITEMS,
                _( "&Backannotate" ),
                _( "Back annotate the footprint fields" ),
                KiBitmap( import_footprint_names_xpm ) );

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
                          HK_ZOOM_IN, IS_ACCELERATOR );  // add an accelerator, not a shortcut
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    // Zoom out
    text = AddHotkeyName( _( "Zoom Out" ), s_Schematic_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );  // add accelerator, not a shortcut
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    // Fit on screen
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );

    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, KiBitmap( zoom_fit_in_page_xpm ) );

    // Separator
    viewMenu->AppendSeparator();

    // Hierarchy
    AddMenuItem( viewMenu,
                ID_HIERARCHY,
                _( "H&ierarchy" ),
                _( "Navigate schematic hierarchy" ),
                KiBitmap( hierarchy_nav_xpm ) );

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    // Menu place:
    // @todo unify IDs
    wxMenu* placeMenu = new wxMenu;

    // Component
    text = AddHotkeyName( _( "Component" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_COMPONENT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_SCH_PLACE_COMPONENT, text,
                HELP_PLACE_COMPONENTS,
                KiBitmap( add_component_xpm ) );

    // Power port
    text = AddHotkeyName( _( "Power Port" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_POWER, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_PLACE_POWER_BUTT, text,
                HELP_PLACE_POWERPORT,
                KiBitmap( add_power_xpm ) );

    // Wire
    text = AddHotkeyName( _( "Wire" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_WIRE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_WIRE_BUTT, text,
                HELP_PLACE_WIRE,
                KiBitmap( add_line_xpm ) );

    // Bus
    text = AddHotkeyName( _( "Bus" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_BUS, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_BUS_BUTT, text,
                HELP_PLACE_BUS,
                KiBitmap( add_bus_xpm ) );

    // Wire to Bus entry
    text = AddHotkeyName( _( "Wire to Bus Entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_WIRE_ENTRY, IS_ACCELERATOR );    // addan accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT, text,
                HELP_PLACE_WIRE2BUS_ENTRY,
                KiBitmap( add_line2bus_xpm ) );

    // Bus to Bus entry
    text = AddHotkeyName( _( "Bus to Bus Entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_BUS_ENTRY, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT, text,
                HELP_PLACE_BUS2BUS_ENTRY,
                KiBitmap( add_bus2bus_xpm ) );

    // No Connect Flag
    text = AddHotkeyName( _( "No Connect Flag" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NOCONN_FLAG, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG, KiBitmap( noconn_xpm ) );

    // Net name
    text = AddHotkeyName( _( "Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_LABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_LABEL_BUTT, text,
                HELP_PLACE_NETLABEL,
                KiBitmap( add_line_label_xpm ) );

    // Global label
    text = AddHotkeyName( _( "Global Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GLABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_GLABEL_BUTT, text,
                HELP_PLACE_GLOBALLABEL,
                KiBitmap( add_glabel_xpm ) );

    // Junction
    text = AddHotkeyName( _( "Junction" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_JUNCTION, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_JUNCTION_BUTT, text,
                HELP_PLACE_JUNCTION,
                KiBitmap( add_junction_xpm ) );

    // Separator
    placeMenu->AppendSeparator();

    // Hierarchical label
    text = AddHotkeyName( _( "Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, IS_ACCELERATOR );          // add an accelerator, not a shortcut
    text = AddHotkeyName( _( "Hierarchical Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, IS_ACCELERATOR );          // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_HIERLABEL_BUTT,
                text, HELP_PLACE_HIER_LABEL,
                KiBitmap( add_hierarchical_label_xpm ) );


    // Hierarchical sheet
    text = AddHotkeyName( _( "Hierarchical Sheet" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HIER_SHEET, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT, text,
                HELP_PLACE_SHEET,
                KiBitmap( add_hierarchical_subsheet_xpm ) );

    // Import hierarchical sheet
    AddMenuItem( placeMenu,
                ID_IMPORT_HLABEL_BUTT,
                _( "Import Hierarchical Label" ),
                HELP_IMPORT_SHEETPIN,
                KiBitmap( import_hierarchical_label_xpm ) );

    // Add hierarchical Pin to Sheet
    AddMenuItem( placeMenu,
                ID_SHEET_PIN_BUTT,
                _( "Hierarchical Pin to Sheet" ),
                HELP_PLACE_SHEETPIN,
                KiBitmap( add_hierar_pin_xpm ) );

    // Separator
    placeMenu->AppendSeparator();

    // Graphic line or polygon
    text = AddHotkeyName( _( "Graphic Polyline" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_POLYLINE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_LINE_COMMENT_BUTT, text,
                HELP_PLACE_GRAPHICLINES,
                KiBitmap( add_dashed_line_xpm ) );

    // Graphic text
    text = AddHotkeyName( _( "Graphic Text" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_TEXT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT, text,
                HELP_PLACE_GRAPHICTEXTS,
                KiBitmap( add_text_xpm ) );

    // Graphic image
    AddMenuItem( placeMenu, ID_ADD_IMAGE_BUTT, _( "Image" ),
                HELP_PLACE_GRAPHICIMAGES,
                KiBitmap( image_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    AddMenuItem( preferencesMenu,
                ID_CONFIG_REQ,
                _( "&Library" ),
                _( "Library preferences" ),
                KiBitmap( library_xpm ) );

    // Colors
    AddMenuItem( preferencesMenu,
                ID_COLORS_SETUP,
                _( "&Colors" ),
                _( "Color preferences" ),
                KiBitmap( palette_xpm ) );

    // Options (Preferences on WXMAC)

#ifdef __WXMAC__
    preferencesMenu->Append( wxID_PREFERENCES );
#else
    AddMenuItem( preferencesMenu,
                wxID_PREFERENCES,
                _( "&Options" ),
                _( "Eeschema preferences" ),
                KiBitmap( preference_xpm ) );
#endif // __WXMAC__


    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    // Save preferences
    AddMenuItem( preferencesMenu,
                ID_CONFIG_SAVE,
                _( "&Save Preferences" ),
                _( "Save application preferences" ),
                KiBitmap( save_setup_xpm ) );

    // Read preferences
    AddMenuItem( preferencesMenu,
                ID_CONFIG_READ,
                _( "&Read Preferences" ),
                _( "Read application preferences" ),
                KiBitmap( read_setup_xpm ) );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;

    // Library viewer
    AddMenuItem( toolsMenu,
                ID_TO_LIBRARY,
                _( "Library &Browser" ),
                _( "Library browser" ),
                KiBitmap( library_browse_xpm ) );


    // Library editor
    AddMenuItem( toolsMenu,
                ID_TO_LIBRARY,
                _( "Library &Editor" ),
                _( "Library editor" ),
                KiBitmap( libedit_xpm ) );

    // Separator
    toolsMenu->AppendSeparator();

    // Annotate
    AddMenuItem( toolsMenu,
                ID_GET_ANNOTATE,
                _( "&Annotate" ),
                _( "Annotate the components in the schematic" ),
                KiBitmap( annotate_xpm ) );

    // ERC
    AddMenuItem( toolsMenu,
                ID_GET_ERC,
                _( "ER&C" ),
                _( "Perform electrical rule check" ),
                KiBitmap( erc_xpm ) );

    // Generate netlist
    AddMenuItem( toolsMenu,
                ID_GET_NETLIST,
                _( "Generate &Netlist" ),
                _( "Generate the component netlist" ),
                KiBitmap( netlist_xpm ) );

    // Generate bill of materials
    AddMenuItem( toolsMenu,
                ID_GET_TOOLS,
                _( "Generate Bill of Materials" ),
                _( "Generate bill of materials" ),
                KiBitmap( tools_xpm ) );

    // Separator
    toolsMenu->AppendSeparator();

    //Run CvPcb
    AddMenuItem( toolsMenu,
                ID_TO_CVPCB,
                _( "A&ssign Component Footprints" ),
                _( "Run CvPcb" ),
                KiBitmap( cvpcb_xpm ) );

    // Run Pcbnew
    AddMenuItem( toolsMenu,
                ID_TO_PCB,
                _( "&Layout Printed Circuit Board" ),
                _( "Run Pcbnew" ),
                KiBitmap( pcbnew_xpm ) );


    // Help Menu:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    AddMenuItem( helpMenu,
                wxID_HELP,
                _( "&Contents" ),
                _( "Open the Eeschema handbook" ),
                KiBitmap( online_help_xpm ) );
    AddMenuItem( helpMenu,
                wxID_INDEX,
                _( "&Getting Started in KiCad" ),
                _( "Open the \"Getting Started in KiCad\" guide for beginners" ),
                KiBitmap( help_xpm ) );

    // About Eeschema
    helpMenu->AppendSeparator();
    AddMenuItem( helpMenu,
                wxID_ABOUT,
                _( "&About Eeschema" ),
                _( "About Eeschema schematic designer" ),
                KiBitmap( info_xpm ) );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( preferencesMenu, _( "&Preferences" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
