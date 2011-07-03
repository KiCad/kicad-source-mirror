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
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_NEW_PROJECT,
                            _( "&New\tCtrl+N" ),
                            _( "New schematic project" ),
                            new_xpm );

    // Open
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_LOAD_PROJECT,
                            _( "&Open\tCtrl+O" ),
                            _( "Open an existing schematic project" ),
                            open_document_xpm );

    // Open Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        wxGetApp().m_fileHistory.RemoveMenu( openRecentMenu );
    openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.UseMenu( openRecentMenu );
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, openRecentMenu,
                                        wxID_ANY, _( "Open &Recent" ),
                                        _( "Open a recent opened schematic project" ),
                                        open_project_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Save schematic project
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_SAVE_PROJECT,
                            _( "&Save Whole Schematic Project\tCtrl+S" ),
                            _( "Save all sheets in the schematic project" ),
                            save_project_xpm );

    // Save current sheet
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_SAVE_ONE_SHEET,
                            _( "Save &Current Sheet Only" ),
                            _( "Save only current schematic sheet" ),
                            save_xpm );

    // Save current sheet as
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_SAVE_ONE_SHEET_AS,
                            _( "Save Current Sheet &as" ),
                            _( "Save current schematic sheet as..." ),
                            save_as_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Page settings
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            ID_SHEET_SET,
                            _( "P&age Settings" ),
                            _( "Settigns for page size and information" ),
                            sheetset_xpm );

    // Print
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_PRINT,
                            _( "P&rint" ),
                            _( "Print schematic" ),
                            print_button );

    // Plot submenu
    wxMenu* choice_plot_fmt = new wxMenu;

    // Plot PostScript
    ADD_MENUITEM_WITH_HELP( choice_plot_fmt, ID_GEN_PLOT_PS,
                            _( "Plot PostScript" ),
                            _( "Plot schematic sheet in PostScript format" ),
                            plot_PS_xpm );

    // Plot HPGL
    ADD_MENUITEM_WITH_HELP( choice_plot_fmt,
                            ID_GEN_PLOT_HPGL,
                            _( "Plot HPGL" ),
                            _( "Plot schematic sheet in HPGL format" ),
                            plot_HPG_xpm );

    // Plot SVG
    ADD_MENUITEM_WITH_HELP( choice_plot_fmt,
                            ID_GEN_PLOT_SVG,
                            _( "Plot SVG" ),
                            _( "Plot schematic sheet in SVG format" ),
                            plot_xpm );

    // Plot DXF
    ADD_MENUITEM_WITH_HELP( choice_plot_fmt,
                            ID_GEN_PLOT_DXF,
                            _( "Plot DXF" ),
                            _( "Plot schematic sheet in DXF format" ),
                            plot_xpm );

    // Plot to Clipboard (Windows only)
#ifdef __WINDOWS__

    ADD_MENUITEM_WITH_HELP( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                            _( "Plot to Clipboard" ),
                            _( "Export drawings to clipboard" ),
                            copy_button );

#endif // __WINDOWS__

    // Plot submenu
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( fileMenu, choice_plot_fmt,
                                        ID_GEN_PLOT, _( "&Plot" ),
                                        _(
                                            "Plot schematic sheet in HPGL, PostScript or SVG format" ),
                                        plot_xpm );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    ADD_MENUITEM_WITH_HELP( fileMenu,
                            wxID_EXIT,
                            _( "&Quit" ),
                            _( "Quit EESchema" ),
                            exit_xpm );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "Undo" ), s_Schematic_Hokeys_Descr, HK_UNDO );

    ADD_MENUITEM_WITH_HELP( editMenu, wxID_UNDO, text, HELP_UNDO,
                            undo_xpm );

    // Redo
    text = AddHotkeyName( _( "Redo" ), s_Schematic_Hokeys_Descr, HK_REDO );

    ADD_MENUITEM_WITH_HELP( editMenu, wxID_REDO, text, HELP_REDO,
                            redo_xpm );

    // Delete
    editMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                            _( "Delete" ), HELP_DELETE_ITEMS,
                            delete_body_xpm );

    // Find
    editMenu->AppendSeparator();
    text = AddHotkeyName( _( "&Find" ), s_Schematic_Hokeys_Descr, HK_FIND_ITEM );
    ADD_MENUITEM_WITH_HELP( editMenu, ID_FIND_ITEMS, text, HELP_FIND,
                            find_xpm );

    // Backannotate
    editMenu->AppendSeparator();
    ADD_MENUITEM_WITH_HELP( editMenu,
                            ID_BACKANNO_ITEMS,
                            _( "&Backannotate" ),
                            _( "Back annotate the footprint fields" ),
                            import_footprint_names_xpm );




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
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN,
                            zoom_in_xpm );

    // Zoom out
    text = AddHotkeyName( _( "Zoom Out" ), s_Schematic_Hokeys_Descr,
                          ID_ZOOM_OUT, false );  // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT,
                            zoom_out_xpm );

    // Fit on screen
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );

    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT,
                            zoom_fit_in_page_xpm );

    // Separator
    viewMenu->AppendSeparator();

    // Hierarchy
    ADD_MENUITEM_WITH_HELP( viewMenu,
                            ID_HIERARCHY,
                            _( "H&ierarchy" ),
                            _( "Navigate schematic hierarchy" ),
                            hierarchy_nav_xpm );

    // Redraw
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    ADD_MENUITEM_WITH_HELP( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW,
                            zoom_redraw_xpm );

    // Menu place:
    // @todo unify IDs
    wxMenu* placeMenu = new wxMenu;

    // Component
    text = AddHotkeyName( _( "Component" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_COMPONENT, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_SCH_PLACE_COMPONENT, text,
                            HELP_PLACE_COMPONENTS,
                            add_component_xpm );

    // Power port
    text = AddHotkeyName( _( "Power port" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_POWER, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_PLACE_POWER_BUTT, text,
                            HELP_PLACE_POWERPORT,
                            add_power_xpm );

    // Wire
    text = AddHotkeyName( _( "Wire" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_WIRE, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_WIRE_BUTT, text,
                            HELP_PLACE_WIRE,
                            add_line_xpm );

    // Bus
    text = AddHotkeyName( _( "Bus" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_BUS, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_BUS_BUTT, text,
                            HELP_PLACE_BUS,
                            add_bus_xpm );

    // Wire to Bus entry
    text = AddHotkeyName( _( "Wire to bus entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_WIRE_ENTRY, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_WIRETOBUS_ENTRY_BUTT, text,
                            HELP_PLACE_WIRE2BUS_ENTRY,
                            add_line2bus_xpm );

    // Bus to Bus entry
    text = AddHotkeyName( _( "Bus to bus entry" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_BUS_ENTRY, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_BUSTOBUS_ENTRY_BUTT, text,
                            HELP_PLACE_BUS2BUS_ENTRY,
                            add_bus2bus_xpm );

    // No connect flag
    text = AddHotkeyName( _( "No connect flag" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NOCONN_FLAG, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG,
                            noconn_button );

    // Net name
    text = AddHotkeyName( _( "Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_LABEL, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_LABEL_BUTT, text,
                            HELP_PLACE_NETLABEL,
                            add_line_label_xpm );

    // Global label
    text = AddHotkeyName( _( "Global label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GLABEL, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_GLABEL_BUTT, text,
                            HELP_PLACE_GLOBALLABEL,
                            add_glabel_xpm );

    // Junction
    text = AddHotkeyName( _( "Junction" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_JUNCTION, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_JUNCTION_BUTT, text,
                            HELP_PLACE_JUNCTION,
                            add_junction_xpm );

    // Separator
    placeMenu->AppendSeparator();

    // Hierarchical label
    text = AddHotkeyName( _( "Hierarchical label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, false );       // add comment, not a shortcut
    text = AddHotkeyName( _( "Hierarchical label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, false );       // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_HIERLABEL_BUTT,
                            text, HELP_PLACE_HIER_LABEL,
                            add_hierarchical_label_xpm );


    // Hierarchical sheet
    text = AddHotkeyName( _( "Hierarchical sheet" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_HIER_SHEET, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_SHEET_SYMBOL_BUTT, text,
                            HELP_PLACE_SHEET,
                            add_hierarchical_subsheet_xpm );

    // Import hierarchical sheet
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_IMPORT_HLABEL_BUTT,
                            _( "Import Hierarchical Label" ),
                            HELP_IMPORT_SHEETPIN,
                            import_hierarchical_label_xpm );

    // Add hierarchical Pin to Sheet
    ADD_MENUITEM_WITH_HELP( placeMenu,
                            ID_SHEET_PIN_BUTT,
                            _( "Add Hierarchical Pin to Sheet" ),
                            HELP_PLACE_SHEETPIN,
                            add_hierar_pin_xpm );

    // Separator
    placeMenu->AppendSeparator();

    // Graphic line or polygon
    text = AddHotkeyName( _( "Graphic polyline" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_POLYLINE, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_LINE_COMMENT_BUTT, text,
                            HELP_PLACE_GRAPHICLINES,
                            add_dashed_line_xpm );

    // Graphic text
    text = AddHotkeyName( _( "Graphic text" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_TEXT, false );    // add comment, not a shortcut
    ADD_MENUITEM_WITH_HELP( placeMenu, ID_TEXT_COMMENT_BUTT, text,
                            HELP_PLACE_GRAPHICTEXTS,
                            add_text_xpm );


    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_REQ,
                            _( "&Library" ),
                            _( "Library preferences" ),
                            library_xpm );

    // Colors
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_COLORS_SETUP,
                            _( "&Colors" ),
                            _( "Color preferences" ),
                            palette_xpm );

    // Options (Preferences on WXMAC)
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            wxID_PREFERENCES,
#ifdef __WXMAC__
                            _( "&Preferences..." ),
#else
                            _( "&Options" ),
#endif // __WXMAC__
                            _( "EESchema preferences" ),
                            preference_xpm );

    // Language submenu
    wxGetApp().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    // Save preferences
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_SAVE,
                            _( "&Save preferences" ),
                            _( "Save application preferences" ),
                            save_setup_xpm );

    // Read preferences
    ADD_MENUITEM_WITH_HELP( preferencesMenu,
                            ID_CONFIG_READ,
                            _( "&Read preferences" ),
                            _( "Read application preferences" ),
                            read_setup_xpm );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;

    // Library viewer
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_TO_LIBRARY,
                            _( "Library &Browser" ),
                            _( "Library browser" ),
                            library_browse_xpm );


    // Library editor
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_TO_LIBRARY,
                            _( "Library &Editor" ),
                            _( "Library editor" ),
                            libedit_xpm );

    // Separator
    toolsMenu->AppendSeparator();

    // Annotate
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_GET_ANNOTATE,
                            _( "&Annotate" ),
                            _( "Annotate the components in the schematic" ),
                            annotate_xpm );

    // ERC
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_GET_ERC,
                            _( "ER&C" ),
                            _( "Perform electrical rule check" ),
                            erc_xpm );

    // Generate netlist
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_GET_NETLIST,
                            _( "Generate &Netlist" ),
                            _( "Generate the component netlist" ),
                            netlist_xpm );

    // Generate bill of materials
    ADD_MENUITEM_WITH_HELP( toolsMenu,
                            ID_GET_TOOLS,
                            _( "Generate Bill of Materials" ),
                            _( "Generate bill of materials" ),
                            tools_xpm );

    // Help Menu:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    // Contents
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_HELP,
                            _( "&Contents" ),
                            _( "Open the Eeschema handbook" ),
                            online_help_xpm );

    // About EESchema
    ADD_MENUITEM_WITH_HELP( helpMenu,
                            wxID_ABOUT,
                            _( "&About EESchema" ),
                            _( "About EESchema schematic designer" ),
                            info_xpm );

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
