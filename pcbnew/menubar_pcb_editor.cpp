/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <filehistory.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pcb_edit_frame.h>
#include <pcbnew_id.h>
#include <pgm_base.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <widgets/wx_menubar.h>


void PCB_EDIT_FRAME::ReCreateMenuBar()
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
            openRecentMenu = new ACTION_MENU( false );
            openRecentMenu->SetTool( selTool );
            openRecentMenu->SetTitle( _( "Open Recent" ) );
            openRecentMenu->SetIcon( BITMAPS::recent );

            fileHistory.UseMenu( openRecentMenu );
            fileHistory.AddFilesToMenu();
        }

        fileMenu->Add( ACTIONS::doNew );
        fileMenu->Add( ACTIONS::open );

        wxMenuItem* item = fileMenu->Add( openRecentMenu );

        // Add the file menu condition here since it needs the item ID for the submenu
        ACTION_CONDITIONS cond;
        cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( fileHistory ) );
        RegisterUIUpdateHandler( item->GetId(), cond );

        fileMenu->Add( PCB_ACTIONS::appendBoard );
        fileMenu->AppendSeparator();
    }

    fileMenu->Add( ACTIONS::save );

    // Save as menu:
    // under a project mgr we do not want to modify the board filename
    // to keep consistency with the project mgr which expects files names same as prj name
    // for main files
    if( Kiface().IsSingle() )
        fileMenu->Add( ACTIONS::saveAs );
    else
        fileMenu->Add( ACTIONS::saveCopyAs );

    fileMenu->AppendSeparator();
    fileMenu->Add( _( "Resc&ue" ),
                   _( "Clear board and get last rescue file automatically saved by PCB editor" ),
                   ID_MENU_RECOVER_BOARD_AUTOSAVE,
                   BITMAPS::rescue );

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false );
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( BITMAPS::import );

    submenuImport->Add( PCB_ACTIONS::importNetlist );
    submenuImport->Add( PCB_ACTIONS::importSpecctraSession );
    submenuImport->Add( _( "Graphics..." ), _( "Import 2D drawing file" ),
                        ID_GEN_IMPORT_GRAPHICS_FILE, BITMAPS::import_vector );

    if( Kiface().IsSingle() )
    {
        submenuImport->Add( _( "Non-KiCad Board File..." ),
                            _( "Import board file from other applications" ),
                            ID_IMPORT_NON_KICAD_BOARD, BITMAPS::import_brd_file );
    }

    fileMenu->AppendSeparator();
    fileMenu->Add( submenuImport );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( BITMAPS::export_file );

    submenuExport->Add( PCB_ACTIONS::exportSpecctraDSN );
    submenuExport->Add( _( "GenCAD..." ), _( "Export GenCAD board representation" ),
                        ID_GEN_EXPORT_FILE_GENCADFORMAT, BITMAPS::post_gencad );
    submenuExport->Add( _( "VRML..." ), _( "Export VRML 3D board representation" ),
                        ID_GEN_EXPORT_FILE_VRML, BITMAPS::export3d );
    submenuExport->Add( _( "IDFv3..." ), _( "Export IDF 3D board representation" ),
                        ID_GEN_EXPORT_FILE_IDF3, BITMAPS::export_idf );
    submenuExport->Add( _( "STEP..." ), _( "Export STEP 3D board representation" ),
                        ID_GEN_EXPORT_FILE_STEP, BITMAPS::export_step );
    submenuExport->Add( _( "SVG..." ), _( "Export SVG board representation" ),
                        ID_GEN_PLOT_SVG, BITMAPS::export_svg );
    submenuExport->Add( _( "Footprint Association (.cmp) File..." ),
                        _( "Export footprint association file (*.cmp) for schematic back annotation" ),
                        ID_PCB_GEN_CMP_FILE, BITMAPS::export_cmp );
    submenuExport->Add( _( "Hyperlynx..." ), "",
                        ID_GEN_EXPORT_FILE_HYPERLYNX, BITMAPS::export_step );

    submenuExport->AppendSeparator();
    submenuExport->Add( _( "Export Footprints to Library..." ),
                        _( "Add footprints used on board to an existing footprint library\n"
                           "(does not remove other footprints from this library)" ),
                        ID_MENU_EXPORT_FOOTPRINTS_TO_LIBRARY, BITMAPS::library_archive );

    submenuExport->Add( _( "Export Footprints to New Library..." ),
                        _( "Create a new footprint library containing the footprints used on board\n"
                           "(if the library already exists it will be replaced)" ),
                        ID_MENU_EXPORT_FOOTPRINTS_TO_NEW_LIBRARY, BITMAPS::library_archive_as );

    fileMenu->Add( submenuExport );

    // Fabrication Outputs submenu
    ACTION_MENU* submenuFabOutputs = new ACTION_MENU( false );
    submenuFabOutputs->SetTool( selTool );
    submenuFabOutputs->SetTitle( _( "Fabrication Outputs" ) );
    submenuFabOutputs->SetIcon( BITMAPS::fabrication );

    submenuFabOutputs->Add( PCB_ACTIONS::generateGerbers );
    submenuFabOutputs->Add( PCB_ACTIONS::generateDrillFiles );
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
    editMenu->Add( ACTIONS::doDelete );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::find );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::editTracksAndVias );
    editMenu->Add( PCB_ACTIONS::editTextAndGraphics );
    editMenu->Add( PCB_ACTIONS::changeFootprints );
    editMenu->Add( PCB_ACTIONS::swapLayers );

    editMenu->AppendSeparator();
    editMenu->Add( PCB_ACTIONS::zoneFillAll );
    editMenu->Add( PCB_ACTIONS::zoneUnfillAll );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::deleteTool );
    editMenu->Add( PCB_ACTIONS::globalDeletions );


    //----- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( PCB_ACTIONS::showLayersManager,    ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::showFootprintBrowser );
    viewMenu->Add( ACTIONS::show3DViewer );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomFitObjects );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,               ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::gridProperties );
    viewMenu->Add( PCB_ACTIONS::togglePolarCoords,    ACTION_MENU::CHECK );

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( BITMAPS::unit_mm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,          ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,            ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits,     ACTION_MENU::CHECK );
    viewMenu->Add( unitsSubMenu );

    viewMenu->Add( ACTIONS::toggleCursorStyle,        ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( PCB_ACTIONS::showRatsnest,         ACTION_MENU::CHECK );
    viewMenu->Add( PCB_ACTIONS::ratsnestLineMode,     ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    // Drawing Mode Submenu
    ACTION_MENU* drawingModeSubMenu = new ACTION_MENU( false, selTool );
    drawingModeSubMenu->SetTitle( _( "&Drawing Mode" ) );
    drawingModeSubMenu->SetIcon( BITMAPS::add_zone );

    drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayEnable,   ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayDisable,  ACTION_MENU::CHECK );
    drawingModeSubMenu->Add( PCB_ACTIONS::zoneDisplayOutlines, ACTION_MENU::CHECK );

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
    placeMenu->Add( PCB_ACTIONS::placeText );
    placeMenu->Add( PCB_ACTIONS::drawLine );
    placeMenu->Add( PCB_ACTIONS::drawArc );
    placeMenu->Add( PCB_ACTIONS::drawRectangle );
    placeMenu->Add( PCB_ACTIONS::drawCircle );
    placeMenu->Add( PCB_ACTIONS::drawPolygon );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::drawAlignedDimension );
    placeMenu->Add( PCB_ACTIONS::drawOrthogonalDimension );
    placeMenu->Add( PCB_ACTIONS::drawCenterDimension );
    placeMenu->Add( PCB_ACTIONS::drawLeader );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::placeCharacteristics );
    placeMenu->Add( PCB_ACTIONS::placeStackup );
    placeMenu->Add( PCB_ACTIONS::placeTarget );

    placeMenu->AppendSeparator();
    placeMenu->Add( PCB_ACTIONS::drillOrigin );
    placeMenu->Add( ACTIONS::gridSetOrigin );

    placeMenu->AppendSeparator();

    ACTION_MENU* autoplaceSubmenu = new ACTION_MENU( false );
    autoplaceSubmenu->SetTitle( _( "Auto-Place Footprints" ) );
    autoplaceSubmenu->SetTool( selTool );
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
    routeMenu->Add( PCB_ACTIONS::routerTuneSingleTrace );
    routeMenu->Add( PCB_ACTIONS::routerTuneDiffPair );
    routeMenu->Add( PCB_ACTIONS::routerTuneDiffPairSkew );

    routeMenu->AppendSeparator();
    routeMenu->Add( PCB_ACTIONS::routerSettingsDialog );


    //-- Inspect Menu --------------------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( PCB_ACTIONS::listNets );
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


    //-- Tools menu ----------------------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    wxMenuItem* update = toolsMenu->Add( ACTIONS::updatePcbFromSchematic );
    update->Enable( !Kiface().IsSingle() );

    toolsMenu->Add( PCB_ACTIONS::showEeschema );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( ACTIONS::showFootprintEditor );
    toolsMenu->Add( PCB_ACTIONS::updateFootprints );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::cleanupTracksAndVias );
    toolsMenu->Add( PCB_ACTIONS::removeUnusedPads );
    toolsMenu->Add( PCB_ACTIONS::cleanupGraphics );
    toolsMenu->Add( PCB_ACTIONS::repairBoard );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::boardReannotate );
    update = toolsMenu->Add( ACTIONS::updateSchematicFromPcb );
    update->Enable( !Kiface().IsSingle() );


#if defined(KICAD_SCRIPTING_WXPYTHON)
    toolsMenu->AppendSeparator();
    toolsMenu->Add( PCB_ACTIONS::showPythonConsole );
#endif

#if defined(KICAD_SCRIPTING) && defined(KICAD_SCRIPTING_ACTION_MENU)
    ACTION_MENU* submenuActionPlugins = new ACTION_MENU( false );
    submenuActionPlugins->SetTool( selTool );
    submenuActionPlugins->SetTitle( _( "External Plugins" ) );
    submenuActionPlugins->SetIcon( BITMAPS::puzzle_piece );

    submenuActionPlugins->Add( _( "Refresh Plugins" ),
                               _( "Reload all python plugins and refresh plugin menus" ),
                               ID_TOOLBARH_PCB_ACTION_PLUGIN_REFRESH,
                               BITMAPS::reload );
#ifdef __APPLE__
    submenuActionPlugins->Add( _( "Reveal Plugin Folder in Finder" ),
                               _( "Reveals the plugins folder in a Finder window" ),
                               ID_TOOLBARH_PCB_ACTION_PLUGIN_SHOW_FOLDER,
                               BITMAPS::directory_open );
#else
    submenuActionPlugins->Add( _( "Open Plugin Directory" ),
                               _( "Opens the directory in the default system file manager" ),
                               ID_TOOLBARH_PCB_ACTION_PLUGIN_SHOW_FOLDER,
                               BITMAPS::directory_open );
#endif
    // Populate the Action Plugin sub-menu: Must be done before Add
    // Since the object is cloned by Add
    submenuActionPlugins->AppendSeparator();
    buildActionPluginMenus( submenuActionPlugins );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( submenuActionPlugins );
#endif


    //-- Preferences menu ----------------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showFootprintLibTable );

    prefsMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    BITMAPS::preference );

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
