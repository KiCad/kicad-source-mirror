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


// Build the files menu. Because some commands are available only if
// Pcbnew is run outside a project (run alone), aIsOutsideProject is false
// when Pcbnew is run from Kicad manager, and true is run as stand alone app.
static void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject );

// Build the export submenu (inside files menu)
static void prepareExportMenu( wxMenu* aParentMenu );

// Build the place submenu
static void preparePlaceMenu( wxMenu* aParentMenu );

// Build the route menu
static void prepareRouteMenu( wxMenu* aParentMenu );

// Build the inspect menu
static void prepareInspectMenu( wxMenu* aParentMenu );

// Build the library management menu
static void prepareLibraryMenu( wxMenu* aParentMenu );

// Build the preferences menu
static void preparePreferencesMenu( PCB_EDIT_FRAME* aFrame, wxMenu* aParentMenu );

// Build the tools menu
static void prepareToolsMenu( wxMenu* aParentMenu );

// Build the help menu
static void prepareHelpMenu( wxMenu* aParentMenu );


void PCB_EDIT_FRAME::ReCreateMenuBar()
{
    SELECTION_TOOL* selTool = m_toolManager->GetTool<SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();
    wxString   text;

    // Recreate all menus:

    // Create File Menu
    wxMenu* filesMenu = new wxMenu;
    prepareFilesMenu( filesMenu, Kiface().IsSingle() );

    //----- Edit menu -----------------------------------------------------------
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
                       _( "Edit &Track && Via Properties..." ), "",
                       width_track_via_xpm,             SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_EDIT_TEXT_AND_GRAPHICS,
                       _( "Edit Text && &Graphic Properties..." ), "",
                       reset_text_xpm,                  SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::exchangeFootprints, SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_SWAP_LAYERS,
                       _( "&Swap Layers..." ),
                       _( "Move tracks or drawings from a layer to another layer" ),
                       swap_layer_xpm,                  SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::zoneFillAll,        SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::zoneUnfillAll,      SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ID_PCB_GLOBAL_DELETE,
                       _( "Glo&bal Deletions..." ),
                       _( "Delete tracks, footprints and graphic items from board" ),
                       general_deletions_xpm,           SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( ID_MENU_PCB_CLEAN,
                       _( "C&leanup Tracks and Vias..." ),
                       _( "Clean stubs, vias, delete break points or unconnected tracks" ),
                       delete_xpm,                      SELECTION_CONDITIONS::ShowAlways );

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
        return ( (PCB_DISPLAY_OPTIONS*) GetDisplayOptions() )->m_DisplayPolarCood;
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
                            layers_manager_xpm, layersPaletteShownCondition );
    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_EXTRA_VERTICAL_TOOLBAR_MICROWAVE,
                            _( "Show Microwa&ve Toolbar" ), HELP_SHOW_HIDE_MICROWAVE_TOOLS,
                             mw_toolbar_xpm, microwaveToolbarShownCondition );
    viewMenu->AddItem( ID_OPEN_MODULE_VIEWER,
                       _( "Footprint &Library Browser" ), _( "Browse footprint libraries" ),
                       modview_icon_xpm, SELECTION_CONDITIONS::ShowAlways );
    text = AddHotkeyName( _( "&3D Viewer" ), g_Board_Editor_Hotkeys_Descr, HK_3D_VIEWER );
    viewMenu->AddItem( ID_MENU_PCB_SHOW_3D_FRAME,
                       text, _( "Show board in 3D viewer" ),
                       three_d_xpm, SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,    SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,   SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,        SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,      SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AppendSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid, gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,  SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddCheckItem( ID_TB_OPTIONS_SHOW_POLAR_COORD,
                            _( "Display &Polar Coordinates" ), wxEmptyString,
                            polar_coord_xpm, polarCoordsCondition );

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

    contrastModeSubMenu->AddSeparator();
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec, SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc, SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddMenu( contrastModeSubMenu );

    viewMenu->AddCheckItem( ID_MENU_PCB_FLIP_VIEW,
                            _( "Flip &Board View" ), _( "Flip (mirror) the board view" ),
                            flip_board_xpm, boardFlippedCondition );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

    //----- Place Menu ----------------------------------------------------------
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

    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceOffboardComponents );
    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceSelectedComponents );

    placeMenu->AddMenu( autoplaceSubmenu );

    //----- Route Menu ----------------------------------------------------------
    wxMenu* routeMenu = new wxMenu;
    prepareRouteMenu( routeMenu );

    //----- Inspect Menu --------------------------------------------------------
    wxMenu* inspectMenu = new wxMenu;
    prepareInspectMenu( inspectMenu );

    //----- Tools menu ----------------------------------------------------------
    wxMenu* toolsMenu = new wxMenu;
    prepareToolsMenu( toolsMenu );

    //----- Preferences and configuration menu ----------------------------------
    wxMenu* configmenu = new wxMenu;
    prepareLibraryMenu( configmenu );
    configmenu->AppendSeparator();

    preparePreferencesMenu( this, configmenu );

    //------ Help menu ----------------------------------------------------------
    wxMenu* helpMenu = new wxMenu;
    prepareHelpMenu( helpMenu );

    // Append all menus to the menuBar
    menuBar->Append( filesMenu, _( "&File" ) );
    menuBar->Append( editMenu, _( "&Edit" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    menuBar->Append( placeMenu, _( "&Place" ) );
    menuBar->Append( routeMenu, _( "Ro&ute" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu, _( "&Tools" ) );
    menuBar->Append( configmenu, _( "P&references" ) );
    menuBar->Append( helpMenu, _( "&Help" ) );

    SetMenuBar( menuBar );
    delete oldMenuBar;

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    // Populate the Action Plugin sub-menu
    RebuildActionPluginMenus();
#endif

}

// Build the preferences menu
void preparePreferencesMenu( PCB_EDIT_FRAME* aFrame, wxMenu* aParentMenu )
{

    wxString text;

    text = AddHotkeyName( _( "&Preferences..." ), g_Board_Editor_Hotkeys_Descr, HK_PREFERENCES );
    AddMenuItem( aParentMenu, wxID_PREFERENCES, text,
                 _( "Show preferences for all open tools" ),
                 KiBitmap( preference_xpm ) );

    if( ADVANCED_CFG::GetCfg().AllowLegacyCanvas() )
    {
        text = AddHotkeyName(
                _( "Legacy Tool&set" ), g_Board_Editor_Hotkeys_Descr, HK_CANVAS_LEGACY );
        AddMenuItem( aParentMenu, ID_MENU_CANVAS_LEGACY, text,
                _( "Use Legacy Toolset (not all features will be available)" ),
                KiBitmap( tools_xpm ), wxITEM_RADIO );
    }

    text = AddHotkeyName( _( "Modern Toolset (&Accelerated)" ), g_Board_Editor_Hotkeys_Descr,
                          HK_CANVAS_OPENGL );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_OPENGL, text,
                 _( "Use Modern Toolset with hardware-accelerated graphics (recommended)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    text = AddHotkeyName( _( "Modern Toolset (Fallbac&k)" ), g_Board_Editor_Hotkeys_Descr,
                          HK_CANVAS_CAIRO );
    AddMenuItem( aParentMenu, ID_MENU_CANVAS_CAIRO, text,
                 _( "Use Modern Toolset with software graphics (fall-back)" ),
                 KiBitmap( tools_xpm ), wxITEM_RADIO );

    aParentMenu->AppendSeparator();

    // Language submenu
    Pgm().AddMenuLanguageList( aParentMenu );
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


// Build the inspect menu
void prepareInspectMenu( wxMenu* aParentMenu )
{
    wxString text;

    AddMenuItem( aParentMenu, ID_MENU_LIST_NETS,
                 _( "&List Nets" ),
                 _( "View list of nets with names and IDs" ),
                 KiBitmap( list_nets_xpm ) );

    text = AddHotkeyName( _( "&Measure" ), g_Board_Editor_Hotkeys_Descr, HK_MEASURE_TOOL );
    AddMenuItem( aParentMenu, ID_PCB_MEASUREMENT_TOOL, text,
                 _( "Measure distance" ),
                 KiBitmap( measurement_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_DRC_CONTROL,
                 _( "&Design Rules Checker" ),
                 _( "Perform design rules check" ),
                 KiBitmap( erc_xpm ) );
}


// Build the library management menu
void prepareLibraryMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu,
                 ID_PREFERENCES_CONFIGURE_PATHS,
                 _( "&Configure Paths..." ),
                 _( "Edit path configuration environment variables" ),
                 KiBitmap( path_xpm ) );

    AddMenuItem( aParentMenu, ID_PCB_LIB_TABLE_EDIT,
                _( "Manage &Footprint Libraries..." ),
                _( "Edit the global and project footprint library lists" ),
                KiBitmap( library_table_xpm ) );

#ifdef BUILD_GITHUB_PLUGIN
    AddMenuItem( aParentMenu, ID_PCB_3DSHAPELIB_WIZARD,
                 _( "Add &3D Shapes Libraries Wizard..." ),
                 _( "Download 3D shape libraries from GitHub" ),
                 KiBitmap( import3d_xpm ) );
#endif
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


// Build the help menu
void prepareHelpMenu( wxMenu* aParentMenu )
{

    AddMenuItem( aParentMenu, wxID_HELP,
                 _( "Pcbnew &Manual" ),
                 _( "Open Pcbnew Manual" ),
                 KiBitmap( online_help_xpm ) );

    AddMenuItem( aParentMenu, wxID_INDEX,
                 _( "&Getting Started in KiCad" ),
                 _( "Open \"Getting Started in KiCad\" guide for beginners" ),
                 KiBitmap( help_xpm ) );

    wxString text = AddHotkeyName( _( "&List Hotkeys..." ), g_Board_Editor_Hotkeys_Descr, HK_HELP );
    AddMenuItem( aParentMenu, ID_PREFERENCES_HOTKEY_SHOW_CURRENT_LIST,
                 text,
                 _( "Display current hotkeys list and corresponding commands" ),
                 KiBitmap( hotkeys_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_HELP_GET_INVOLVED,
                 _( "Get &Involved" ),
                 _( "Contribute to KiCad (opens a web browser)" ),
                 KiBitmap( info_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, wxID_ABOUT, _( "&About KiCad" ), KiBitmap( about_xpm ) );
}


// Build the files menu.
void prepareFilesMenu( wxMenu* aParentMenu, bool aIsOutsideProject )
{
    wxString text;

    // Some commands are available only if Pcbnew is run outside a project (run alone).
    // aIsOutsideProject is false when Pcbnew is run from Kicad manager.

    FILE_HISTORY&  fhist = Kiface().GetFileHistory();

    // Load Recent submenu
    static wxMenu* openRecentMenu;

    // Add this menu to list menu managed by m_fileHistory
    // (the file history will be updated when adding/removing files in history
    if( openRecentMenu )
        fhist.RemoveMenu( openRecentMenu );

    openRecentMenu = new wxMenu();

    fhist.UseMenu( openRecentMenu );
    fhist.AddFilesToMenu();

    if( aIsOutsideProject )
    {
        text = AddHotkeyName( _( "&New" ), g_Board_Editor_Hotkeys_Descr, HK_NEW );
        AddMenuItem( aParentMenu, ID_NEW_BOARD,
                     text, _( "Create new board" ),
                     KiBitmap( new_board_xpm ) );

        text = AddHotkeyName( _( "&Open..." ), g_Board_Editor_Hotkeys_Descr, HK_OPEN );
        AddMenuItem( aParentMenu, ID_LOAD_FILE, text,
                     _( "Open existing board" ),
                     KiBitmap( open_brd_file_xpm ) );

        AddMenuItem( aParentMenu, openRecentMenu,
                     -1, _( "Open &Recent" ),
                     _( "Open recently opened board" ),
                     KiBitmap( recent_xpm ) );

        aParentMenu->AppendSeparator();
    }

    text = AddHotkeyName( _( "&Save" ), g_Board_Editor_Hotkeys_Descr, HK_SAVE );
    AddMenuItem( aParentMenu, ID_SAVE_BOARD, text,
                 _( "Save current board" ),
                 KiBitmap( save_xpm ) );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    // when not under a project mgr, we are free to change filenames, cwd ...
    if( Kiface().IsSingle() )      // not when under a project mgr (pcbnew is run as stand alone)
    {
        text = AddHotkeyName( _( "Sa&ve As..." ), g_Board_Editor_Hotkeys_Descr, HK_SAVEAS );
        AddMenuItem( aParentMenu, ID_SAVE_BOARD_AS, text,
                     _( "Save current board with new name" ),
                     KiBitmap( save_as_xpm ) );
    }
    // under a project mgr, we can save a copy of the board,
    // but do not change the current board file name
    else
    {
        text = AddHotkeyName( _( "Sa&ve Copy As..." ), g_Board_Editor_Hotkeys_Descr, HK_SAVEAS );
        AddMenuItem( aParentMenu, ID_COPY_BOARD_AS, text,
                     _( "Save copy of the current board" ),
                     KiBitmap( save_as_xpm ) );
    }

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_MENU_RECOVER_BOARD_AUTOSAVE,
                 _( "Resc&ue" ),
                 _( "Clear board and get last rescue file automatically saved by Pcbnew" ),
                 KiBitmap( rescue_xpm ) );

    if( aIsOutsideProject )
    {
        AddMenuItem( aParentMenu, ID_APPEND_FILE,
                     _( "&Append Board..." ),
                     _( "Append another board to currently loaded board" ),
                     KiBitmap( add_board_xpm ) );

        AddMenuItem( aParentMenu, ID_IMPORT_NON_KICAD_BOARD,
                     _( "Import Non-KiCad Board File..." ),
                     _( "Import board file from other applications" ),
                     KiBitmap( import_brd_file_xpm ) );
    }

    AddMenuItem( aParentMenu, ID_MENU_READ_BOARD_BACKUP_FILE,
                 _( "Revert to Las&t Backup" ),
                 _( "Clear board and get previous backup version of board" ),
                 KiBitmap( undo_xpm ) );

    aParentMenu->AppendSeparator();

    //----- Import submenu ------------------------------------------------------
    wxMenu* submenuImport = new wxMenu();

    AddMenuItem( submenuImport, ID_GET_NETLIST,
                 _( "&Netlist..." ),
                 _( "Read netlist and update board connectivity" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( submenuImport, ID_GEN_IMPORT_SPECCTRA_SESSION,
                 _( "&Specctra Session..." ),
                 _( "Import routed \"Specctra Session\" (*.ses) file" ),
                 KiBitmap( import_xpm ) );

    AddMenuItem( submenuImport, ID_GEN_IMPORT_GRAPHICS_FILE,
                 _( "&Graphics..." ),
                 _( "Import 2D Drawing file to Pcbnew on Drawings layer" ),
                 KiBitmap( import_vector_xpm ) );

    AddMenuItem( aParentMenu, submenuImport,
                 ID_GEN_IMPORT_FILE, _( "&Import" ),
                 _( "Import files" ), KiBitmap( import_xpm ) );


    //----- Export submenu ------------------------------------------------------
    wxMenu* submenuexport = new wxMenu();
    prepareExportMenu( submenuexport );

    AddMenuItem( aParentMenu, submenuexport,
                 ID_GEN_EXPORT_FILE, _( "E&xport" ),
                 _( "Export board" ), KiBitmap( export_xpm ) );


    //----- Fabrication Outputs submenu -----------------------------------------
    wxMenu* fabricationOutputsMenu = new wxMenu;
    AddMenuItem( fabricationOutputsMenu, ID_GEN_PLOT_GERBER,
                 _( "&Gerbers (.gbr)..." ),
                 _( "Generate Gerbers for fabrication" ),
                 KiBitmap( post_compo_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_DRILL_FILE,
                 _( "&Drill Files (.drl)..." ),
                 _( "Generate Excellon drill file(s)" ),
                 KiBitmap( post_drill_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_POS_MODULES_FILE,
                 _( "Footprint &Positions (.pos)..." ),
                 _( "Generate footprint position file for pick and place" ),
                 KiBitmap( post_compo_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_GEN_EXPORT_FILE_MODULE_REPORT,
                 _( "&Footprint Report (.rpt)..." ),
                 _( "Create report of all footprints from current board" ),
                 KiBitmap( tools_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_D356_FILE,
                 _( "IPC-D-356 Netlist File..." ),
                 _( "Generate IPC-D-356 netlist file" ),
                 KiBitmap( netlist_xpm ) );

    AddMenuItem( fabricationOutputsMenu, ID_PCB_GEN_BOM_FILE_FROM_BOARD,
                 _( "&BOM..." ),
                 _( "Create bill of materials from current schematic" ),
                 KiBitmap( bom_xpm ) );

    AddMenuItem( aParentMenu, fabricationOutputsMenu,
                 -1, _( "&Fabrication Outputs" ),
                 _( "Generate files for fabrication" ),
                 KiBitmap( fabrication_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_BOARD_SETUP_DIALOG,
                 _( "&Board Setup..." ),
                 _( "Edit board setup including layers, design rules and various defaults" ),
                 KiBitmap( options_board_xpm ) );

    aParentMenu->AppendSeparator();

    AddMenuItem( aParentMenu, ID_SHEET_SET,
                 _( "Page S&ettings..." ),
                 _( "Settings for sheet size and frame references" ),
                 KiBitmap( sheetset_xpm ) );

    text = AddHotkeyName( _( "&Print..." ), g_Board_Editor_Hotkeys_Descr, HK_PRINT );
    AddMenuItem( aParentMenu, wxID_PRINT, text,
                 _( "Print board" ),
                 KiBitmap( print_button_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_PLOT,
                 _( "P&lot..." ),
                 _( "Plot board in HPGL, PostScript or Gerber RS-274X format)" ),
                 KiBitmap( plot_xpm ) );

    aParentMenu->AppendSeparator();

    //----- archive submenu -----------------------------------------------------
    wxMenu* submenuarchive = new wxMenu();

    AddMenuItem( submenuarchive, ID_MENU_ARCHIVE_MODULES_IN_LIBRARY,
                 _( "&Archive Footprints in Existing Library..." ),
                 _( "Archive all footprints to existing library in footprint Lib table"
                    "(does not remove other footprints in this library)" ),
                 KiBitmap( library_archive_xpm ) );

    AddMenuItem( submenuarchive, ID_MENU_CREATE_LIBRARY_AND_ARCHIVE_MODULES,
                 _( "&Create New Library and Archive Footprints..." ),
                 _( "Archive all footprints to a new library\n"
                    "(if the library already exists it will be replaced)" ),
                 KiBitmap( library_archive_as_xpm ) );

    AddMenuItem( aParentMenu, submenuarchive,
                 ID_MENU_ARCHIVE_MODULES,
                 _( "Arc&hive Footprints" ),
                 _( "Archive or add all footprints in library file" ),
                 KiBitmap( library_archive_xpm ) );

    aParentMenu->AppendSeparator();
    AddMenuItem( aParentMenu, wxID_EXIT, _( "&Exit" ), _( "Close Pcbnew" ), KiBitmap( exit_xpm ) );
}


// Build the import/export submenu (inside files menu)
void prepareExportMenu( wxMenu* aParentMenu )
{
    AddMenuItem( aParentMenu, ID_GEN_EXPORT_SPECCTRA,
                 _( "S&pecctra DSN..." ),
                 _( "Export current board to \"Specctra DSN\" file" ),
                 KiBitmap( export_dsn_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_EXPORT_FILE_GENCADFORMAT,
                 _( "&GenCAD..." ), _( "Export GenCAD format" ),
                 KiBitmap( export_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_EXPORT_FILE_VRML,
                 _( "&VRML..." ),
                 _( "Export VRML board representation" ),
                 KiBitmap( export3d_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_EXPORT_FILE_IDF3,
                 _( "I&DFv3..." ), _( "IDFv3 board and symbol export" ),
                 KiBitmap( export_idf_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_EXPORT_FILE_STEP,
                 _( "S&TEP..." ), _( "STEP export" ),
                 KiBitmap( export_step_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_PLOT_SVG,
                 _( "&SVG..." ),
                 _( "Export board file in Scalable Vector Graphics format" ),
                 KiBitmap( plot_svg_xpm ) );

    AddMenuItem( aParentMenu, ID_PCB_GEN_CMP_FILE,
                 _( "&Footprint Association (.cmp) File..." ),
                 _( "Export footprint association file (*.cmp) for schematic back annotation" ),
                 KiBitmap( create_cmp_file_xpm ) );

    AddMenuItem( aParentMenu, ID_GEN_EXPORT_FILE_HYPERLYNX,
                 _( "&Hyperlynx..." ), _( "Hyperlynx export" ),
                 KiBitmap( export_step_xpm ) );

}
