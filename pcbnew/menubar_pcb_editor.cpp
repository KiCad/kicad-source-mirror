/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <advanced_config.h>
#include <bitmaps.h>
#include <file_history.h>
#include <kiface_base.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <python_scripting.h>
#include <tool/action_manager.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/wx_menubar.h>


void PCB_EDIT_FRAME::doReCreateMenuBar()
{
    PCB_SELECTION_TOOL* selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    // Recreate all menus:

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU*   fileMenu = new ACTION_MENU( false, selTool );
    static ACTION_MENU* openRecentMenu;

    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        FILE_HISTORY& fileHistory = GetFileHistory();

        // Create the menu if it does not exist. Adding a file to/from the history
        // will automatically refresh the menu.
        if( !openRecentMenu )
        {
            openRecentMenu = new ACTION_MENU( false, selTool );
            openRecentMenu->SetIcon( BITMAPS::recent );

            fileHistory.UseMenu( openRecentMenu );
            fileHistory.AddFilesToMenu();
        }

        // Ensure the title is up to date after changing language
        openRecentMenu->SetTitle( _( "Open Recent" ) );
        fileHistory.UpdateClearText( openRecentMenu, _( "Clear Recent Files" ) );

        fileMenu->Add( ACTIONS::doNew );
        fileMenu->Add( ACTIONS::open );

        wxMenuItem* item = fileMenu->Add( openRecentMenu->Clone() );

        // Add the file menu condition here since it needs the item ID for the submenu
        ACTION_CONDITIONS cond;
        cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( fileHistory ) );
        RegisterUIUpdateHandler( item->GetId(), cond );
    }

    fileMenu->Add( PCB_ACTIONS::appendBoard );
    fileMenu->AppendSeparator();

    fileMenu->Add( ACTIONS::save );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    if( Kiface().IsSingle() )
        fileMenu->Add( ACTIONS::saveAs );
    else
        fileMenu->Add( ACTIONS::saveCopy );

    fileMenu->Add( ACTIONS::revert );

    fileMenu->AppendSeparator();
    fileMenu->Add( PCB_ACTIONS::rescueAutosave );

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false, selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( BITMAPS::import );

    submenuImport->Add( PCB_ACTIONS::importNetlist,          ACTION_MENU::NORMAL, _( "Netlist..." ) );
    submenuImport->Add( PCB_ACTIONS::importSpecctraSession,  ACTION_MENU::NORMAL, _( "Specctra Session..." ) );
    submenuImport->Add( PCB_ACTIONS::placeImportedGraphics,  ACTION_MENU::NORMAL, _( "Graphics..." ) );
    submenuImport->Add( PCB_ACTIONS::openNonKicadBoard );

    fileMenu->AppendSeparator();
    fileMenu->Add( submenuImport );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( BITMAPS::export_file );

    submenuExport->Add( PCB_ACTIONS::exportSpecctraDSN, ACTION_MENU::NORMAL, _( "Specctra DSN..." ) );
    submenuExport->Add( PCB_ACTIONS::exportGenCAD,      ACTION_MENU::NORMAL, _( "GenCAD..." ) );
    submenuExport->Add( PCB_ACTIONS::exportVRML,        ACTION_MENU::NORMAL, _( "VRML..." ) );
    submenuExport->Add( PCB_ACTIONS::exportIDF,         ACTION_MENU::NORMAL, _( "IDFv3..." ) );
    submenuExport->Add( PCB_ACTIONS::exportSTEP,        ACTION_MENU::NORMAL, _( "STEP/GLB/BREP/XAO/PLY/STL..." ) );
    submenuExport->Add( PCB_ACTIONS::exportCmpFile,     ACTION_MENU::NORMAL, _( "Footprint Association (.cmp) File..." ) );
    submenuExport->Add( PCB_ACTIONS::exportHyperlynx,   ACTION_MENU::NORMAL, _( "Hyperlynx..." ) );

    if( ADVANCED_CFG::GetCfg().m_ShowPcbnewExportNetlist && m_exportNetlistAction )
        submenuExport->Add( *m_exportNetlistAction );

    submenuExport->AppendSeparator();
    submenuExport->Add( PCB_ACTIONS::exportFootprints,  ACTION_MENU::NORMAL, _( "Footprints..." ) );

    fileMenu->Add( submenuExport );

    // Fabrication Outputs submenu
    ACTION_MENU* submenuFabOutputs = new ACTION_MENU( false, selTool );
    submenuFabOutputs->SetTitle( _( "Fabrication Outputs" ) );
    submenuFabOutputs->SetIcon( BITMAPS::fabrication );

    submenuFabOutputs->Add( PCB_ACTIONS::generateGerbers );
    submenuFabOutputs->Add( PCB_ACTIONS::generateDrillFiles );
    submenuFabOutputs->Add( PCB_ACTIONS::generateIPC2581File );
    submenuFabOutputs->Add( PCB_ACTIONS::generateODBPPFile );

    submenuFabOutputs->Add( PCB_ACTIONS::generatePosFile );
    submenuFabOutputs->Add( PCB_ACTIONS::generateReportFile );
    submenuFabOutputs->Add( PCB_ACTIONS::generateD356File );
    submenuFabOutputs->Add( PCB_ACTIONS::generateBOM );
    fileMenu->Add( submenuFabOutputs );

    fileMenu->AppendSeparator();
    fileMenu->Add( PCB_ACTIONS::boardSetup );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::pageSettings );
    fileMenu->Add( ACTIONS::print );
    fileMenu->Add( ACTIONS::plot );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "PCB Editor" ) );

    //-- Edit menu -----------------------------------------------------------
    //
    ACTION_MENU* editMenu = new ACTION_MENU( false, selTool );

    editMenu->Add( ACTIONS::undo );
    editMenu->Add( ACTIONS::redo );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::pasteSpecial );
    editMenu->Add( ACTIONS::doDelete );

    editMenu->AppendSeparator();

    // Select Submenu
    ACTION_MENU* selectSubMenu = new ACTION_MENU( false, selTool );
    selectSubMenu->SetTitle( _( "&Select" ) );

    selectSubMenu->Add( ACTIONS::selectAll );
    selectSubMenu->Add( ACTIONS::unselectAll );

    editMenu->Add( selectSubMenu );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::find );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::editTracksAndVias );
    editMenu->Add( PCB_ACTIONS::editTextAndGraphics );
    editMenu->Add( PCB_ACTIONS::editTeardrops );
    editMenu->Add( PCB_ACTIONS::changeFootprints );
    editMenu->Add( PCB_ACTIONS::swapLayers );
    editMenu->Add( ACTIONS::gridOrigin );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::zoneFillAll );
    editMenu->Add( PCB_ACTIONS::zoneUnfillAll );
    editMenu->Add( PCB_ACTIONS::regenerateAllTuning );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::deleteTool );
    editMenu->Add( PCB_ACTIONS::globalDeletions );


    //----- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    ACTION_MENU* showHidePanels = new ACTION_MENU( false, selTool );
    showHidePanels->SetTitle( _( "Panels" ) );
    showHidePanels->Add( ACTIONS::showProperties,                 ACTION_MENU::CHECK );
    showHidePanels->Add( PCB_ACTIONS::showSearch,                 ACTION_MENU::CHECK );
    showHidePanels->Add( PCB_ACTIONS::showLayersManager,          ACTION_MENU::CHECK );
    showHidePanels->Add( PCB_ACTIONS::showNetInspector,           ACTION_MENU::CHECK );

    if( ADVANCED_CFG::GetCfg().m_EnablePcbDesignBlocks )
        showHidePanels->Add( PCB_ACTIONS::showDesignBlockPanel, ACTION_MENU::CHECK, _( "Design Blocks" ) );

    viewMenu->Add( showHidePanels );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::showFootprintBrowser );
    viewMenu->Add( ACTIONS::show3DViewer );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomFitObjects );
    viewMenu->Add( ACTIONS::zoomFitSelection );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    // Drawing Mode Submenu
    ACTION_MENU* drawingModeSubMenu = new ACTION_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( BITMAPS::add_zone );

    drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayFilled,   ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayOutline,  ACTION_MENU::CHECK );

    if( ADVANCED_CFG::GetCfg().m_ExtraZoneDisplayModes )
    {
        drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayFractured,    ACTION_MENU::CHECK );
        drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayTriangulated, ACTION_MENU::CHECK );
    }

    drawingModeSubMenu->AppendSeparator();
    drawingModeSubMenu->Add( PCB_ACTIONS::padDisplayMode,      ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::viaDisplayMode,      ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::trackDisplayMode,    ACTION_MENU::CHECK );

    drawingModeSubMenu->AppendSeparator();
    drawingModeSubMenu->Add( PCB_ACTIONS::graphicsOutlines,    ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::textOutlines,        ACTION_MENU::CHECK );

    viewMenu->Add( drawingModeSubMenu );

    // Contrast Mode Submenu
    ACTION_MENU* contrastModeSubMenu = new ACTION_MENU( false, selTool );
    contrastModeSubMenu->SetTitle( _( "&Contrast Mode" ) );
    contrastModeSubMenu->SetIcon( BITMAPS::contrast_mode );

    contrastModeSubMenu->Add( ACTIONS::highContrastMode,    ACTION_MENU::CHECK );
    contrastModeSubMenu->Add( PCB_ACTIONS::layerAlphaDec );
    contrastModeSubMenu->Add( PCB_ACTIONS::layerAlphaInc );
    viewMenu->Add( contrastModeSubMenu );

    viewMenu->Add( PCB_ACTIONS::flipBoard,                  ACTION_MENU::CHECK );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

    //-- Place Menu ----------------------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( PCB_ACTIONS::placeFootprint );
    placeMenu->Add( PCB_ACTIONS::drawVia );
    placeMenu->Add( PCB_ACTIONS::drawZone );
    placeMenu->Add( PCB_ACTIONS::drawRuleArea );

    ACTION_MENU* muwaveSubmenu = new ACTION_MENU( false, selTool );
    muwaveSubmenu->SetTitle( _( "Draw Microwave Shapes" ) );
    muwaveSubmenu->SetIcon( BITMAPS::mw_add_line );
    muwaveSubmenu->Add( PCB_ACTIONS::microwaveCreateLine );
    muwaveSubmenu->Add( PCB_ACTIONS::microwaveCreateGap );
    muwaveSubmenu->Add( PCB_ACTIONS::microwaveCreateStub );
    muwaveSubmenu->Add( PCB_ACTIONS::microwaveCreateStubArc );
    muwaveSubmenu->Add( PCB_ACTIONS::microwaveCreateFunctionShape );
    placeMenu->Add( muwaveSubmenu );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::drawLine );
    placeMenu->Add( PCB_ACTIONS::drawArc );
    placeMenu->Add( PCB_ACTIONS::drawRectangle );
    placeMenu->Add( PCB_ACTIONS::drawCircle );
    placeMenu->Add( PCB_ACTIONS::drawPolygon );
    placeMenu->Add( PCB_ACTIONS::drawBezier );
    placeMenu->Add( PCB_ACTIONS::placeReferenceImage );
    placeMenu->Add( PCB_ACTIONS::placeText );
    placeMenu->Add( PCB_ACTIONS::drawTextBox );
    placeMenu->Add( PCB_ACTIONS::drawTable );
    placeMenu->Add( PCB_ACTIONS::placePoint );
    placeMenu->Add( PCB_ACTIONS::placeBarcode );

    placeMenu->AppendSeparator();
    ACTION_MENU* dimensionSubmenu = new ACTION_MENU( false, selTool );
    dimensionSubmenu->SetTitle( _( "Draw Dimensions" ) );
    dimensionSubmenu->SetIcon( BITMAPS::add_aligned_dimension );
    dimensionSubmenu->Add( PCB_ACTIONS::drawOrthogonalDimension );
    dimensionSubmenu->Add( PCB_ACTIONS::drawAlignedDimension );
    dimensionSubmenu->Add( PCB_ACTIONS::drawCenterDimension );
    dimensionSubmenu->Add( PCB_ACTIONS::drawRadialDimension );
    dimensionSubmenu->Add( PCB_ACTIONS::drawLeader );
    placeMenu->Add( dimensionSubmenu );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::placeCharacteristics );
    placeMenu->Add( PCB_ACTIONS::placeStackup );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::drillOrigin );
    placeMenu->Add( PCB_ACTIONS::drillResetOrigin );
    placeMenu->Add( ACTIONS::gridSetOrigin );
    placeMenu->Add( ACTIONS::gridResetOrigin );

    placeMenu->AppendSeparator();
    ACTION_MENU* autoplaceSubmenu = new ACTION_MENU( false, selTool );
    autoplaceSubmenu->SetTitle( _( "Auto-Place Footprints" ) );
    autoplaceSubmenu->SetIcon( BITMAPS::mode_module );

    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceOffboardComponents );
    autoplaceSubmenu->Add( PCB_ACTIONS::autoplaceSelectedComponents );

    placeMenu->Add( autoplaceSubmenu );

    //-- Route Menu ----------------------------------------------------------
    //
    ACTION_MENU* routeMenu = new ACTION_MENU( false, selTool );

    routeMenu->Add( PCB_ACTIONS::selectLayerPair );

    routeMenu->AppendSeparator();
    routeMenu->Add( PCB_ACTIONS::routeSingleTrack );
    routeMenu->Add( PCB_ACTIONS::routeDiffPair );

    routeMenu->AppendSeparator();
    routeMenu->Add( PCB_ACTIONS::tuneSingleTrack );
    routeMenu->Add( PCB_ACTIONS::tuneDiffPair );
    routeMenu->Add( PCB_ACTIONS::tuneSkew );

    routeMenu->AppendSeparator();
    routeMenu->Add( PCB_ACTIONS::routerSettingsDialog );


    //-- Inspect Menu --------------------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( PCB_ACTIONS::boardStatistics );
    inspectMenu->Add( ACTIONS::measureTool );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( PCB_ACTIONS::runDRC );
    inspectMenu->Add( ACTIONS::prevMarker );
    inspectMenu->Add( ACTIONS::nextMarker );
    inspectMenu->Add( ACTIONS::excludeMarker );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( PCB_ACTIONS::inspectClearance );
    inspectMenu->Add( PCB_ACTIONS::inspectConstraints );
    inspectMenu->Add( PCB_ACTIONS::showFootprintAssociations );
    inspectMenu->Add( PCB_ACTIONS::diffFootprint );


    //-- Tools menu ----------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    toolsMenu->Add( ACTIONS::updatePcbFromSchematic )->Enable( !Kiface().IsSingle() );
    toolsMenu->Add( PCB_ACTIONS::showEeschema );

    if( !Kiface().IsSingle() )
        toolsMenu->Add( ACTIONS::showProjectManager );

    toolsMenu->Add( ACTIONS::showCalculatorTools );

    if( ADVANCED_CFG::GetCfg().m_EnableDrcRuleEditor )
    {
        toolsMenu->AppendSeparator();
        toolsMenu->Add( PCB_ACTIONS::drcRuleEditor );
    }

    toolsMenu->AppendSeparator();
    toolsMenu->Add( ACTIONS::showFootprintEditor );
    toolsMenu->Add( PCB_ACTIONS::updateFootprints );

    //Zones management
    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::zonesManager );

    if( ADVANCED_CFG::GetCfg().m_EnableGenerators )
    {
        toolsMenu->AppendSeparator();
        toolsMenu->Add( PCB_ACTIONS::generatorsShowManager );
        toolsMenu->Add( PCB_ACTIONS::regenerateAll );
        toolsMenu->Add( PCB_ACTIONS::regenerateSelected );
    }

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::cleanupTracksAndVias );
    toolsMenu->Add( PCB_ACTIONS::removeUnusedPads );
    toolsMenu->Add( PCB_ACTIONS::cleanupGraphics );
    toolsMenu->Add( PCB_ACTIONS::repairBoard );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::collect3DModels );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::boardReannotate );
    toolsMenu->Add( ACTIONS::updateSchematicFromPcb )->Enable( !Kiface().IsSingle() );

    if( SCRIPTING::IsWxAvailable() )
    {
        toolsMenu->AppendSeparator();
        toolsMenu->Add( PCB_ACTIONS::showPythonConsole );
    }

    ACTION_MENU* multichannelSubmenu = new ACTION_MENU( false, selTool );
    multichannelSubmenu->SetTitle( _( "Multi-Channel" ) );
    multichannelSubmenu->SetIcon( BITMAPS::mode_module );
    multichannelSubmenu->Add( PCB_ACTIONS::generatePlacementRuleAreas );
    multichannelSubmenu->Add( PCB_ACTIONS::repeatLayout );

    toolsMenu->Add( multichannelSubmenu );

    ACTION_MENU* submenuActionPlugins = new ACTION_MENU( false, selTool );
    submenuActionPlugins->SetTitle( _( "External Plugins" ) );
    submenuActionPlugins->SetIcon( BITMAPS::puzzle_piece );

    submenuActionPlugins->Add( ACTIONS::pluginsReload );
    submenuActionPlugins->Add( PCB_ACTIONS::pluginsShowFolder );

    // Populate the Action Plugin sub-menu: Must be done before Add
    // Since the object is cloned by Add
    submenuActionPlugins->AppendSeparator();
    buildActionPluginMenus( submenuActionPlugins );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( submenuActionPlugins );

    //-- Preferences menu ----------------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showFootprintLibTable );

    if( ADVANCED_CFG::GetCfg().m_EnablePcbDesignBlocks )
        prefsMenu->Add( ACTIONS::showDesignBlockLibTable );

    prefsMenu->Add( ACTIONS::openPreferences );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, selTool );


    //--MenuBar -----------------------------------------------------------
    //
    menuBar->Append( fileMenu,    _( "&File" ) );
    menuBar->Append( editMenu,    _( "&Edit" ) );
    menuBar->Append( viewMenu,    _( "&View" ) );
    menuBar->Append( placeMenu,   _( "&Place" ) );
    menuBar->Append( routeMenu,   _( "Ro&ute" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu,   _( "&Tools" ) );
    menuBar->Append( prefsMenu,   _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;

}
