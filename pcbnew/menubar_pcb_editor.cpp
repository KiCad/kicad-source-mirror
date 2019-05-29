/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <pcb_edit_frame.h>

#include <advanced_config.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/tool_manager.h>
#include <tool/conditional_menu.h>
#include <tool/actions.h>
#include <tool/selection_conditions.h>
#include <tools/selection_tool.h>
#include <tools/pcb_actions.h>
#include "help_common_strings.h"
#include "hotkeys.h"
#include "pcbnew.h"
#include "pcbnew_id.h"


// Build the route menu
static void prepareRouteMenu( wxMenu* aParentMenu );

// Build the tools menu
static void prepareToolsMenu( wxMenu* aParentMenu );


void PCB_EDIT_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    auto modifiedDocumentCondition = [ this ] ( const SELECTION& sel ) {
        return GetScreen()->IsModify();
    };

    // Recreate all menus:

    //-- File menu -----------------------------------------------------------
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

        fileMenu->AddItem( ACTIONS::doNew,         SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ACTIONS::open,          SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddMenu( openRecentMenu,         SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddSeparator();
    }

    fileMenu->AddItem( ACTIONS::save,              modifiedDocumentCondition );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    if( Kiface().IsSingle() )
        fileMenu->AddItem( ACTIONS::saveAs,         SELECTION_CONDITIONS::ShowAlways );
    else
        fileMenu->AddItem( ACTIONS::saveCopyAs,     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_MENU_RECOVER_BOARD_AUTOSAVE,
                       _( "Resc&ue" ),
                       _( "Clear board and get last rescue file automatically saved by Pcbnew" ),
                       rescue_xpm,                  SELECTION_CONDITIONS::ShowAlways );

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        fileMenu->AddItem( PCB_ACTIONS::appendBoard, SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ID_IMPORT_NON_KICAD_BOARD,
                           _( "Import Non-KiCad Board File..." ),
                           _( "Import board file from other applications" ),
                           import_brd_file_xpm,      SELECTION_CONDITIONS::ShowAlways );
    }

    fileMenu->AddItem( ID_MENU_READ_BOARD_BACKUP_FILE,
                       _( "Revert to Last Backup" ),
                       _( "Clear board and get previous backup version of board" ),
                       undo_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU();
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );

    submenuImport->Add( _( "Netlist..." ), _( "Read netlist and update board connectivity" ),
                        ID_GET_NETLIST, netlist_xpm );
    submenuImport->Add( _( "Specctra Session..." ), _( "Import routed Specctra session (*.ses) file" ),
                        ID_GEN_IMPORT_SPECCTRA_SESSION, import_xpm );
    submenuImport->Add( _( "Graphics..." ), _( "Import 2D drawing file" ),
                        ID_GEN_IMPORT_GRAPHICS_FILE, import_vector_xpm );

    fileMenu->AddMenu( submenuImport,                SELECTION_CONDITIONS::ShowAlways );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU();
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );

    submenuExport->Add( _( "Specctra DSN..." ), _( "Export Specctra DSN routing info" ),
                        ID_GEN_EXPORT_SPECCTRA, export_dsn_xpm );
    submenuExport->Add( _( "GenCAD..." ), _( "Export GenCAD board representation" ),
                        ID_GEN_EXPORT_FILE_GENCADFORMAT, export_xpm );
    submenuExport->Add( _( "VRML..." ), _( "Export VRML 3D board representation" ),
                        ID_GEN_EXPORT_FILE_VRML, export3d_xpm );
    submenuExport->Add( _( "IDFv3..." ), _( "Export IDF 3D board representation" ),
                        ID_GEN_EXPORT_FILE_IDF3, export_idf_xpm );
    submenuExport->Add( _( "STEP..." ), _( "Export STEP 3D board representation" ),
                        ID_GEN_EXPORT_FILE_STEP, export_step_xpm );
    submenuExport->Add( _( "SVG..." ), _( "Export SVG board representation" ),
                        ID_GEN_PLOT_SVG, plot_svg_xpm );
    submenuExport->Add( _( "Footprint Association (.cmp) File..." ),
                        _( "Export footprint association file (*.cmp) for schematic back annotation" ),
                        ID_PCB_GEN_CMP_FILE, create_cmp_file_xpm );
    submenuExport->Add( _( "Hyperlynx..." ), "",
                        ID_GEN_EXPORT_FILE_HYPERLYNX, export_step_xpm );

    fileMenu->AddMenu( submenuExport,                SELECTION_CONDITIONS::ShowAlways );

    // Fabrication Outputs submenu
    ACTION_MENU* submenuFabOutputs = new ACTION_MENU();
    submenuFabOutputs->SetTool( selTool );
    submenuFabOutputs->SetTitle( _( "Fabrication Outputs" ) );
    submenuFabOutputs->SetIcon( fabrication_xpm );

    submenuFabOutputs->Add( _( "&Gerbers (.gbr)..." ),
                            _( "Generate Gerbers for fabrication" ),
                            ID_GEN_PLOT_GERBER, post_compo_xpm );
    submenuFabOutputs->Add( _( "&Drill Files (.drl)..." ),
                            _( "Generate Excellon drill file(s)" ),
                            ID_PCB_GEN_DRILL_FILE, post_drill_xpm );
    submenuFabOutputs->Add( _( "Footprint &Positions (.pos)..." ),
                            _( "Generate footprint position file for pick and place" ),
                            ID_PCB_GEN_POS_MODULES_FILE, post_compo_xpm );
    submenuFabOutputs->Add( _( "&Footprint Report (.rpt)..." ),
                            _( "Create report of all footprints from current board" ),
                            ID_GEN_EXPORT_FILE_MODULE_REPORT, tools_xpm );
    submenuFabOutputs->Add( _( "IPC-D-356 Netlist File..." ),
                            _( "Generate IPC-D-356 netlist file" ),
                            ID_PCB_GEN_D356_FILE, netlist_xpm );
    submenuFabOutputs->Add( _( "&BOM..." ),
                            _( "Create bill of materials from current schematic" ),
                            ID_PCB_GEN_BOM_FILE_FROM_BOARD, bom_xpm );

    fileMenu->AddMenu( submenuFabOutputs,              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_BOARD_SETUP_DIALOG,
                       _( "&Board Setup..." ),
                       _( "Edit board setup including layers, design rules and various defaults" ),
                       options_board_xpm,              SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::pageSettings,      SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::print,             SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::plot,              SELECTION_CONDITIONS::ShowAlways );


    // Archive submenu
    ACTION_MENU* submenuArchive = new ACTION_MENU();
    submenuArchive->SetTool( selTool );
    submenuArchive->SetTitle( _( "Archive Footprints" ) );
    submenuArchive->SetIcon( library_archive_xpm );

    submenuArchive->Add( _( "&Archive Footprints in Existing Library..." ),
                         _( "Archive all footprints to existing library in footprint Lib table"
                            "(does not remove other footprints in this library)" ),
                         ID_MENU_ARCHIVE_MODULES_IN_LIBRARY, library_archive_xpm );

    submenuArchive->Add( _( "&Create New Library and Archive Footprints..." ),
                         _( "Archive all footprints to a new library\n"
                            "(if the library already exists it will be replaced)" ),
                         ID_MENU_CREATE_LIBRARY_AND_ARCHIVE_MODULES, library_archive_as_xpm );

    fileMenu->AddSeparator();
    fileMenu->AddMenu( submenuArchive,             SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    // Don't use ACTIONS::quit; wxWidgets moves this on OSX and expects to find it via wxID_EXIT
    fileMenu->AddItem( wxID_EXIT, _( "Quit" ), "", exit_xpm, SELECTION_CONDITIONS::ShowAlways );

    //-- Edit menu -----------------------------------------------------------
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

    editMenu->AddItem( ACTIONS::undo,                   enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                   enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                    SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                   SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,                  noActiveToolCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::deleteTool,         SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::find,                   SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ID_PCB_EDIT_TRACKS_AND_VIAS,
                       _( "Edit Track && Via Properties..." ), "",
                       width_track_via_xpm,             SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_EDIT_TEXT_AND_GRAPHICS,
                       _( "Edit Text && Graphic Properties..." ), "",
                       reset_text_xpm,                  SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::exchangeFootprints, SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_SWAP_LAYERS,
                       _( "Swap Layers..." ),
                       _( "Move tracks or drawings from a layer to another layer" ),
                       swap_layer_xpm,                  SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::zoneFillAll,        SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::zoneUnfillAll,      SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ID_PCB_GLOBAL_DELETE,
                       _( "Global Deletions..." ),
                       _( "Delete tracks, footprints and graphic items from board" ),
                       general_deletions_xpm,          SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_CLEAN,
                       _( "Cleanup Tracks and Vias..." ),
                       _( "Clean stubs, vias, delete break points or unconnected tracks" ),
                       delete_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    //----- View menu -----------------------------------------------------------
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, selTool );

    auto layersPaletteShownCondition = [ this ] ( const SELECTION& aSel ) {
        return LayerManagerShown();
    };
    auto microwaveToolbarShownCondition = [ this ] ( const SELECTION& aSel ) {
        return MicrowaveToolbarShown();
    };
    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto polarCoordsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowPolarCoords();
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
    auto ratsnestShownCondition = [ this ] ( const SELECTION& aSel ) {
        return GetBoard()->IsElementVisible( LAYER_RATSNEST );
    };
    auto curvedRatsnestCondition = [ this ] ( const SELECTION& aSel ) {
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayRatsnestLinesCurved;
    };
    auto boardFlippedCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalCanvas()->GetView()->IsMirroredX();
    };
    auto zonesFilledCondition = [ this ] ( const SELECTION& aSel ) {
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayZonesMode == 0;
    };
    auto zonesWireframedCondition = [ this ] ( const SELECTION& aSel ) {
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayZonesMode == 1;
    };
    auto zonesOutlinedCondition = [ this ] ( const SELECTION& aSel ) {
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayZonesMode == 2;
    };
    auto sketchTracksCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPcbTrackFill;
    };
    auto sketchViasCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayViaFill;
    };
    auto sketchPadsCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPadFill;
    };
    auto contrastModeCondition = [ this ] ( const SELECTION& aSel ) {
        return !( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_ContrastModeDisplay;
    };

    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_MANAGE_LAYERS_VERTICAL_TOOLBAR,
                            _( "Show La&yers Manager" ), HELP_SHOW_HIDE_LAYERMANAGER,
                            layers_manager_xpm,             layersPaletteShownCondition );

    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR_MICROWAVE,
                            _( "Show Microwa&ve Toolbar" ), HELP_SHOW_HIDE_MICROWAVE_TOOLS,
                             mw_toolbar_xpm,                microwaveToolbarShownCondition );

    viewMenu->AddItem( ID_OPEN_MODULE_VIEWER,
                       _( "Footprint &Library Browser" ), _( "Browse footprint libraries" ),
                       modview_icon_xpm,                    SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddItem( ID_MENU_PCB_SHOW_3D_FRAME,
                       AddHotkeyName( _( "&3D Viewer" ), g_Board_Editor_Hotkeys_Descr, HK_3D_VIEWER ),
                       _( "Show board in 3D viewer" ),
                       three_d_xpm,                         SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,               SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,              SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,              SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                 SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AppendSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,            gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,             SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddCheckItem( PCB_ACTIONS::togglePolarCoords, polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,    imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,      metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,    fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( PCB_ACTIONS::showRatsnest,     ratsnestShownCondition );
    viewMenu->AddCheckItem( PCB_ACTIONS::ratsnestLineMode, curvedRatsnestCondition );

    viewMenu->AddSeparator();
    // Drawing Mode Submenu
    CONDITIONAL_MENU* drawingModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( add_zone_xpm );

    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::zoneDisplayEnable,   zonesFilledCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::zoneDisplayDisable,  zonesWireframedCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::zoneDisplayOutlines, zonesOutlinedCondition );

    drawingModeSubMenu->AddSeparator();
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::padDisplayMode,      sketchPadsCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::viaDisplayMode,      sketchViasCondition );
    drawingModeSubMenu->AddCheckItem( PCB_ACTIONS::trackDisplayMode,    sketchTracksCondition );
    viewMenu->AddMenu( drawingModeSubMenu );

    // Contrast Mode Submenu
    CONDITIONAL_MENU* contrastModeSubMenu = new CONDITIONAL_MENU( false, selTool );
    contrastModeSubMenu->SetTitle( _( "&Contrast Mode" ) );
    contrastModeSubMenu->SetIcon( contrast_mode_xpm );

    contrastModeSubMenu->AddCheckItem( PCB_ACTIONS::highContrastMode,   contrastModeCondition );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec, SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc, SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddMenu( contrastModeSubMenu );

    viewMenu->AddCheckItem( ID_MENU_PCB_FLIP_VIEW,
                            _( "Flip &Board View" ), _( "Flip (mirror) the board view" ),
                            flip_board_xpm, boardFlippedCondition );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

    //-- Place Menu ----------------------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( PCB_ACTIONS::placeModule,     SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawVia,         SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawZone,        SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawZoneKeepout, SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::placeText,       SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawArc,         SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawCircle,      SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawLine,        SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawPolygon,     SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::drawDimension,   SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::placeTarget,     SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::drillOrigin,     SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( ACTIONS::gridSetOrigin,       SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();

    ACTION_MENU* autoplaceSubmenu = new ACTION_MENU;
    autoplaceSubmenu->SetTitle( _( "Auto-Place Footprints" ) );
    autoplaceSubmenu->SetTool( selTool );
    autoplaceSubmenu->SetIcon( mode_module_xpm );

    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceOffboardComponents );
    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceSelectedComponents );

    placeMenu->AddMenu( autoplaceSubmenu );

    //-- Route Menu ----------------------------------------------------------
    //
    wxMenu* routeMenu = new wxMenu;
    prepareRouteMenu( routeMenu );

    //-- Inspect Menu --------------------------------------------------------
    //
    wxMenu* inspectMenu = new wxMenu;

    AddMenuItem( inspectMenu, ID_MENU_LIST_NETS,
                 _( "&List Nets" ),
                 _( "View list of nets with names and IDs" ),
                 KiBitmap( list_nets_xpm ) );

    AddMenuItem( inspectMenu, ID_PCB_MEASUREMENT_TOOL,
                 AddHotkeyName( _( "&Measure" ), g_Board_Editor_Hotkeys_Descr, HK_MEASURE_TOOL ),
                 _( "Measure distance" ),
                 KiBitmap( measurement_xpm ) );

    inspectMenu->AppendSeparator();
    AddMenuItem( inspectMenu, ID_DRC_CONTROL,
                 _( "&Design Rules Checker" ),
                 _( "Perform design rules check" ),
                 KiBitmap( erc_xpm ) );

    //-- Tools menu ----------------------------------------------------------
    //
    wxMenu* toolsMenu = new wxMenu;
    prepareToolsMenu( toolsMenu );

    //-- Preferences menu ----------------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, selTool );

    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetGalCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    prefsMenu->AddItem( ID_PREFERENCES_CONFIGURE_PATHS, _( "&Configure Paths..." ),
                        _( "Edit path configuration environment variables" ),
                        path_xpm,            SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddItem( ID_PCB_LIB_TABLE_EDIT, _( "Manage &Footprint Libraries..." ),
                        _( "Edit the global and project footprint library tables." ),
                        library_table_xpm,   SELECTION_CONDITIONS::ShowAlways );

#ifdef BUILD_GITHUB_PLUGIN
    prefsMenu->AddItem( ID_PCB_3DSHAPELIB_WIZARD, _( "Add &3D Shapes Libraries Wizard..." ),
                        _( "Download 3D shape libraries from GitHub" ),
                        import3d_xpm,        SELECTION_CONDITIONS::ShowAlways );
#endif
    prefsMenu->AddItem( wxID_PREFERENCES,
                        AddHotkeyName( _( "&Preferences..." ), g_Module_Editor_Hotkeys_Descr, HK_PREFERENCES ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,      SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    Pgm().AddMenuLanguageList( prefsMenu );

    prefsMenu->AddSeparator();
    prefsMenu->AddCheckItem( ACTIONS::acceleratedGraphics, acceleratedGraphicsCondition );
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics, standardGraphicsCondition );

    //--MenuBar -----------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( routeMenu, _( "Ro&ute" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( prefsMenu, _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    // Populate the Action Plugin sub-menu
    RebuildActionPluginMenus();
#endif

}


// Build the route menu
void prepareRouteMenu( wxMenu* aParentMenu )
{
    wxString text;

    AddMenuItem( aParentMenu, ID_AUX_TOOLBAR_PCB_SELECT_LAYER_PAIR,
                 _( "Set &Layer Pair..." ), _( "Change active layer pair" ),
                 KiBitmap( select_layer_pair_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Single Track" ), g_Board_Editor_Hotkeys_Descr,
                          HK_ADD_NEW_TRACK, IS_ACCELERATOR );
    AddMenuItem( aParentMenu, ID_TRACK_BUTT, text,
                 _( "Interactively route single track" ),
                 KiBitmap( add_tracks_xpm ) );

    text = AddHotkeyName( _( "&Differential Pair" ), g_Board_Editor_Hotkeys_Descr,
                          HK_ROUTE_DIFF_PAIR, IS_ACCELERATOR );
    AddMenuItem( aParentMenu, ID_DIFF_PAIR_BUTT, text,
                 _( "Interactively route differential pair" ),
                 KiBitmap( ps_diff_pair_xpm ) );

    aParentMenu->AppendSeparator();

    text = AddHotkeyName( _( "&Tune Track Length" ), g_Board_Editor_Hotkeys_Descr,
                          HK_ROUTE_TUNE_SINGLE, IS_ACCELERATOR );
    AddMenuItem( aParentMenu, ID_TUNE_SINGLE_TRACK_LEN_BUTT, text,
                 _( "Tune length of single track" ),
                 KiBitmap( ps_tune_length_xpm ) );

    text = AddHotkeyName( _( "Tune Differential Pair &Length" ), g_Board_Editor_Hotkeys_Descr,
                          HK_ROUTE_TUNE_DIFF_PAIR, IS_ACCELERATOR );
    AddMenuItem( aParentMenu, ID_TUNE_DIFF_PAIR_LEN_BUTT, text,
                 _( "Tune length of differential pair" ),
                 KiBitmap( ps_diff_pair_tune_length_xpm ) );

    text = AddHotkeyName( _( "Tune Differential Pair S&kew/Phase" ), g_Board_Editor_Hotkeys_Descr,
                          HK_ROUTE_TUNE_SKEW, IS_ACCELERATOR );
    AddMenuItem( aParentMenu, ID_TUNE_DIFF_PAIR_SKEW_BUTT, text,
                 _( "Tune skew/phase of a differential pair" ),
                 KiBitmap( ps_diff_pair_tune_phase_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_MENU_INTERACTIVE_ROUTER_SETTINGS,
                 _( "&Interactive Router Settings..." ),
                 _( "Configure interactive router" ),
                 KiBitmap( tools_xpm ) );
}


// Build the tools menu
void prepareToolsMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu,
                 ID_UPDATE_PCB_FROM_SCH,
                 _( "Update &PCB from Schematic..." ),
                 _( "Update PCB design with current schematic (forward annotation)" ),
                 KiBitmap( update_pcb_from_sch_xpm ) );

    AddMenuItem( aParentMenu, ID_MENU_PCB_UPDATE_FOOTPRINTS,
                 _( "Update &Footprints from Library..." ),
                 _( "Update footprints to include any changes from the library" ),
                 KiBitmap( reload_xpm ) );

    bool needsSeparator = true;

#if defined(KICAD_SCRIPTING_WXPYTHON)
    if( needsSeparator )
    {
        aParentMenu->AppendSeparator();
        needsSeparator = false;
    }

    AddMenuItem( aParentMenu, ID_TOOLBARH_PCB_SCRIPTING_CONSOLE,
                 _( "&Scripting Console" ),
                 _( "Show/Hide the Python scripting console" ),
                 KiBitmap( py_script_xpm ) );
#endif

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    if( needsSeparator )
    {
        aParentMenu->AppendSeparator();
        needsSeparator = false;
    }

    wxMenu* submenuActionPluginsMenu = new wxMenu();

    AddMenuItem( aParentMenu, submenuActionPluginsMenu, ID_TOOLBARH_PCB_ACTION_PLUGIN,
                 _( "&External Plugins..." ),
                 _( "Execute or reload python action plugins" ),
                 KiBitmap( hammer_xpm ) );

    AddMenuItem( submenuActionPluginsMenu, ID_TOOLBARH_PCB_ACTION_PLUGIN_REFRESH,
                 _( "&Refresh Plugins" ),
                 _( "Reload all python plugins and refresh plugin menus" ),
                 KiBitmap( reload_xpm ) );

    submenuActionPluginsMenu->AppendSeparator();
#endif
}
