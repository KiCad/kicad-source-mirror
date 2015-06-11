/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009-2014 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2014 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <kiface_i.h>
#include <pgm_base.h>
#include <schframe.h>

#include <general.h>
#include <eeschema_id.h>
#include <hotkeys.h>
#include <menus_helpers.h>

#include <help_common_strings.h>

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

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        AddMenuItem( fileMenu,
                     ID_NEW_PROJECT,
                     _( "&New Schematic Project" ),
                     _( "Clear current schematic hierarchy and start a new schematic root sheet" ),
                     KiBitmap( new_xpm ) );

        text = AddHotkeyName( _( "&Open Schematic Project" ), g_Schematic_Hokeys_Descr, HK_LOAD_SCH );
        AddMenuItem( fileMenu,
                     ID_LOAD_PROJECT, text,
                     _( "Open an existing schematic hierarchy" ),
                     KiBitmap( open_document_xpm ) );
    }

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

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        AddMenuItem( fileMenu, openRecentMenu,
                     wxID_ANY, _( "Open &Recent" ),
                     _( "Open a recent opened schematic project" ),
                     KiBitmap( open_project_xpm ) );
    }

    AddMenuItem( fileMenu,
                 ID_APPEND_PROJECT, _( "App&end Schematic Sheet" ),
                 _( "Append schematic sheet to current project" ),
                 KiBitmap( open_document_xpm ) );

    fileMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Save Schematic Project" ),
                          g_Schematic_Hokeys_Descr, HK_SAVE_SCH );
    AddMenuItem( fileMenu,
                 ID_SAVE_PROJECT, text,
                 _( "Save all sheets in schematic project" ),
                 KiBitmap( save_project_xpm ) );

    AddMenuItem( fileMenu,
                 ID_UPDATE_ONE_SHEET,
                 _( "Save &Current Sheet Only" ),
                 _( "Save only current schematic sheet" ),
                 KiBitmap( save_xpm ) );

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        AddMenuItem( fileMenu,
                     ID_SAVE_ONE_SHEET_UNDER_NEW_NAME,
                     _( "Save C&urrent Sheet As" ),
                     _( "Save current schematic sheet as..." ),
                     KiBitmap( save_as_xpm ) );
    }

    fileMenu->AppendSeparator();

    AddMenuItem( fileMenu,
                 ID_SHEET_SET,
                 _( "Pa&ge Settings" ),
                 _( "Setting for sheet size and frame references" ),
                 KiBitmap( sheetset_xpm ) );

    AddMenuItem( fileMenu,
                 wxID_PRINT,
                 _( "Pri&nt" ),
                 _( "Print schematic sheet" ),
                 KiBitmap( print_button_xpm ) );

    // Plot submenu
    wxMenu* choice_plot_fmt = new wxMenu;
    AddMenuItem( choice_plot_fmt, ID_GEN_PLOT_SCHEMATIC,
                 _( "&Plot" ),
                 _( "Plot schematic sheet in PostScript, PDF, SVG, DXF or HPGL format" ),
                 KiBitmap( plot_xpm ) );

    // Plot to Clipboard
    AddMenuItem( choice_plot_fmt, ID_GEN_COPY_SHEET_TO_CLIPBOARD,
                 _( "Plot to C&lipboard" ),
                 _( "Export drawings to clipboard" ),
                 KiBitmap( copy_button_xpm ) );

    // Plot
    AddMenuItem( fileMenu, choice_plot_fmt,
                 ID_GEN_PLOT, _( "&Plot" ),
                 _( "Plot schematic sheet in HPGL, PostScript or SVG format" ),
                 KiBitmap( plot_xpm ) );

    // Separator
    fileMenu->AppendSeparator();

    // Quit
    AddMenuItem( fileMenu,
                 wxID_EXIT,
                 _( "&Close" ),
                 _( "Close Eeschema" ),
                 KiBitmap( exit_xpm ) );

    // Menu Edit:
    wxMenu* editMenu = new wxMenu;

    // Undo
    text = AddHotkeyName( _( "&Undo" ), g_Schematic_Hokeys_Descr, HK_UNDO );

    AddMenuItem( editMenu, wxID_UNDO, text, HELP_UNDO, KiBitmap( undo_xpm ) );

    // Redo
    text = AddHotkeyName( _( "&Redo" ), g_Schematic_Hokeys_Descr, HK_REDO );

    AddMenuItem( editMenu, wxID_REDO, text, HELP_REDO, KiBitmap( redo_xpm ) );

    // Delete
    editMenu->AppendSeparator();
    AddMenuItem( editMenu, ID_SCHEMATIC_DELETE_ITEM_BUTT,
                 _( "&Delete" ), HELP_DELETE_ITEMS,
                 KiBitmap( delete_xpm ) );

    // Find
    editMenu->AppendSeparator();
    text = AddHotkeyName( _( "&Find" ), g_Schematic_Hokeys_Descr, HK_FIND_ITEM );
    AddMenuItem( editMenu, ID_FIND_ITEMS, text, HELP_FIND, KiBitmap( find_xpm ) );

    // Find/Replace
    text = AddHotkeyName( _( "Find and Re&place" ), g_Schematic_Hokeys_Descr,
                          HK_FIND_REPLACE );
    AddMenuItem( editMenu, wxID_REPLACE, text, HELP_REPLACE,
                 KiBitmap( find_replace_xpm ) );

    // Import footprint association from the CvPcb cmp file:
    editMenu->AppendSeparator();
    AddMenuItem( editMenu, ID_BACKANNO_ITEMS,
                 _( "Import Footprint Selection" ),
                 HELP_IMPORT_FOOTPRINTS,
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

    text = AddHotkeyName( _( "Zoom &In" ), g_Schematic_Hokeys_Descr,
                          HK_ZOOM_IN, IS_ACCELERATOR );  // add an accelerator, not a shortcut
    AddMenuItem( viewMenu, ID_ZOOM_IN, text, HELP_ZOOM_IN, KiBitmap( zoom_in_xpm ) );

    text = AddHotkeyName( _( "Zoom &Out" ), g_Schematic_Hokeys_Descr,
                          HK_ZOOM_OUT, IS_ACCELERATOR );  // add accelerator, not a shortcut
    AddMenuItem( viewMenu, ID_ZOOM_OUT, text, HELP_ZOOM_OUT, KiBitmap( zoom_out_xpm ) );

    text = AddHotkeyName( _( "&Fit on Screen" ), g_Schematic_Hokeys_Descr, HK_ZOOM_AUTO );

    AddMenuItem( viewMenu, ID_ZOOM_PAGE, text, HELP_ZOOM_FIT, KiBitmap( zoom_fit_in_page_xpm ) );

    text = AddHotkeyName( _( "&Redraw" ), g_Schematic_Hokeys_Descr, HK_ZOOM_REDRAW );

    AddMenuItem( viewMenu, ID_ZOOM_REDRAW, text, HELP_ZOOM_REDRAW, KiBitmap( zoom_redraw_xpm ) );

    viewMenu->AppendSeparator();

    AddMenuItem( viewMenu,
                 ID_HIERARCHY,
                 _( "Show &Hierarchical Navigator" ),
                 _( "Navigate hierarchical sheets" ),
                 KiBitmap( hierarchy_nav_xpm ) );

    AddMenuItem( viewMenu,
                 ID_POPUP_SCH_LEAVE_SHEET,
                 _( "&Leave Sheet" ),
                 _( "Leave Sheet" ),
                 KiBitmap( leave_sheet_xpm ) );

    // Menu place:
    wxMenu* placeMenu = new wxMenu;

    text = AddHotkeyName( _( "&Component" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_COMPONENT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_SCH_PLACE_COMPONENT, text,
                 HELP_PLACE_COMPONENTS,
                 KiBitmap( add_component_xpm ) );

    text = AddHotkeyName( _( "&Power Port" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_NEW_POWER, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_PLACE_POWER_BUTT, text,
                 HELP_PLACE_POWERPORT,
                 KiBitmap( add_power_xpm ) );

    text = AddHotkeyName( _( "&Wire" ), g_Schematic_Hokeys_Descr,
                          HK_BEGIN_WIRE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_WIRE_BUTT, text,
                 HELP_PLACE_WIRE,
                 KiBitmap( add_line_xpm ) );

    text = AddHotkeyName( _( "&Bus" ), g_Schematic_Hokeys_Descr,
                          HK_BEGIN_BUS, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_BUS_BUTT, text,
                 HELP_PLACE_BUS,
                 KiBitmap( add_bus_xpm ) );

    text = AddHotkeyName( _( "Wire to Bus &Entry" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_WIRE_ENTRY, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_WIRETOBUS_ENTRY_BUTT, text,
                 HELP_PLACE_WIRE2BUS_ENTRY,
                 KiBitmap( add_line2bus_xpm ) );

    text = AddHotkeyName( _( "Bus &to Bus Entry" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_BUS_ENTRY, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_BUSTOBUS_ENTRY_BUTT, text,
                 HELP_PLACE_BUS2BUS_ENTRY,
                 KiBitmap( add_bus2bus_xpm ) );

    text = AddHotkeyName( _( "&No Connect Flag" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_NOCONN_FLAG, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_NOCONN_BUTT, text, HELP_PLACE_NC_FLAG, KiBitmap( noconn_xpm ) );

    text = AddHotkeyName( _( "&Junction" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_JUNCTION, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_JUNCTION_BUTT, text,
                 HELP_PLACE_JUNCTION,
                 KiBitmap( add_junction_xpm ) );

    text = AddHotkeyName( _( "&Label" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_LABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_LABEL_BUTT, text,
                 HELP_PLACE_NETLABEL,
                 KiBitmap( add_line_label_xpm ) );

    text = AddHotkeyName( _( "Gl&obal Label" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_GLABEL, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_GLABEL_BUTT, text,
                 HELP_PLACE_GLOBALLABEL,
                 KiBitmap( add_glabel_xpm ) );

    placeMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Hierarchical Label" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_HLABEL, IS_ACCELERATOR );          // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_HIERLABEL_BUTT,
                 text, HELP_PLACE_HIER_LABEL,
                 KiBitmap( add_hierarchical_label_xpm ) );


    text = AddHotkeyName( _( "Hierarchical &Sheet" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_HIER_SHEET, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_SHEET_SYMBOL_BUTT, text,
                 HELP_PLACE_SHEET,
                 KiBitmap( add_hierarchical_subsheet_xpm ) );

    AddMenuItem( placeMenu,
                 ID_IMPORT_HLABEL_BUTT,
                 _( "I&mport Hierarchical Label" ),
                 HELP_IMPORT_SHEETPIN,
                 KiBitmap( import_hierarchical_label_xpm ) );

    AddMenuItem( placeMenu,
                 ID_SHEET_PIN_BUTT,
                 _( "Hierarchical Pi&n to Sheet" ),
                 HELP_PLACE_SHEETPIN,
                 KiBitmap( add_hierar_pin_xpm ) );

    placeMenu->AppendSeparator();

    text = AddHotkeyName( _( "Graphic Pol&yline" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_POLYLINE, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_LINE_COMMENT_BUTT, text,
                 HELP_PLACE_GRAPHICLINES,
                 KiBitmap( add_dashed_line_xpm ) );

    text = AddHotkeyName( _( "&Graphic Text" ), g_Schematic_Hokeys_Descr,
                          HK_ADD_GRAPHIC_TEXT, IS_ACCELERATOR );    // add an accelerator, not a shortcut
    AddMenuItem( placeMenu, ID_TEXT_COMMENT_BUTT, text,
                 HELP_PLACE_GRAPHICTEXTS,
                 KiBitmap( add_text_xpm ) );

    // Graphic image
    AddMenuItem( placeMenu, ID_ADD_IMAGE_BUTT, _( "&Image" ),
                 HELP_PLACE_GRAPHICIMAGES,
                 KiBitmap( image_xpm ) );

    // Menu Preferences:
    wxMenu* preferencesMenu = new wxMenu;

    // Library
    AddMenuItem( preferencesMenu,
                 ID_CONFIG_REQ,
                 _( "Component &Libraries" ),
                 _( "Configure component libraries and paths" ),
                 KiBitmap( library_xpm ) );

    // Colors
    AddMenuItem( preferencesMenu,
                 ID_COLORS_SETUP,
                 _( "Set &Colors Scheme" ),
                 _( "Set color preferences" ),
                 KiBitmap( palette_xpm ) );

    // Options (Preferences on WXMAC)

#ifdef __WXMAC__
    preferencesMenu->Append( wxID_PREFERENCES );
#else
    AddMenuItem( preferencesMenu,
                 wxID_PREFERENCES,
                 _( "Schematic Editor &Options" ),
                 _( "Set Eeschema preferences" ),
                 KiBitmap( preference_xpm ) );
#endif // __WXMAC__


    // Language submenu
    Pgm().AddMenuLanguageList( preferencesMenu );

    // Hotkey submenu
    AddHotkeyConfigMenu( preferencesMenu );

    // Separator
    preferencesMenu->AppendSeparator();

    AddMenuItem( preferencesMenu,
                 ID_CONFIG_SAVE,
                 _( "&Save Preferences" ),
                 _( "Save application preferences" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( preferencesMenu,
                 ID_CONFIG_READ,
                 _( "Load Prefe&rences" ),
                 _( "Load application preferences" ),
                 KiBitmap( read_setup_xpm ) );

    // Menu Tools:
    wxMenu* toolsMenu = new wxMenu;

    AddMenuItem( toolsMenu,
                 ID_RUN_LIBRARY,
                 _( "Library &Editor" ), HELP_RUN_LIB_EDITOR,
                 KiBitmap( libedit_xpm ) );

    AddMenuItem( toolsMenu,
                 ID_TO_LIBVIEW,
                 _( "Library &Browser" ),  HELP_RUN_LIB_VIEWER,
                 KiBitmap( library_browse_xpm ) );

    AddMenuItem( toolsMenu,
                 ID_RESCUE_CACHED,
                 _( "&Rescue Cached Components" ),
                 _( "Find old components in the project cache and rescue them to a new library" ),
                 KiBitmap( copycomponent_xpm ) );

    toolsMenu->AppendSeparator();

    AddMenuItem( toolsMenu,
                 ID_GET_ANNOTATE,
                 _( "&Annotate Schematic" ), HELP_ANNOTATE,
                 KiBitmap( annotate_xpm ) );

    // ERC
    AddMenuItem( toolsMenu,
                 ID_GET_ERC,
                 _( "Electrical Rules &Checker" ),
                 _( "Perform electrical rules check" ),
                 KiBitmap( erc_xpm ) );

    AddMenuItem( toolsMenu,
                 ID_GET_NETLIST,
                 _( "Generate &Netlist File" ),
                 _( "Generate the component netlist file" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( toolsMenu,
                 ID_GET_TOOLS,
                 _( "Generate Bill of &Materials" ),
                 HELP_GENERATE_BOM,
                 KiBitmap( bom_xpm ) );

    toolsMenu->AppendSeparator();

    // Run CvPcb
    AddMenuItem( toolsMenu,
                 ID_RUN_CVPCB,
                 _( "A&ssign Component Footprint" ),
                 _( "Run CvPcb" ),
                 KiBitmap( cvpcb_xpm ) );

    // Run Pcbnew
    AddMenuItem( toolsMenu,
                 ID_RUN_PCB,
                 _( "&Layout Printed Circuit Board" ),
                 _( "Run Pcbnew" ),
                 KiBitmap( pcbnew_xpm ) );

    // Help Menu:
    wxMenu* helpMenu = new wxMenu;

    // Version info
    AddHelpVersionInfoMenuEntry( helpMenu );

    AddMenuItem( helpMenu,
                 wxID_HELP,
                 _( "Eesc&hema Manual" ),
                 _( "Open Eeschema manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( helpMenu,
                 wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

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
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    menuBar->Thaw();

    // Associate the menu bar with the frame, if no previous menubar
    if( GetMenuBar() == NULL )
        SetMenuBar( menuBar );
    else
        menuBar->Refresh();
}
