/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file eeschema/menubar.cpp
 * @brief (Re)Create the main menubar for the schematic frame
 */


#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>

#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "hotkeys.h"
#include "sch_edit_frame.h"

// helper functions that build specific submenus:

// Build the place submenu
static void preparePlaceMenu( wxMenu* aParentMenu );

// Build the files menu. Because some commands are available only if
// Eeschemat is run outside a project (run alone), aIsOutsideProject is false
// when Eeschema is run from Kicad manager, and true is run as stand alone app.
static void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject );

// Build the inspect menu
static void prepareInspectMenu( wxMenu* aParentMenu );

// Build the tools menu
static void prepareToolsMenu( wxMenu* aParentMenu );

// Build the help menu
static void prepareHelpMenu( wxMenu* aParentMenu );

// Build the edit menu
static void prepareEditMenu( wxMenu* aParentMenu );

// Build the view menu
static void prepareViewMenu( wxMenu* aParentMenu );

// Build the preferences menu
static void preparePreferencesMenu( SCH_EDIT_FRAME* aFrame, wxMenu* aParentMenu );


void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Menu File:
    wxMenu* fileMenu = new wxMenu;
    prepareFilesMenu( fileMenu, Kiface().IsSingle() );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;
    prepareEditMenu( editMenu );

    // Menu View:
    wxMenu* viewMenu = new wxMenu;
    prepareViewMenu( viewMenu );

    // Menu place:
    wxMenu* placeMenu = new wxMenu;
    preparePlaceMenu( placeMenu );

    // Menu Inspect:
    wxMenu* inspectMenu = new wxMenu;
    prepareInspectMenu( inspectMenu );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;
    prepareToolsMenu( toolsMenu );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;
    preparePreferencesMenu( this, preferencesMenu );

    // Help Menu:
    wxMenu* helpMenu = new wxMenu;
    prepareHelpMenu( helpMenu );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


void prepareViewMenu( wxMenu* aParentMenu )
{
    wxString text;

    AddMenuItem( aParentMenu,
                 ID_TO_LIBVIEW,
                 _( "Symbol Library &Browser" ),  HELP_RUN_LIB_VIEWER,
                 KiBitmap( library_browse_xpm ) );

    AddMenuItem( aParentMenu,
                 ID_HIERARCHY,
                 _( "Show &Hierarchical Navigator" ),
                 _( "Navigate schematic hierarchy" ),
                 KiBitmap( hierarchy_nav_xpm ) );

    text = AddHotkeyName( _( "&Leave Sheet" ), g_Schematic_Hotkeys_Descr, HK_LEAVE_SHEET );
    AddMenuItem( aParentMenu,
                 ID_POPUP_SCH_LEAVE_SHEET, text,
                 _( "Return to parent schematic sheet" ),
                 KiBitmap( leave_sheet_xpm ) );

    aParentMenu->AppendSeparator();

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

    text = AddHotkeyName( _( "Zoom &In" ), g_Schematic_Hotkeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );  // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), g_Schematic_Hotkeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );  // add accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Zoom to Fit" ), g_Schematic_Hotkeys_Descr, HK_ZOOM_AUTO );

    AddMenuItem( aParentMenu, ID_ZOOM_PAGE, text,
                 HELP_ZOOM_FIT, KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "Zoom to Selection" ), g_Eeschema_Hotkeys_Descr, HK_ZOOM_SELECTION );

    AddMenuItem( aParentMenu, ID_MENU_ZOOM_SELECTION, text, KiBitmap( zoom_area_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), g_Schematic_Hotkeys_Descr, HK_ZOOM_REDRAW );

    AddMenuItem( aParentMenu, ID_ZOOM_REDRAW, text,
                 HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_TB_OPTIONS_SHOW_GRID,
                 _( "Show &Grid" ), wxEmptyString,
                 KiBitmap( grid_xpm ), wxITEM_CHECK );

    AddMenuItem( aParentMenu, ID_GRID_SETTINGS,
                 _( "Grid Settings..." ), wxEmptyString,
                 KiBitmap( grid_xpm ) );

    // Units submenu
    wxMenu* unitsSubMenu = new wxMenu;
    AddMenuItem( unitsSubMenu, ID_TB_OPTIONS_SELECT_UNIT_INCH,
                 _( "&Imperial" ), _( "Use imperial units" ),
                 KiBitmap( unit_inch_xpm ), wxITEM_RADIO );

    AddMenuItem( unitsSubMenu, ID_TB_OPTIONS_SELECT_UNIT_MM,
                 _( "&Metric" ), _( "Use metric units" ),
                 KiBitmap( unit_mm_xpm ), wxITEM_RADIO );

    AddMenuItem( aParentMenu, unitsSubMenu,
                 -1, _( "&Units" ),
                 _( "Select which units are displayed" ),
                 KiBitmap( unit_mm_xpm ) );

    AddMenuItem( aParentMenu, ID_TB_OPTIONS_SELECT_CURSOR,
                 _( "Full &Window Crosshair" ),
                 _( "Change cursor shape" ),
                 KiBitmap( cursor_shape_xpm ), wxITEM_CHECK );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_TB_OPTIONS_HIDDEN_PINS,
                 _( "Show Hidden &Pins" ),
                 wxEmptyString,
                 KiBitmap( hidden_pin_xpm ), wxITEM_CHECK );

#ifdef __APPLE__
    aParentMenu->AppendSeparator();
#endif
}


void preparePlaceMenu( wxMenu* aParentMenu )
{
    wxString text;

    text = AddHotkeyName( _( "&Symbol" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_NEW_COMPONENT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_PLACE_COMPONENT, text,
                 HELP_PLACE_COMPONENTS,
                 KiBitmap( add_component_xpm ) );

    text = AddHotkeyName( _( "&Power Port" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_NEW_POWER, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_PLACE_POWER_BUTT, text,
                 HELP_PLACE_POWERPORT,
                 KiBitmap( add_power_xpm ) );

    text = AddHotkeyName( _( "&Wire" ), g_Schematic_Hotkeys_Descr,
                          HK_BEGIN_WIRE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_WIRE_BUTT, text,
                 HELP_PLACE_WIRE,
                 KiBitmap( add_line_xpm ) );

    text = AddHotkeyName( _( "&Bus" ), g_Schematic_Hotkeys_Descr,
                          HK_BEGIN_BUS, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_BUS_BUTT, text,
                 HELP_PLACE_BUS,
                 KiBitmap( add_bus_xpm ) );

    text = AddHotkeyName( _( "Wire to Bus &Entry" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_WIRE_ENTRY, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_WIRETOBUS_ENTRY_BUTT, text,
                 HELP_PLACE_WIRE2BUS_ENTRY,
                 KiBitmap( add_line2bus_xpm ) );

    text = AddHotkeyName( _( "Bus &to Bus Entry" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_BUS_ENTRY, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_BUSTOBUS_ENTRY_BUTT, text,
                 HELP_PLACE_BUS2BUS_ENTRY,
                 KiBitmap( add_bus2bus_xpm ) );

    text = AddHotkeyName( _( "&No Connect Flag" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_NOCONN_FLAG, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG, KiBitmap( noconn_xpm ) );

    text = AddHotkeyName( _( "&Junction" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_JUNCTION, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_JUNCTION_BUTT, text,
                 HELP_PLACE_JUNCTION,
                 KiBitmap( add_junction_xpm ) );

    text = AddHotkeyName( _( "&Label" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_LABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_LABEL_BUTT, text,
                 HELP_PLACE_NETLABEL,
                 KiBitmap( add_line_label_xpm ) );

    text = AddHotkeyName( _( "Gl&obal Label" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_GLABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_GLABEL_BUTT, text,
                 HELP_PLACE_GLOBALLABEL,
                 KiBitmap( add_glabel_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Hierarchical Label" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_HLABEL, IS_ACCELERATOR );          // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_HIERLABEL_BUTT,
                 text, HELP_PLACE_HIER_LABEL,
                 KiBitmap( add_hierarchical_label_xpm ) );


    text = AddHotkeyName( _( "Hierar&chical Sheet" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_HIER_SHEET, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_SHEET_SYMBOL_BUTT, text,
                 HELP_PLACE_SHEET,
                 KiBitmap( add_hierarchical_subsheet_xpm ) );

    AddMenuItem( aParentMenu,
                 ID_MENU_IMPORT_HLABEL_BUTT,
                 _( "I&mport Hierarchical Label" ),
                 HELP_IMPORT_SHEETPIN,
                 KiBitmap( import_hierarchical_label_xpm ) );

    AddMenuItem( aParentMenu,
                 ID_MENU_SHEET_PIN_BUTT,
                 _( "Hierarchical Pi&n to Sheet" ),
                 HELP_PLACE_SHEETPIN,
                 KiBitmap( add_hierar_pin_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "Graphic Pol&yline" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_GRAPHIC_POLYLINE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_LINE_COMMENT_BUTT, text,
                 HELP_PLACE_GRAPHICLINES,
                 KiBitmap( add_dashed_line_xpm ) );

    text = AddHotkeyName( _( "&Graphic Text" ), g_Schematic_Hotkeys_Descr,
                          HK_ADD_GRAPHIC_TEXT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( aParentMenu, ID_MENU_TEXT_COMMENT_BUTT, text,
                 HELP_PLACE_GRAPHICTEXTS,
                 KiBitmap( text_xpm ) );

    // Add graphic image
    AddMenuItem( aParentMenu, ID_MENU_ADD_IMAGE_BUTT, _( "&Image" ),
                 HELP_PLACE_GRAPHICIMAGES,
                 KiBitmap( image_xpm ) );
}


void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject )
{
    wxString text;

    // @todo: static probably not OK in multiple open projects.
    // Open Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();

    Kiface().GetFileHistory().UseMenu( openRecentMenu );
    Kiface().GetFileHistory().AddFilesToMenu( openRecentMenu );

    if( aIsOutsideProject )   // not when under a project mgr
    {
        text = AddHotkeyName( _( "&New..." ), g_Schematic_Hotkeys_Descr, HK_NEW );
        AddMenuItem( aParentMenu, ID_NEW_PROJECT, text,
                     _( "Start new schematic root sheet" ),
                     KiBitmap( new_document_xpm ) );

        text = AddHotkeyName( _( "&Open..." ), g_Schematic_Hotkeys_Descr, HK_OPEN );
        AddMenuItem( aParentMenu, ID_LOAD_PROJECT, text,
                     _( "Open existing schematic" ),
                     KiBitmap( open_document_xpm ) );

        AddMenuItem( aParentMenu, openRecentMenu, -1, _( "Open &Recent" ),
                     _( "Open recently opened schematic" ),
                     KiBitmap( recent_xpm ) );

        aParentMenu->AppendSeparator();
    }

    text = AddHotkeyName( _( "&Save" ), g_Schematic_Hotkeys_Descr, HK_SAVE );
    AddMenuItem( aParentMenu, ID_SAVE_PROJECT, text,
                 _( "Save changes" ),
                 KiBitmap( save_xpm ) );

    AddMenuItem( aParentMenu, ID_UPDATE_ONE_SHEET, _( "Save &Current Sheet" ),
                 _( "Save only the current sheet" ),
                 KiBitmap( save_xpm ) );

    text = AddHotkeyName( _( "Save C&urrent Sheet As..." ), g_Schematic_Hotkeys_Descr, HK_SAVEAS );
    AddMenuItem( aParentMenu, ID_SAVE_ONE_SHEET_UNDER_NEW_NAME, text,
                 _( "Save a copy of the current sheet" ),
                 KiBitmap( save_as_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_APPEND_PROJECT, _( "App&end Schematic Sheet Content..." ),
                 _( "Append schematic sheet content from another project to the current sheet" ),
                 KiBitmap( add_document_xpm ) );

    AddMenuItem( aParentMenu, ID_IMPORT_NON_KICAD_SCH, _( "&Import Non KiCad Schematic..." ),
                 _( "Replace current schematic sheet with one imported from another application" ),
                 KiBitmap( import_document_xpm ) );   // TODO needs a different icon

    aParentMenu->AppendSeparator();

    // Import submenu
    wxMenu* submenuImport = new wxMenu();

    AddMenuItem( submenuImport, ID_BACKANNO_ITEMS, _( "&Footprint Association File..." ),
                 HELP_IMPORT_FOOTPRINTS,
                 KiBitmap( import_footprint_names_xpm ) );

    AddMenuItem( aParentMenu, submenuImport, ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ),
                 KiBitmap( import_xpm ) );


    // Export submenu
    wxMenu* submenuExport = new wxMenu();

    AddMenuItem( submenuExport, ID_GEN_COPY_SHEET_TO_CLIPBOARD, _( "Drawing to C&lipboard" ),
                 _( "Export drawings to clipboard" ),
                 KiBitmap( copy_xpm ) );

    AddMenuItem( aParentMenu, submenuExport, ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export files" ),
                 KiBitmap( export_xpm ) );

    aParentMenu->AppendSeparator();

    // Edit page layout:
    AddMenuItem( aParentMenu, ID_SHEET_SET, _( "Page S&ettings..." ),
                 _( "Settings for sheet size and frame references" ),
                 KiBitmap( sheetset_xpm ) );

    text = AddHotkeyName( _( "&Print..." ), g_Schematic_Hotkeys_Descr, HK_PRINT );
    AddMenuItem( aParentMenu, wxID_PRINT, text,
                 _( "Print schematic sheet" ),
                 KiBitmap( print_button_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_PLOT_SCHEMATIC, _( "P&lot..." ),
                 _( "Plot schematic sheet in PostScript, PDF, SVG, DXF or HPGL format" ),
                 KiBitmap( plot_xpm ) );

    aParentMenu->AppendSeparator();

    // Quit
    AddMenuItem( aParentMenu, wxID_EXIT, _( "&Exit" ),
                 _( "Close Eeschema" ),
                 KiBitmap( exit_xpm ) );
}


void prepareEditMenu( wxMenu* aParentMenu )
{
    wxString text;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), g_Schematic_Hotkeys_Descr, HK_UNDO );

    AddMenuItem( aParentMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), g_Schematic_Hotkeys_Descr, HK_REDO );

    AddMenuItem( aParentMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Cut" ), g_Schematic_Hotkeys_Descr, HK_EDIT_CUT );
    AddMenuItem( aParentMenu, wxID_CUT, text,
                 _( "Cuts the selected item(s) to the Clipboard" ),
                 KiBitmap( cut_xpm ) );

    text = AddHotkeyName( _( "&Copy" ), g_Schematic_Hotkeys_Descr, HK_EDIT_COPY );
    AddMenuItem( aParentMenu, wxID_COPY, text,
                 _( "Copies the selected item(s) to the Clipboard" ),
                 KiBitmap( copy_xpm ) );

    text = AddHotkeyName( _( "&Paste" ), g_Schematic_Hotkeys_Descr, HK_EDIT_PASTE );
    AddMenuItem( aParentMenu, wxID_PASTE, text,
                 _( "Pastes item(s) from the Clipboard" ),
                 KiBitmap( paste_xpm ) );

    // Delete
    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, ID_MENU_DELETE_ITEM_BUTT,
                 _( "&Delete" ), HELP_DELETE_ITEMS,
                 KiBitmap( delete_xpm ) );

    // Find
    aParentMenu->AppendSeparator();
    text = AddHotkeyName( _( "&Find..." ), g_Schematic_Hotkeys_Descr, HK_FIND_ITEM );
    AddMenuItem( aParentMenu, ID_FIND_ITEMS, text, HELP_FIND, KiBitmap( find_xpm ) );

    // Find/Replace
    text = AddHotkeyName( _( "Find and Re&place..." ), g_Schematic_Hotkeys_Descr,
                          HK_FIND_REPLACE );
    AddMenuItem( aParentMenu, wxID_REPLACE, text, HELP_REPLACE,
                 KiBitmap( find_replace_xpm ) );

    aParentMenu->AppendSeparator();

    // Update field values
    AddMenuItem( aParentMenu, ID_UPDATE_FIELDS,
                 _( "Update Fields from Library..." ),
                 _( "Sets symbol fields to original library values" ),
                 KiBitmap( update_fields_xpm ) );
}


void prepareInspectMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu, ID_GET_ERC,
                 _( "Electrical Rules &Checker" ),
                 _( "Perform electrical rules check" ),
                 KiBitmap( erc_xpm ) );
}


void prepareToolsMenu( wxMenu* aParentMenu )
{
    wxString text;

    text = AddHotkeyName( _( "Update PCB from Schematic..." ), g_Schematic_Hotkeys_Descr,
                          HK_UPDATE_PCB_FROM_SCH );

    AddMenuItem( aParentMenu,
                 ID_UPDATE_PCB_FROM_SCH,
                 text, _( "Updates PCB design with current schematic (forward annotation)." ),
                 KiBitmap( update_pcb_from_sch_xpm ) );

    // Run Pcbnew
    AddMenuItem( aParentMenu, ID_RUN_PCB, _( "&Open PCB Editor" ),
                 _( "Run Pcbnew" ),
                 KiBitmap( pcbnew_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_RUN_LIBRARY, _( "Symbol Library &Editor" ),
                 HELP_RUN_LIB_EDITOR,
                 KiBitmap( libedit_xpm ) );

    AddMenuItem( aParentMenu, ID_RESCUE_CACHED, _( "&Rescue Symbols..." ),
                 _( "Find old symbols in project and rename/rescue them" ),
                 KiBitmap( rescue_xpm ) );

    AddMenuItem( aParentMenu, ID_REMAP_SYMBOLS, _( "Remap Symbols..." ),
                 _( "Remap legacy library symbols to symbol library table" ),
                 KiBitmap( rescue_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_OPEN_CMP_TABLE, _( "Edit Symbol Field&s..." ),
                 KiBitmap( spreadsheet_xpm ) );

    AddMenuItem( aParentMenu, ID_EDIT_COMPONENTS_TO_SYMBOLS_LIB_ID,
                 _( "Edit Symbol Library References..." ),
                 _( "Edit links between schematic symbols and library symbols" ),
                 KiBitmap( edit_cmp_symb_links_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_GET_ANNOTATE, _( "&Annotate Schematic..." ),
                 HELP_ANNOTATE,
                 KiBitmap( annotate_xpm ) );

    AddMenuItem( aParentMenu, ID_GET_NETLIST, _( "Generate &Netlist File..." ),
                 _( "Generate netlist file" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( aParentMenu, ID_GET_TOOLS, _( "Generate Bill of &Materials..." ),
                 HELP_GENERATE_BOM,
                 KiBitmap( bom_xpm ) );

    aParentMenu->AppendSeparator();

    // Run CvPcb
    AddMenuItem( aParentMenu, ID_RUN_CVPCB, _( "A&ssign Footprints..." ),
                 _( "Assign PCB footprints to schematic symbols" ),
                 KiBitmap( cvpcb_xpm ) );

    aParentMenu->AppendSeparator();

#ifdef KICAD_SPICE
    // Simulator
    AddMenuItem( aParentMenu, ID_SIM_SHOW, _("Simula&tor"),
                 _( "Simulate circuit" ),
                 KiBitmap( simulator_xpm ) );
#endif /* KICAD_SPICE */

}


void prepareHelpMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu, wxID_HELP, _( "Eeschema &Manual" ),
                 _( "Open Eeschema Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( aParentMenu, wxID_INDEX, _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    wxString text = AddHotkeyName( _( "&List Hotkeys..." ), g_Eeschema_Hotkeys_Descr, HK_HELP );
    AddMenuItem( aParentMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST, text,
                 _( "Displays current hotkeys table and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, ID_HELP_GET_INVOLVED, _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );
}


static void preparePreferencesMenu( SCH_EDIT_FRAME* aFrame, wxMenu* aParentMenu )
{
    // Path configuration edit dialog.
    AddMenuItem( aParentMenu, ID_PREFERENCES_CONFIGURE_PATHS, _( "Configure Pa&ths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    // Library
    AddMenuItem( aParentMenu, ID_EDIT_SYM_LIB_TABLE, _( "Manage Symbol Libraries..." ),
                 _( "Edit the global and project symbol library lists" ),
                 KiBitmap( library_table_xpm ) );

    // Options (Preferences on WXMAC)
    wxString text = AddHotkeyName( _( "&Preferences..." ), g_Eeschema_Hotkeys_Descr, HK_PREFERENCES );
    AddMenuItem( aParentMenu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    aParentMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( aParentMenu );

#ifndef __WXMAC__
    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), g_Eeschema_Hotkeys_Descr,
                          HK_CANVAS_OPENGL );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (Fallba&ck)" ), g_Eeschema_Hotkeys_Descr,
                          HK_CANVAS_CAIRO );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );
#endif

    aParentMenu->AppendSeparator();

    // Import/export
    AddMenuItem( aParentMenu, ID_CONFIG_SAVE, _( "&Save Project File..." ),
                 _( "Save project preferences into a project file" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( aParentMenu, ID_CONFIG_READ, _( "Load P&roject File..." ),
                 _( "Load project preferences from a project file" ),
                 KiBitmap( import_setup_xpm ) );
}
