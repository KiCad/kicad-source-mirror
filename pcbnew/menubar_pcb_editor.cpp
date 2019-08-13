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
#include "pcbnew.h"
#include "pcbnew_id.h"


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
    auto disp_opt = static_cast<PCB_DISPLAY_OPTIONS*>( GetDisplayOptions() );

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        // Add this menu to list menu managed by m_fileHistory
        // (the file history will be updated when adding/removing files in history)
        if( openRecentMenu )
            Kiface().GetFileHistory().RemoveMenu( openRecentMenu );

        openRecentMenu = new ACTION_MENU( false );
        openRecentMenu->SetTool( selTool );
        openRecentMenu->SetTitle( _( "Open Recent" ) );
        openRecentMenu->SetIcon( recent_xpm );

        Kiface().GetFileHistory().UseMenu( openRecentMenu );
        Kiface().GetFileHistory().AddFilesToMenu( openRecentMenu );

        fileMenu->AddItem( ACTIONS::doNew,           SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ACTIONS::open,            SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddMenu( openRecentMenu,           SELECTION_CONDITIONS::ShowAlways );

        fileMenu->AddItem( PCB_ACTIONS::appendBoard, SELECTION_CONDITIONS::ShowAlways );
        fileMenu->AddItem( ID_IMPORT_NON_KICAD_BOARD,
                           _( "Import Non-KiCad Board File..." ),
                           _( "Import board file from other applications" ),
                           import_brd_file_xpm,      SELECTION_CONDITIONS::ShowAlways );

        fileMenu->AddSeparator();
    }

    fileMenu->AddItem( ACTIONS::save,                modifiedDocumentCondition );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    if( Kiface().IsSingle() )
        fileMenu->AddItem( ACTIONS::saveAs,          SELECTION_CONDITIONS::ShowAlways );
    else
        fileMenu->AddItem( ACTIONS::saveCopyAs,      SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ID_MENU_RECOVER_BOARD_AUTOSAVE,
                       _( "Resc&ue" ),
                       _( "Clear board and get last rescue file automatically saved by Pcbnew" ),
                       rescue_xpm,                   SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddItem( ID_MENU_READ_BOARD_BACKUP_FILE,
                       _( "Revert to Last Backup" ),
                       _( "Clear board and get previous backup version of board" ),
                       undo_xpm,                     SELECTION_CONDITIONS::ShowAlways );

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false );
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );

    submenuImport->Add( PCB_ACTIONS::importNetlist );
    submenuImport->Add( PCB_ACTIONS::importSpecctraSession );
    submenuImport->Add( _( "Graphics..." ), _( "Import 2D drawing file" ),
                        ID_GEN_IMPORT_GRAPHICS_FILE, import_vector_xpm );

    fileMenu->AddSeparator();
    fileMenu->AddMenu( submenuImport,                SELECTION_CONDITIONS::ShowAlways );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );

    submenuExport->Add( PCB_ACTIONS::exportSpecctraDSN );
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
    ACTION_MENU* submenuFabOutputs = new ACTION_MENU( false );
    submenuFabOutputs->SetTool( selTool );
    submenuFabOutputs->SetTitle( _( "Fabrication Outputs" ) );
    submenuFabOutputs->SetIcon( fabrication_xpm );

    submenuFabOutputs->Add( PCB_ACTIONS::generateGerbers );
    submenuFabOutputs->Add( PCB_ACTIONS::generateDrillFiles );
    submenuFabOutputs->Add( PCB_ACTIONS::generatePosFile );
    submenuFabOutputs->Add( PCB_ACTIONS::generateReportFile );
    submenuFabOutputs->Add( PCB_ACTIONS::generateD356File );
    submenuFabOutputs->Add( PCB_ACTIONS::generateBOM );

    fileMenu->AddMenu( submenuFabOutputs,            SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( PCB_ACTIONS::boardSetup,      SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddItem( ACTIONS::pageSettings,        SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::print,               SELECTION_CONDITIONS::ShowAlways );
    fileMenu->AddItem( ACTIONS::plot,                SELECTION_CONDITIONS::ShowAlways );

    // Archive submenu
    ACTION_MENU* submenuArchive = new ACTION_MENU( false );
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
    fileMenu->AddMenu( submenuArchive,               SELECTION_CONDITIONS::ShowAlways );

    fileMenu->AddSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "Pcbnew" ) );

    fileMenu->Resolve();

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
        return ToolStackIsEmpty();
    };

    editMenu->AddItem( ACTIONS::undo,                       enableUndoCondition );
    editMenu->AddItem( ACTIONS::redo,                       enableRedoCondition );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::cut,                        SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::copy,                       SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::paste,                      noActiveToolCondition );
    editMenu->AddItem( ACTIONS::doDelete,                   SELECTION_CONDITIONS::NotEmpty );
    editMenu->AddItem( ACTIONS::duplicate,                  SELECTION_CONDITIONS::NotEmpty );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::find,                       SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::editTracksAndVias,      SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::editTextAndGraphics,    SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::changeFootprints,       SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::swapLayers,             SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( PCB_ACTIONS::zoneFillAll,            SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::zoneUnfillAll,          SELECTION_CONDITIONS::ShowAlways );

    editMenu->AddSeparator();
    editMenu->AddItem( ACTIONS::deleteTool,                 SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::globalDeletions,        SELECTION_CONDITIONS::ShowAlways );
    editMenu->AddItem( PCB_ACTIONS::cleanupTracksAndVias,   SELECTION_CONDITIONS::ShowAlways );

    editMenu->Resolve();

    //----- View menu -----------------------------------------------------------
    //
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
    auto ratsnestShownCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return disp_opt->m_ShowGlobalRatsnest;
    };
    auto curvedRatsnestCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return disp_opt->m_DisplayRatsnestLinesCurved;
    };
    auto boardFlippedCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetView()->IsMirroredX();
    };
    auto zonesFilledCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return disp_opt->m_DisplayZonesMode == 0;
    };
    auto zonesWireframedCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return disp_opt->m_DisplayZonesMode == 1;
    };
    auto zonesOutlinedCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return disp_opt->m_DisplayZonesMode == 2;
    };
    auto sketchTracksCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return !disp_opt->m_DisplayPcbTrackFill;
    };
    auto sketchViasCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return !disp_opt->m_DisplayViaFill;
    };
    auto sketchPadsCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return !disp_opt->m_DisplayPadFill;
    };
    auto contrastModeCondition = [ this, disp_opt ] ( const SELECTION& aSel ) {
        return !disp_opt->m_ContrastModeDisplay;
    };

    viewMenu->AddCheckItem( PCB_ACTIONS::showLayersManager,    layersPaletteShownCondition );
    viewMenu->AddCheckItem( PCB_ACTIONS::showMicrowaveToolbar, microwaveToolbarShownCondition );
    viewMenu->AddItem( ACTIONS::showFootprintBrowser,          SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::show3DViewer,                  SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddItem( ACTIONS::zoomInCenter,                  SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,                 SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,                 SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomTool,                      SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,                    SELECTION_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,               gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,                SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddCheckItem( PCB_ACTIONS::togglePolarCoords,    polarCoordsCondition );

    // Units submenu
    CONDITIONAL_MENU* unitsSubMenu = new CONDITIONAL_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->AddCheckItem( ACTIONS::imperialUnits,        imperialUnitsCondition );
    unitsSubMenu->AddCheckItem( ACTIONS::metricUnits,          metricUnitsCondition );
    viewMenu->AddMenu( unitsSubMenu );

    viewMenu->AddCheckItem( ACTIONS::toggleCursorStyle,        fullCrosshairCondition );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( PCB_ACTIONS::showRatsnest,         ratsnestShownCondition );
    viewMenu->AddCheckItem( PCB_ACTIONS::ratsnestLineMode,     curvedRatsnestCondition );

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

    contrastModeSubMenu->AddCheckItem( ACTIONS::highContrastMode,       contrastModeCondition );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaDec, SELECTION_CONDITIONS::ShowAlways );
    contrastModeSubMenu->AddItem( PCB_ACTIONS::layerAlphaInc, SELECTION_CONDITIONS::ShowAlways );
    viewMenu->AddMenu( contrastModeSubMenu );

    viewMenu->AddCheckItem( PCB_ACTIONS::flipBoard,                     boardFlippedCondition );

#ifdef __APPLE__
    viewMenu->AddSeparator();
#endif

    viewMenu->Resolve();

    //-- Place Menu ----------------------------------------------------------
    //
    CONDITIONAL_MENU* placeMenu = new CONDITIONAL_MENU( false, selTool );

    placeMenu->AddItem( PCB_ACTIONS::placeModule,      SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawVia,          SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawZone,         SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawZoneKeepout,  SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::placeText,        SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawArc,          SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawCircle,       SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawLine,         SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( PCB_ACTIONS::drawPolygon,      SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::drawDimension,    SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::placeTarget,      SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();
    placeMenu->AddItem( PCB_ACTIONS::drillOrigin,      SELECTION_CONDITIONS::ShowAlways );
    placeMenu->AddItem( ACTIONS::gridSetOrigin,        SELECTION_CONDITIONS::ShowAlways );

    placeMenu->AddSeparator();

    ACTION_MENU* autoplaceSubmenu = new ACTION_MENU( false );
    autoplaceSubmenu->SetTitle( _( "Auto-Place Footprints" ) );
    autoplaceSubmenu->SetTool( selTool );
    autoplaceSubmenu->SetIcon( mode_module_xpm );

    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceOffboardComponents );
    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceSelectedComponents );

    placeMenu->AddMenu( autoplaceSubmenu );

    placeMenu->Resolve();

    //-- Route Menu ----------------------------------------------------------
    //
    CONDITIONAL_MENU* routeMenu = new CONDITIONAL_MENU( false, selTool );

    routeMenu->AddItem( PCB_ACTIONS::selectLayerPair,        SELECTION_CONDITIONS::ShowAlways );

    routeMenu->AddSeparator();
    routeMenu->AddItem( PCB_ACTIONS::routeSingleTrack,       SELECTION_CONDITIONS::ShowAlways );
    routeMenu->AddItem( PCB_ACTIONS::routeDiffPair,          SELECTION_CONDITIONS::ShowAlways );

    routeMenu->AddSeparator();
    routeMenu->AddItem( PCB_ACTIONS::routerTuneSingleTrace,  SELECTION_CONDITIONS::ShowAlways );
    routeMenu->AddItem( PCB_ACTIONS::routerTuneDiffPair,     SELECTION_CONDITIONS::ShowAlways );
    routeMenu->AddItem( PCB_ACTIONS::routerTuneDiffPairSkew, SELECTION_CONDITIONS::ShowAlways );

    routeMenu->AddSeparator();
    routeMenu->AddItem( PCB_ACTIONS::routerSettingsDialog,   SELECTION_CONDITIONS::ShowAlways );

    routeMenu->Resolve();

    //-- Inspect Menu --------------------------------------------------------
    //
    CONDITIONAL_MENU* inspectMenu = new CONDITIONAL_MENU( false, selTool );

    inspectMenu->AddItem( PCB_ACTIONS::listNets,             SELECTION_CONDITIONS::ShowAlways );
    inspectMenu->AddItem( ACTIONS::measureTool,              SELECTION_CONDITIONS::ShowAlways );
    inspectMenu->AddItem( PCB_ACTIONS::boardStatistics,      SELECTION_CONDITIONS::ShowAlways );

    inspectMenu->AddSeparator();
    inspectMenu->AddItem( PCB_ACTIONS::runDRC,               SELECTION_CONDITIONS::ShowAlways );

    inspectMenu->Resolve();

    //-- Tools menu ----------------------------------------------------------
    //
    CONDITIONAL_MENU* toolsMenu = new CONDITIONAL_MENU( false, selTool );

    toolsMenu->AddItem( ACTIONS::updatePcbFromSchematic,     SELECTION_CONDITIONS::ShowAlways );
    toolsMenu->AddItem( PCB_ACTIONS::updateFootprints,       SELECTION_CONDITIONS::ShowAlways );

#if defined(KICAD_SCRIPTING_WXPYTHON)
    auto pythonConsoleShownCondition = [] ( const SELECTION& aSel ) {
        wxMiniFrame* pythonConsole = (wxMiniFrame *) PCB_EDIT_FRAME::findPythonConsole();
        return pythonConsole && pythonConsole->IsShown();
    };

    toolsMenu->AddSeparator();
    toolsMenu->AddCheckItem( PCB_ACTIONS::showPythonConsole,  pythonConsoleShownCondition );
#endif

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    ACTION_MENU* submenuActionPlugins = new ACTION_MENU( false );
    submenuActionPlugins->SetTool( selTool );
    submenuActionPlugins->SetTitle( _( "External Plugins" ) );
    submenuActionPlugins->SetIcon( hammer_xpm );

    submenuActionPlugins->Add( _( "Refresh Plugins" ),
                               _( "Reload all python plugins and refresh plugin menus" ),
                               ID_TOOLBARH_PCB_ACTION_PLUGIN_REFRESH, reload_xpm );
    submenuActionPlugins->AppendSeparator();

    toolsMenu->AddSeparator();
    toolsMenu->AddMenu( submenuActionPlugins,                 SELECTION_CONDITIONS::ShowAlways );
#endif

    toolsMenu->Resolve();

    //-- Preferences menu ----------------------------------------------------
    //
    CONDITIONAL_MENU* prefsMenu = new CONDITIONAL_MENU( false, selTool );

    auto acceleratedGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL;
    };
    auto standardGraphicsCondition = [ this ] ( const SELECTION& aSel ) {
        return GetCanvas()->GetBackend() == EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO;
    };

    prefsMenu->AddItem( ACTIONS::configurePaths,              SELECTION_CONDITIONS::ShowAlways );
    prefsMenu->AddItem( ACTIONS::showFootprintLibTable,       SELECTION_CONDITIONS::ShowAlways );

#ifdef BUILD_GITHUB_PLUGIN
    prefsMenu->AddItem( ID_PCB_3DSHAPELIB_WIZARD,
                        _( "Add &3D Shapes Libraries Wizard..." ),
                        _( "Download 3D shape libraries from GitHub" ),
                        import3d_xpm,                         SELECTION_CONDITIONS::ShowAlways );
#endif
    prefsMenu->AddItem( wxID_PREFERENCES,
                        _( "Preferences...\tCTRL+," ),
                        _( "Show preferences for all open tools" ),
                        preference_xpm,                       SELECTION_CONDITIONS::ShowAlways );

    prefsMenu->AddSeparator();
    AddMenuLanguageList( prefsMenu, selTool );

    prefsMenu->AddSeparator();
    prefsMenu->AddCheckItem( ACTIONS::acceleratedGraphics, acceleratedGraphicsCondition );
    prefsMenu->AddCheckItem( ACTIONS::standardGraphics, standardGraphicsCondition );

    prefsMenu->Resolve();

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
    buildActionPluginMenus( submenuActionPlugins );
#endif
}
