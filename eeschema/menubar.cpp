/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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


#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_actions.h>

#include "eeschema_id.h"
#include "general.h"
#include "help_common_strings.h"
#include "ee_hotkeys.h"
#include "sch_edit_frame.h"

class CONDITIONAL_MENU;

// helper functions that build specific submenus:

// Build the tools menu
static void prepareToolsMenu( wxMenu* aParentMenu );

// Build the preferences menu
static void preparePreferencesMenu( SCH_EDIT_FRAME* aFrame, wxMenu* aParentMenu );


void SCH_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    auto modifiedDocumentCondition = [] ( const SELECTION& sel ) {
        SCH_SHEET_LIST sheetList( g_RootSheet );
        return sheetList.IsModified();
    };

    //
    // Menu File:
    //
    CONDITIONAL_MENU*   fileMenu = new CONDITIONAL_MENU( false, selTool );
    static ACTION_MENU* openRecentMenu;

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        // Add this menu to list menu managed by m_fileHistory
        // (the file history will be updated when adding/removing files in history)
        if( openRecentMenu )
            Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

        openRecentMenu = new ACTION_MENU();
        openRecentMenu->SetTool( selTool );
        openRecentMenu->SetTitle( _( "Open Recent" ) );
        openRecentMenu->SetIcon( recent_xpm );

        Kiface().GetFileHistory().UseMenu( openRecentMenu );
        Kiface().GetFileHistory().AddFilesToMenu( openRecentMenu );

        fileMenu->AddItem( ACTIONS::doNew,         EE_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ACTIONS::open,          EE_CONDITIONS::ShowAlways );
        fileMenu->AddMenu( openRecentMenu,         EE_CONDITIONS::ShowAlways );
        fileMenu->AddSeparator();
    }

    fileMenu->AddItem( ACTIONS::save,              modifiedDocumentCondition );
    fileMenu->AddItem( ACTIONS::saveAs,            EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::saveAll,           modifiedDocumentCondition );

    fileMenu->AddSeparator();

    fileMenu->AddItem( ID_APPEND_PROJECT, _( "Append Schematic Sheet Content..." ),
                       _( "Append schematic sheet content from another project to the current sheet" ),
                       add_document_xpm,           EE_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_IMPORT_NON_KICAD_SCH, _( "Import Non KiCad Schematic..." ),
                       _( "Replace current schematic sheet with one imported from another application" ),
                       import_document_xpm,        EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU();
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );

    submenuImport->Add( _( "Footprint Association File..." ), HELP_IMPORT_FOOTPRINTS,
                        ID_BACKANNO_ITEMS, import_footprint_names_xpm );

    fileMenu->AddMenu( submenuImport,              EE_CONDITIONS::ShowAlways );


    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU();
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );

    submenuExport->Add( _( "Drawing to Clipboard" ), _( "Export drawings to clipboard" ),
                        ID_GEN_COPY_SHEET_TO_CLIPBOARD, copy_xpm );

    submenuExport->Add( _( "Netlist..." ),  _( "Export netlist file" ),
                        ID_GET_NETLIST, netlist_xpm );

    fileMenu->AddMenu( submenuExport,              EE_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::pageSettings,         EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::print,             EE_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::plot,              EE_CONDITIONS::ShowAlways );

    // Quit
    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::quit,              EE_CONDITIONS::ShowAlways );

    //
    // Menu Edit:
    //
    CONDITIONAL_MENU* editMenu = new CONDITIONAL_MENU( false, selTool );

    auto enableUndoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetUndoCommandCount() > 0;
    };
    auto enableRedoCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen() && GetScreen()->GetRedoCommandCount() > 0;
    };
    auto noActiveToolCondition = [ this ] ( const SELECTION& aSelection ) {
        return GetToolId() == ID_NO_TOOL_SELECTED;
    };

    editMenu->AddItem( ACTIONS::undo,                enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                 SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,               noActiveToolCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( EE_ACTIONS::deleteItemCursor, SELECTION_CONDITIONS::ShowAlways );

    // Find
    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::find,                SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ACTIONS::findAndReplace,      SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    // Update field values
    editMenu->AddItem( ID_UPDATE_FIELDS, _( "Update Fields from Library..." ),
                       _( "Sets symbol fields to original library values" ),
                       update_fields_xpm,            SELECTION_CONDITIONS::ShowAlways );

    //
    // Menu View:
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto belowRootSheetCondition = [] ( const SELECTION& aSel ) {
        return g_CurrentSheet->Last() != g_RootSheet;
    };
    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto imperialUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == INCHES;
    };
    auto metricUnitsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetUserUnits() == MILLIMETRES;
    };
    auto fullCrosshairCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalDisplayOptions().m_fullscreenCursor;
    };
    auto hiddenPinsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowAllPins();
    };

    viewMenu->AddItem( EE_ACTIONS::showLibraryBrowser,    EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EE_ACTIONS::navigateHierarchy,     EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( EE_ACTIONS::leaveSheet,            belowRootSheetCondition );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                 EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,               EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,          gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,           EE_CONDITIONS::ShowAlways );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,   imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,     metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,   fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( EE_ACTIONS::toggleHiddenPins, hiddenPinsCondition );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

    //
    // Menu place:
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( EE_ACTIONS::placeSymbol,            EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placePower,             EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawWire,               EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawBus,                EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeBusWireEntry,      EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeBusBusEntry,       EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeNoConnect,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeJunction,          EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeLabel,             EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeGlobalLabel,       EE_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( EE_ACTIONS::placeHierarchicalLabel, EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::drawSheet,              EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::importSheetPin,         EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSheetPin,          EE_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( EE_ACTIONS::drawLines,              EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeSchematicText,     EE_CONDITIONS::ShowAlways );
    placeMenu->AddItem( EE_ACTIONS::placeImage,             EE_CONDITIONS::ShowAlways );

    //
    // Menu Inspect:
    //
    wxMenu* inspectMenu = new wxMenu;
    AddMenuItem( inspectMenu, ID_GET_ERC, _( "Electrical Rules &Checker" ),
                 _( "Perform electrical rules check" ), KiBitmap( erc_xpm ) );

    //
    // Menu Tools:
    //
    wxMenu* toolsMenu = new wxMenu;
    prepareToolsMenu( toolsMenu );

    //
    // Menu Preferences:
    //
    wxMenu* preferencesMenu = new wxMenu;
    preparePreferencesMenu( this, preferencesMenu );

    // Create the menubar and append all submenus
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( preferencesMenu, _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


void prepareToolsMenu( wxMenu* aParentMenu )
{
    wxString text;

    text = AddHotkeyName( _( "Update PCB from Schematic..." ), g_Schematic_Hotkeys_Descr,
                          HK_UPDATE_PCB_FROM_SCH );

    AddMenuItem( aParentMenu,
                 ID_UPDATE_PCB_FROM_SCH,
                 text, _( "Update PCB design with current schematic." ),
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

    AddMenuItem( aParentMenu, ID_REMAP_SYMBOLS, _( "Remap S&ymbols..." ),
                 _( "Remap legacy library symbols to symbol library table" ),
                 KiBitmap( rescue_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_OPEN_CMP_TABLE, _( "Edit Symbol Field&s..." ),
                 KiBitmap( spreadsheet_xpm ) );

    AddMenuItem( aParentMenu, ID_EDIT_COMPONENTS_TO_SYMBOLS_LIB_ID,
                 _( "Edit Symbol &Library References..." ),
                 _( "Edit links between schematic symbols and library symbols" ),
                 KiBitmap( edit_cmp_symb_links_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_GET_ANNOTATE, _( "&Annotate Schematic..." ),
                 HELP_ANNOTATE,
                 KiBitmap( annotate_xpm ) );

    AddMenuItem( aParentMenu, ID_BUS_MANAGER, _( "Bus &Definitions..." ),
                 HELP_BUS_MANAGER,
                 KiBitmap( bus_definition_tool_xpm ) );

    aParentMenu->AppendSeparator();

    // Run CvPcb
    AddMenuItem( aParentMenu, ID_RUN_CVPCB, _( "A&ssign Footprints..." ),
                 _( "Assign PCB footprints to schematic symbols" ),
                 KiBitmap( cvpcb_xpm ) );

    AddMenuItem( aParentMenu, ID_GET_TOOLS, _( "Generate Bill of &Materials..." ),
                 HELP_GENERATE_BOM,
                 KiBitmap( bom_xpm ) );

    aParentMenu->AppendSeparator();

#ifdef KICAD_SPICE
    // Simulator
    AddMenuItem( aParentMenu, ID_SIM_SHOW, _("Simula&tor"),
                 _( "Simulate circuit" ),
                 KiBitmap( simulator_xpm ) );
#endif /* KICAD_SPICE */

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

    aParentMenu->AppendSeparator();

    // Import/export
    AddMenuItem( aParentMenu, ID_CONFIG_SAVE, _( "&Save Project File..." ),
                 _( "Save project preferences into a project file" ),
                 KiBitmap( save_setup_xpm ) );

    AddMenuItem( aParentMenu, ID_CONFIG_READ, _( "Load P&roject File..." ),
                 _( "Load project preferences from a project file" ),
                 KiBitmap( import_setup_xpm ) );
}
