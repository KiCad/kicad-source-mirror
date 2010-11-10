/**
 * @file menubar.cpp
 * @brief Create the main menubar for the schematic frame
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

#include "help_common_strings.h"

/**
 * @brief Create or update the menubar for the schematic frame
 */
void WinEDA_SchematicFrame::ReCreateMenuBar()
{
    wxString    text;
    wxMenuItem* item;
    wxMenuBar*  menuBar = GetMenuBar();

    /**
     * Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly.
     */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    /**
     * File menu
     */
    wxMenu* filesMenu = new wxMenu;

    /* New */
    item = new wxMenuItem( filesMenu, ID_NEW_PROJECT, _( "&New\tCtrl+N" ),
                           _( "New schematic project" ) );

    item->SetBitmap( new_xpm );
    filesMenu->Append( item );

    /* Open */
    item = new wxMenuItem( filesMenu, ID_LOAD_PROJECT, _( "&Open\tCtrl+O" ),
                           _( "Open an existing schematic project" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    /* Open Recent submenu */
    wxMenu* openRecentMenu = new wxMenu();
    wxGetApp().m_fileHistory.AddFilesToMenu( openRecentMenu );
    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, openRecentMenu,
                                        -1, _( "Open &Recent" ),
                                        _( "Open a recent opened schematic project" ),
                                        open_project_xpm );

    /* Separator */
    filesMenu->AppendSeparator();

    /* Save */
    /* Save Project */
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT,
                           _( "&Save Whole Schematic Project\tCtrl+S" ),
                           _( "Save all sheets in the schematic project" ) );
    item->SetBitmap( save_project_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET, _( "Save &Current Sheet Only" ),
                           _( "Save only current schematic sheet" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    /* Save as... */
    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET_AS,
                           _( "Save Current Sheet &as" ),
                           _( "Save current schematic sheet as..." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );

    /* Separator */
    filesMenu->AppendSeparator();

    /* Print */
    item = new wxMenuItem( filesMenu, wxID_PRINT, _( "P&rint" ), _( "Print schematic" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    /* Plot submenu */
    wxMenu* choice_plot_fmt = new wxMenu;
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_PS,
                           _( "Plot PostScript" ),
                           _( "Plot schematic sheet in PostScript format" ) );
    item->SetBitmap( plot_PS_xpm );
    choice_plot_fmt->Append( item );

    /* Plot HPGL */
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_HPGL, _( "Plot HPGL" ),
                           _( "Plot schematic sheet in HPGL format" ) );
    item->SetBitmap( plot_HPG_xpm );
    choice_plot_fmt->Append( item );

    /* Plot SVG */
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_SVG, _( "Plot SVG" ),
                           _( "Plot schematic sheet in SVG format" ) );
    item->SetBitmap( plot_xpm );
    choice_plot_fmt->Append( item );

    /* Plot DXF */
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_DXF, _( "Plot DXF" ),
                           _( "Plot schematic sheet in DXF format" ) );
    item->SetBitmap( plot_xpm );
    choice_plot_fmt->Append( item );

    /* Under windows, one can draw to the clipboard */
#ifdef __WINDOWS__

    item = new wxMenuItem( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                           _( "Plot to Clipboard" ),
                           _( "Export drawings to clipboard" ) );
    item->SetBitmap( copy_button );
    choice_plot_fmt->Append( item );

#endif

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, choice_plot_fmt,
                                        ID_GEN_PLOT, _( "&Plot" ),
                                        _( "Plot schematic sheet in HPGL, PostScript or SVG format" ),
                                        plot_xpm );

    /* Quit on all platforms except WXMAC */
#if !defined(__WXMAC__)

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ), _( "Quit EESchema" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

#endif /* !defined( __WXMAC__) */


    /**
     * Edit menu
     */
    wxMenu* editMenu = new wxMenu;

    /* Undo */
    text = AddHotkeyName( _( "Undo" ), s_Schematic_Hokeys_Descr, HK_UNDO );

    item = new wxMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, wxITEM_NORMAL );
    item->SetBitmap( undo_xpm );
    editMenu->Append( item );

    /* Redo */
    text = AddHotkeyName( _( "Redo" ), s_Schematic_Hokeys_Descr, HK_REDO );

    item = new wxMenuItem( editMenu, wxID_REDO, text, HELP_REDO, wxITEM_NORMAL );
    item->SetBitmap( redo_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Delete */
    item = new wxMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                           _( "Delete" ), HELP_DELETE_ITEMS, wxITEM_NORMAL );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Find */
    text = AddHotkeyName( _( "&Find" ), s_Schematic_Hokeys_Descr, HK_FIND_ITEM );
    item = new wxMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND, wxITEM_NORMAL );
    item->SetBitmap( find_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Backannotate */
    item = new wxMenuItem( editMenu, ID_BACKANNO_ITEMS, _( "Backannotate" ),
                           _( "Back annotated footprint fields" ),
                           wxITEM_NORMAL );
    item->SetBitmap( backanno_xpm );
    editMenu->Append( item );


    /**
     * View menu
     */
    wxMenu* viewMenu = new wxMenu;

    /* Important Note for ZOOM IN and ZOOM OUT commands from menubar:
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
    /* Zoom in */
    text = AddHotkeyName( _( "Zoom In" ), s_Schematic_Hokeys_Descr,
                          ID_ZOOM_IN, false );  // add comment, not a shortcut
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, wxITEM_NORMAL );
    item->SetBitmap( zoom_in_xpm );
    viewMenu->Append( item );

    /* Zoom out */
    text = AddHotkeyName( _( "Zoom Out" ), s_Schematic_Hokeys_Descr,
                          ID_ZOOM_OUT, false );  // add comment, not a shortcut
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, wxITEM_NORMAL );
    item->SetBitmap( zoom_out_xpm );
    viewMenu->Append( item );

    /* Fit on screen */
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );
    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, wxITEM_NORMAL );
    item->SetBitmap( zoom_auto_xpm );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    /* Redraw view */
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );
    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, wxITEM_NORMAL );
    item->SetBitmap( zoom_redraw_xpm );
    viewMenu->Append( item );


    /**
     * Place menu
     * TODO: Unify the ID names!
     */
    wxMenu* placeMenu = new wxMenu;

    /* Component */
    text = AddHotkeyName( _( "&Component" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_COMPONENT, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_COMPONENT_BUTT, text,
                           HELP_PLACE_COMPONENTS, wxITEM_NORMAL );
    item->SetBitmap( add_component_xpm );
    placeMenu->Append( item );

    /* Power port */
    item = new wxMenuItem( placeMenu, ID_PLACE_POWER_BUTT, _( "&Power port" ),
                           HELP_PLACE_POWERPORT, wxITEM_NORMAL );
    item->SetBitmap( add_power_xpm );
    placeMenu->Append( item );

    /* Wire */
    text = AddHotkeyName( _( "&Wire" ), s_Schematic_Hokeys_Descr,
                          HK_BEGIN_WIRE, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_WIRE_BUTT, text,
                           HELP_PLACE_WIRE, wxITEM_NORMAL );
    item->SetBitmap( add_line_xpm );
    placeMenu->Append( item );

    /* Bus */
    item = new wxMenuItem( placeMenu, ID_BUS_BUTT, _( "&Bus" ),
                           HELP_PLACE_BUS, wxITEM_NORMAL );
    item->SetBitmap( add_bus_xpm );
    placeMenu->Append( item );

    /* Wire to Bus */
    item = new wxMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT, _( "W&ire to bus entry" ),
                           HELP_PLACE_WIRE2BUS_ENTRY, wxITEM_NORMAL );
    item->SetBitmap( add_line2bus_xpm );
    placeMenu->Append( item );

    /* Bus to Bus */
    item = new wxMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT, _( "B&us to bus entry" ),
                           HELP_PLACE_BUS2BUS_ENTRY, wxITEM_NORMAL );
    item->SetBitmap( add_bus2bus_xpm );
    placeMenu->Append( item );

    /* No connect flag */
    text = AddHotkeyName( _( "No connect flag" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_NOCONN_FLAG, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG, wxITEM_NORMAL );
    item->SetBitmap( noconn_button );
    placeMenu->Append( item );

    /* Net name */
    text = AddHotkeyName( _( "Label" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_LABEL, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_LABEL_BUTT, text,
                           HELP_PLACE_NETLABEL, wxITEM_NORMAL  );
    item->SetBitmap( add_line_label_xpm );
    placeMenu->Append( item );

    /* Global label */
    item = new wxMenuItem( placeMenu, ID_GLABEL_BUTT, _( "Global label" ),
                           HELP_PLACE_GLOBALLABEL, wxITEM_NORMAL );
    item->SetBitmap( add_glabel_xpm );
    placeMenu->Append( item );

    /* Junction */
    text = AddHotkeyName( _( "Junction" ), s_Schematic_Hokeys_Descr,
                          HK_ADD_JUNCTION, false );    // add comment, not a shortcut
    item = new wxMenuItem( placeMenu, ID_JUNCTION_BUTT, text,
                           HELP_PLACE_JUNCTION, wxITEM_NORMAL );
    item->SetBitmap( add_junction_xpm );
    placeMenu->Append( item );

    /* Separator */
    placeMenu->AppendSeparator();

    /* Hierarchical label */
    item = new wxMenuItem( placeMenu, ID_HIERLABEL_BUTT, _( "Hierarchical label" ),
                           HELP_PLACE_HIER_LABEL, wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_label_xpm );
    placeMenu->Append( item );

    /* Hierarchical sheet */
    item = new wxMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT, _( "Hierarchical sheet" ),
                           HELP_PLACE_SHEET, wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_subsheet_xpm );
    placeMenu->Append( item );

    /* Import hierarchical sheet */
    item = new wxMenuItem( placeMenu, ID_IMPORT_HLABEL_BUTT, _( "Import Hierarchical Label" ),
                           HELP_IMPORT_PINSHEET, wxITEM_NORMAL );
    item->SetBitmap( import_hierarchical_label_xpm );
    placeMenu->Append( item );

    /* Add hierarchical Pin to Sheet */
    item = new wxMenuItem( placeMenu, ID_SHEET_LABEL_BUTT, _( "Add Hierarchical Pin to Sheet" ),
                           HELP_PLACE_PINSHEET, wxITEM_NORMAL );
    item->SetBitmap( add_hierar_pin_xpm );
    placeMenu->Append( item );

    /* Separator */
    placeMenu->AppendSeparator();

    /* Graphic line or polygon */
    item = new wxMenuItem( placeMenu, ID_LINE_COMMENT_BUTT, _( "Graphic line or polygon" ),
                           HELP_PLACE_GRAPHICLINES, wxITEM_NORMAL );
    item->SetBitmap( add_dashed_line_xpm );
    placeMenu->Append( item );

    /* Graphic text */
    item = new wxMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT, _( "Graphic text" ),
                           HELP_PLACE_GRAPHICTEXTS, wxITEM_NORMAL );
    item->SetBitmap( add_text_xpm );
    placeMenu->Append( item );


    /**
     * Preferences Menu
     */
    wxMenu* configmenu = new wxMenu;

    /* Library */
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Library" ),
                           _( "Library preferences" ) );
    item->SetBitmap( library_xpm );
    configmenu->Append( item );

    /* Colors */
    item = new wxMenuItem( configmenu, ID_COLORS_SETUP, _( "&Colors" ),
                           _( "Color preferences" ) );
    item->SetBitmap( palette_xpm );
    configmenu->Append( item );

    /* Options */
    item = new wxMenuItem( configmenu, ID_OPTIONS_SETUP, _( "&Options" ),
                           _( "Eeschema general options and preferences" ) );
    item->SetBitmap( preference_xpm );
    configmenu->Append( item );

    /* Language submenu */
    wxGetApp().AddMenuLanguageList( configmenu );

    /* Hotkey submenu */
    AddHotkeyConfigMenu( configmenu );

    /* Separator */
    configmenu->AppendSeparator();

    /* Save preferences */
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE, _( "&Save preferences" ),
                           _( "Save application preferences" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    /* Read preferences */
    item = new wxMenuItem( configmenu, ID_CONFIG_READ, _( "&Read preferences" ),
                           _( "Read application preferences" ) );
    item->SetBitmap( read_setup_xpm );
    configmenu->Append( item );


    /**
     * Help Menu
     */
    wxMenu* helpMenu = new wxMenu;

    AddHelpVersionInfoMenuEntry( helpMenu );

    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    item->SetBitmap( online_help_xpm );
    helpMenu->Append( item );

    /* About on all platforms except WXMAC */
#if !defined(__WXMAC__)

    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT, _( "&About" ),
                           _( "About eeschema schematic designer" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );

#endif /* !defined(__WXMAC__) */


    /**
     * Create the menubar and append all submenus
     */
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    /* Associate the menu bar with the frame */
    SetMenuBar( menuBar );
}
