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

#include "program.h"
#include "general.h"
#include "protos.h"
#include "eeschema_id.h"
#include "hotkeys.h"

/**
 * @brief Create or update the menubar for the schematic frame
 */
void WinEDA_SchematicFrame::ReCreateMenuBar()
{
    wxString   text;
    wxMenuItem *item;
    wxMenuBar  *menuBar = GetMenuBar();

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
                     _("Open a recent opened schematic project" ),
                                               open_project_xpm );

    /* Separator */
    filesMenu->AppendSeparator();

    /* Save */
    /* Save Project */
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT, _( "&Save Whole Schematic Project\tCtrl+S" ),
                           _( "Save all sheets in the schematic project" ) );
    item->SetBitmap( save_project_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET, _( "&Save Current Sheet Only" ),
                           _( "Save only current schematic sheet" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    /* Save as... */
    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET_AS,
                           _( "Save Current Sheet &as\tShift+Ctrl+S" ),
                           _( "Save current schematic sheet as..." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );

    /* Separator */
    filesMenu->AppendSeparator();

    /* Print */
    item = new wxMenuItem( filesMenu, ID_GEN_PRINT, _( "P&rint\tCtrl+P" ),
                           _( "Print schematic sheet" ) );
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
                                        _( "Plot schematic sheet in HPGL, PostScript or SVG format" ), plot_xpm );

    /* Quit on all platforms except WXMAC */
#if !defined(__WXMAC__)

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, wxID_EXIT, _( "&Quit" ),
                           _( "Quit EESchema" ) );
    filesMenu->Append( item );

#endif /* !defined( __WXMAC__) */



    /**
     * Edit menu
     */
    wxMenu* editMenu = new wxMenu;

    /* Undo */
    text  = AddHotkeyName( _( "Undo" ), s_Schematic_Hokeys_Descr, HK_UNDO);

    item = new wxMenuItem( editMenu, wxID_UNDO, text,
                           _( "Undo last edition" ), wxITEM_NORMAL );
    item->SetBitmap( undo_xpm );
    editMenu->Append( item );

    /* Redo */
    text  = AddHotkeyName( _( "Redo" ), s_Schematic_Hokeys_Descr, HK_REDO);

    item = new wxMenuItem( editMenu, wxID_REDO, text,
                           _( "Redo the last undo command" ), wxITEM_NORMAL );
    item->SetBitmap( redo_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Delete */
    item = new wxMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                           _( "Delete" ), _( "Delete items" ), wxITEM_NORMAL );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );

    /* Separator */
    editMenu->AppendSeparator();

    /* Find */
    item = new wxMenuItem( editMenu, ID_FIND_ITEMS, _( "&Find\tCtrl+F" ),
                           _( "Find components and texts" ), wxITEM_NORMAL );
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

    /* Zoom in */
#if !defined( __WXMAC__)
    text  = AddHotkeyName( _( "Zoom In" ), s_Schematic_Hokeys_Descr, HK_ZOOM_IN);
#else
    text = _( "Zoom In\tCtrl++" );
#endif

    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, text, _( "Zoom In" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_in_xpm );
    viewMenu->Append( item );

    /* Zoom out */
#if !defined( __WXMAC__)
    text = AddHotkeyName( _( "Zoom Out" ), s_Schematic_Hokeys_Descr,
                         HK_ZOOM_OUT );
#else
    text = _( "Zoom Out\tCtrl+-" );
#endif /* !defined( __WXMAC__) */

    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, text, _( "Zoom Out" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_out_xpm );
    viewMenu->Append( item );

    /* Fit on screen */
#if !defined( __WXMAC__)
    text = AddHotkeyName( _( "Fit on Screen" ), s_Schematic_Hokeys_Descr,
                         HK_ZOOM_AUTO );
#else
    text = _( "Fit on Screen\tCtrl+0" );
#endif

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, text,
                           _( "Fit the schematic sheet on the screen" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_auto_xpm );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    /* Redraw view */
#if !defined( __WXMAC__)
    text = AddHotkeyName( _( "Redraw" ), s_Schematic_Hokeys_Descr,
                         HK_ZOOM_REDRAW );
#else
    text = _( "Redraw\tCtrl+R" );
#endif

    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, text,
                           _( "Redraw the schematic view" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_redraw_xpm );
    viewMenu->Append( item );



    /**
     * Place menu
     * TODO: Unify the ID names!
     */
    wxMenu* placeMenu = new wxMenu;

    /* Component */
    item = new wxMenuItem( placeMenu, ID_COMPONENT_BUTT, _( "&Component" ),
                           _( "Place the component" ), wxITEM_NORMAL );
    item->SetBitmap( add_component_xpm );
    placeMenu->Append( item );

    /* Power port */
    item = new wxMenuItem( placeMenu, ID_PLACE_POWER_BUTT, _( "&Power port" ),
                           _( "Place the power port" ), wxITEM_NORMAL );
    item->SetBitmap( add_power_xpm );
    placeMenu->Append( item );

    /* Wire */
    item = new wxMenuItem( placeMenu, ID_WIRE_BUTT, _( "&Wire" ),
                           _( "Place the wire" ), wxITEM_NORMAL );
    item->SetBitmap( add_line_xpm );
    placeMenu->Append( item );

    /* Bus */
    item = new wxMenuItem( placeMenu, ID_BUS_BUTT,  _( "&Bus" ),
                           _( "Place bus" ), wxITEM_NORMAL );
    item->SetBitmap( add_bus_xpm );
    placeMenu->Append( item );

    /* Wire to Bus */
    item = new wxMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT,
                           _( "W&ire to bus entry" ),
                           _( "Place a wire to bus entry" ), wxITEM_NORMAL );
    item->SetBitmap( add_line2bus_xpm );
    placeMenu->Append( item );

    /* Bus to Bus */
    item = new wxMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT,
                           _( "B&us to bus entry" ),
                           _( "Place a bus to bus entry" ), wxITEM_NORMAL );
    item->SetBitmap( add_bus2bus_xpm );
    placeMenu->Append( item );

    /* No connect flag */
    item = new wxMenuItem( placeMenu, ID_NOCONN_BUTT,  _( "No connect flag" ),
                           _( "Place a no connect flag" ), wxITEM_NORMAL );
    item->SetBitmap( noconn_button );
    placeMenu->Append( item );

    /* Net name */
    item = new wxMenuItem( placeMenu, ID_LABEL_BUTT, _( "Net name" ),
                           _( "Place net name" ), wxITEM_NORMAL  );
    item->SetBitmap( add_line_label_xpm );
    placeMenu->Append( item );

    /* Global label */
    item = new wxMenuItem( placeMenu, ID_GLABEL_BUTT, _( "Global label" ),
                           _( "Place a global label. Warning: all global labels with the same name are connected in whole hierarchy" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_glabel_xpm );
    placeMenu->Append( item );

    /* Junction */
    item = new wxMenuItem( placeMenu, ID_JUNCTION_BUTT, _( "Junction" ),
            _( "Place junction" ), wxITEM_NORMAL );
    item->SetBitmap( add_junction_xpm );
    placeMenu->Append( item );

    /* Separator */
    placeMenu->AppendSeparator();

    /* Hierarchical label */
    item = new wxMenuItem( placeMenu, ID_HIERLABEL_BUTT,
                           _( "Hierarchical label" ),
                           _( "Place a hierarchical label. This label will be seen as a pin sheet in the sheet symbol" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_label_xpm );
    placeMenu->Append( item );

    /* Hierarchical sheet */
    item = new wxMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT,
                           _( "Hierarchical sheet" ),
                           _( "Create a hierarchical sheet" ), wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_subsheet_xpm );
    placeMenu->Append( item );

    /* Import hierarchical sheet */
    item = new wxMenuItem( placeMenu, ID_IMPORT_HLABEL_BUTT,
                           _( "Import Hierarchical Label" ),
                           _( "Place a pin sheet created by importing a hierarchical label from sheet" ),
                           wxITEM_NORMAL );
    item->SetBitmap( import_hierarchical_label_xpm );
    placeMenu->Append( item );

    /* Add hierarchical Pin to Sheet */
    item = new wxMenuItem( placeMenu, ID_SHEET_LABEL_BUTT,
                           _( "Add Hierarchical Pin to Sheet" ),
                           _( "Place a hierarchical pin to sheet" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_hierar_pin_xpm );
    placeMenu->Append( item );

    /* Separator */
    placeMenu->AppendSeparator();

    /* Graphic line or polygon */
    item = new wxMenuItem( placeMenu, ID_LINE_COMMENT_BUTT,
                           _( "Graphic line or polygon" ),
                           _( "Place graphic lines or polygons" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_dashed_line_xpm );
    placeMenu->Append( item );

    /* Graphic text */
    item = new wxMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT,
                           _( "Graphic text" ),
                           _( "Place graphic text for comment" ),
                           wxITEM_NORMAL );
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
    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    item->SetBitmap( help_xpm );
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

