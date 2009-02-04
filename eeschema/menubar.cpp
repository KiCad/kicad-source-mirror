/*************************************************************/
/* menubar.cpp  - create the menubar for the schematic frame */
/*************************************************************/

#ifdef __GNUG__
#pragma implementation
#endif

#include "fctsys.h"
#include "common.h"
#include "appl_wxstruct.h"
#include "program.h"
#include "libcmp.h"
#include "general.h"
#include "bitmaps.h"
#include "protos.h"
#include "id.h"
#include "hotkeys.h"


/************************************************/
void WinEDA_SchematicFrame::ReCreateMenuBar()
/************************************************/

/* create or update the menubar for the schematic frame
 */
{
    wxString   msg;
    wxMenuItem *item;
    wxMenuBar  *menuBar = GetMenuBar();

    /* Destroy the existing menu bar so it can be rebuilt.  This allows
     * language changes of the menu text on the fly. */
    if( menuBar )
        SetMenuBar( NULL );

    menuBar = new wxMenuBar();

    wxMenu* filesMenu = new wxMenu;

    // Menu File:
    item = new wxMenuItem( filesMenu, ID_NEW_PROJECT, _( "&New" ),
                           _( "New schematic project" ) );
    item->SetBitmap( new_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_LOAD_PROJECT, _( "&Open" ),
                           _( "Open an existing schematic project" ) );
    item->SetBitmap( open_xpm );
    filesMenu->Append( item );

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_SAVE_PROJECT, _( "&Save Project" ),
                           _( "Save all sheets in the schematic project" ) );
    item->SetBitmap( save_project_xpm );
    filesMenu->Append( item );

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET, _( "&Save" ),
                           _( "Save only current schematic sheet" ) );
    item->SetBitmap( save_xpm );
    filesMenu->Append( item );

    item = new wxMenuItem( filesMenu, ID_SAVE_ONE_SHEET_AS, _( "Save &as.." ),
                           _( "Save current schematic sheet as.." ) );
    item->SetBitmap( save_as_xpm );
    filesMenu->Append( item );

    // Print and Plot section:
    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_GEN_PRINT, _( "P&rint" ),
                           _( "Print schematic sheet" ) );
    item->SetBitmap( print_button );
    filesMenu->Append( item );

    /* Plot Submenu */
    wxMenu* choice_plot_fmt = new wxMenu;
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_PS,
                           _( "Plot PostScript" ),
                           _( "Plot schematic sheet in PostScript format" ) );
    item->SetBitmap( plot_PS_xpm );
    choice_plot_fmt->Append( item );

    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_HPGL, _( "Plot HPGL" ),
                           _( "Plot schematic sheet in HPGL format" ) );
    item->SetBitmap( plot_HPG_xpm );
    choice_plot_fmt->Append( item );

    item = new wxMenuItem( choice_plot_fmt, ID_GEN_PLOT_SVG, _( "Plot SVG" ),
                           _( "Plot schematic sheet in SVG format" ) );
    item->SetBitmap( plot_xpm );
    choice_plot_fmt->Append( item );

#ifdef __WINDOWS__
    /* Under windows, one can draw to the clipboard */
    item = new wxMenuItem( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                           _( "Plot to Clipboard" ),
                           _( "Export drawings to clipboard" ) );
    item->SetBitmap( copy_button );
    choice_plot_fmt->Append( item );
#endif

    ADD_MENUITEM_WITH_HELP_AND_SUBMENU( filesMenu, choice_plot_fmt,
                                        ID_GEN_PLOT, _( "&Plot" ),
                                        _( "Plot schematic sheet in HPGL, PostScript or SVG format" ), plot_xpm );

    filesMenu->AppendSeparator();
    item = new wxMenuItem( filesMenu, ID_EXIT, _( "E&xit" ),
                           _( "Quit Eeschema" ) );
    item->SetBitmap( exit_xpm );
    filesMenu->Append( item );

    /* Add the file history */
    wxGetApp().m_fileHistory.AddFilesToMenu( filesMenu );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;
    msg  = AddHotkeyName( _( "&Undo\t" ), s_Schematic_Hokeys_Descr, HK_UNDO );
    item = new wxMenuItem( editMenu, ID_SCHEMATIC_UNDO, msg,
                           _( "Undo last edition" ), wxITEM_NORMAL );
    item->SetBitmap( undo_xpm );
    editMenu->Append( item );

    msg  = AddHotkeyName( _( "&Redo\t" ), s_Schematic_Hokeys_Descr, HK_REDO );
    item = new wxMenuItem( editMenu, ID_SCHEMATIC_REDO, msg,
                           _( "Redo the last undo command" ), wxITEM_NORMAL );
    item->SetBitmap( redo_xpm );
    editMenu->Append( item );

    editMenu->AppendSeparator();

    item = new wxMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                           _( "Delete" ), _( "Delete items" ), wxITEM_NORMAL );
    item->SetBitmap( delete_body_xpm );
    editMenu->Append( item );

    editMenu->AppendSeparator();

    item = new wxMenuItem( editMenu, ID_FIND_ITEMS, _( "Find" ),
                           _( "Find components and texts" ), wxITEM_NORMAL );
    item->SetBitmap( find_xpm );
    editMenu->Append( item );

    editMenu->AppendSeparator();

    item = new wxMenuItem( editMenu, ID_BACKANNO_ITEMS, _( "Backannotate" ),
                           _( "Back annotated footprint fields" ),
                           wxITEM_NORMAL );
    item->SetBitmap( backanno_xpm );
    editMenu->Append( item );

    // Menu View:
    wxMenu* viewMenu = new wxMenu;
    msg  = AddHotkeyName( _( "Zoom in" ), s_Schematic_Hokeys_Descr, HK_ZOOM_IN);
    item = new wxMenuItem( viewMenu, ID_ZOOM_IN, msg, _( "Zoom in" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_in_xpm );
    viewMenu->Append( item );

    msg = AddHotkeyName( _( "Zoom out" ), s_Schematic_Hokeys_Descr,
                         HK_ZOOM_OUT );
    item = new wxMenuItem( viewMenu, ID_ZOOM_OUT, msg, _( "Zoom out" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_out_xpm );
    viewMenu->Append( item );

    item = new wxMenuItem( viewMenu, ID_ZOOM_PAGE, _( "Zoom auto" ),
                           _( "Zoom auto" ), wxITEM_NORMAL );
    item->SetBitmap( zoom_auto_xpm );
    viewMenu->Append( item );

    viewMenu->AppendSeparator();

    msg = AddHotkeyName( _( "Redraw view" ), s_Schematic_Hokeys_Descr,
                         HK_ZOOM_REDRAW );
    item = new wxMenuItem( viewMenu, ID_ZOOM_REDRAW, msg, _( "Zoom auto" ),
                           wxITEM_NORMAL );
    item->SetBitmap( zoom_redraw_xpm );
    viewMenu->Append( item );

    // Place Menu
    //TODO: Unify the ID names!
    wxMenu* placeMenu = new wxMenu;

    item = new wxMenuItem( placeMenu, ID_COMPONENT_BUTT, _( "&Component" ),
                           _( "Place the component" ), wxITEM_NORMAL );
    item->SetBitmap( add_component_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_PLACE_POWER_BUTT, _( "&Power port" ),
                           _( "Place the power port" ), wxITEM_NORMAL );
    item->SetBitmap( add_power_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_WIRE_BUTT, _( "&Wire" ),
                           _( "Place the wire" ), wxITEM_NORMAL );
    item->SetBitmap( add_line_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_BUS_BUTT,  _( "&Bus" ),
                           _( "Place bus" ), wxITEM_NORMAL );
    item->SetBitmap( add_bus_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT,
                           _( "W&ire to bus entry" ),
                           _( "Place a wire to bus entry" ), wxITEM_NORMAL );
    item->SetBitmap( add_line2bus_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT,
                           _( "B&us to bus entry" ),
                           _( "Place a bus to bus entry" ), wxITEM_NORMAL );
    item->SetBitmap( add_bus2bus_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_NOCONN_BUTT,  _( "No connect flag" ),
                           _( "Place a no connect flag" ), wxITEM_NORMAL );
    item->SetBitmap( noconn_button );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_LABEL_BUTT, _( "Net name" ),
                           _( "Place net name" ), wxITEM_NORMAL  );
    item->SetBitmap( add_line_label_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_GLABEL_BUTT, _( "Global label" ),
                           _( "Place a global label. Warning: all global labels with the same name are connected in whole hierarchy" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_glabel_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_JUNCTION_BUTT, _( "Place Junction" ),
            _( "Place junction" ), wxITEM_NORMAL );
    item->SetBitmap( add_junction_xpm );
    placeMenu->Append( item );

    placeMenu->AppendSeparator();

    item = new wxMenuItem( placeMenu, ID_HIERLABEL_BUTT,
                           _( "Hierarchical label" ),
                           _( "Place a hierarchical label. This label will be seen as a pin sheet in the sheet symbol" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_label_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT,
                           _( "Hierarchical sheet" ),
                           _( "Create a hierarchical sheet" ), wxITEM_NORMAL );
    item->SetBitmap( add_hierarchical_subsheet_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_IMPORT_GLABEL_BUTT,
                           _( "Import Hierarchical Label" ),
                           _( "Place a pin sheet created by importing a hierarchical label from sheet" ),
                           wxITEM_NORMAL );
    item->SetBitmap( import_hierarchical_label_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_SHEET_LABEL_BUTT,
                           _( "Add Hierarchical Pin to Sheet" ),
                           _( "Place a hierarchical pin to sheet" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_hierar_pin_xpm );
    placeMenu->Append( item );

    placeMenu->AppendSeparator();

    item = new wxMenuItem( placeMenu, ID_LINE_COMMENT_BUTT,
                           _( "Graphic line or polygon" ),
                           _( "Place graphic lines or polygons" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_dashed_line_xpm );
    placeMenu->Append( item );

    item = new wxMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT,
                           _( "Graphic text (comment)" ),
                           _( "Place graphic text (comment)" ),
                           wxITEM_NORMAL );
    item->SetBitmap( add_text_xpm );
    placeMenu->Append( item );

    // Menu Configuration:
    wxMenu* configmenu = new wxMenu;
    item = new wxMenuItem( configmenu, ID_CONFIG_REQ, _( "&Library" ),
                           _( "Library preferences" ) );
    item->SetBitmap( library_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_COLORS_SETUP, _( "&Colors" ),
                           _( "Color preferences" ) );
    item->SetBitmap( palette_xpm );
    configmenu->Append( item );

    // Options
    item = new wxMenuItem( configmenu, ID_OPTIONS_SETUP, _( "&Options" ),
                           _( "General options..." ) );
    item->SetBitmap( preference_xpm );
    configmenu->Append( item );

    // Font selection and setup
    AddFontSelectionMenu( configmenu );

    wxGetApp().AddMenuLanguageList( configmenu );

    configmenu->AppendSeparator();
    item = new wxMenuItem( configmenu, ID_CONFIG_SAVE, _( "&Save preferences" ),
                           _( "Save application preferences" ) );
    item->SetBitmap( save_setup_xpm );
    configmenu->Append( item );

    item = new wxMenuItem( configmenu, ID_CONFIG_READ, _( "&Read preferences" ),
                           _( "Read application preferences" ) );
    item->SetBitmap( read_setup_xpm );
    configmenu->Append( item );

    configmenu->AppendSeparator();
    AddHotkeyConfigMenu( configmenu );

    // Menu Help:
    wxMenu* helpMenu = new wxMenu;
    item = new wxMenuItem( helpMenu, ID_GENERAL_HELP, _( "&Contents" ),
                           _( "Open the eeschema manual" ) );
    item->SetBitmap( help_xpm );
    helpMenu->Append( item );

    item = new wxMenuItem( helpMenu, ID_KICAD_ABOUT, _( "&About" ),
                           _( "About eeschema schematic designer" ) );
    item->SetBitmap( info_xpm );
    helpMenu->Append( item );


    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( configmenu, _( "&Preferences" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    // Associate the menu bar with the frame
    SetMenuBar( menuBar );
}
