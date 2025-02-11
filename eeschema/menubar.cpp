/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
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
#include <file_history.h>
#include <kiface_base.h>
#include <schematic.h>
#include <tool/action_manager.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_actions.h>
#include "eeschema_id.h"
#include "sch_edit_frame.h"
#include <widgets/wx_menubar.h>
#include <advanced_config.h>


void SCH_EDIT_FRAME::doReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();

    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU*   fileMenu = new ACTION_MENU( false, selTool );
    static ACTION_MENU* openRecentMenu;

    if( Kiface().IsSingle() )   // When not under a project mgr
    {
        FILE_HISTORY& fileHistory = GetFileHistory();

        // Add this menu to the list of menus managed by the file history
        // (the file history will be updated when adding/removing files in history)
        if( !openRecentMenu )
        {
            openRecentMenu = new ACTION_MENU( false, selTool );
            openRecentMenu->SetIcon( BITMAPS::recent );

            fileHistory.UseMenu( openRecentMenu );
            fileHistory.AddFilesToMenu( openRecentMenu );
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
        fileMenu->AppendSeparator();
    }

    fileMenu->Add( ACTIONS::save );

    if( Kiface().IsSingle() )
        fileMenu->Add( ACTIONS::saveAs );
    else
        fileMenu->Add( EE_ACTIONS::saveCurrSheetCopyAs );

    fileMenu->Add( ACTIONS::revert );

    fileMenu->AppendSeparator();

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false, selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( BITMAPS::import );

    submenuImport->Add( _( "Non-KiCad Schematic..." ),
                _( "Replace current schematic sheet with one imported from another application" ),
                ID_IMPORT_NON_KICAD_SCH,
                BITMAPS::import_document );

    submenuImport->Add( EE_ACTIONS::importFPAssignments, ACTION_MENU::NORMAL,
                        _( "Footprint Assignments..." ) );
    submenuImport->Add( EE_ACTIONS::importGraphics, ACTION_MENU::NORMAL, _( "Graphics..." ) );

    fileMenu->Add( submenuImport );


    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( BITMAPS::export_file );
    submenuExport->Add( EE_ACTIONS::drawSheetOnClipboard, ACTION_MENU::NORMAL,
                        _( "Drawing to Clipboard" ) );
    submenuExport->Add( EE_ACTIONS::exportNetlist,        ACTION_MENU::NORMAL, _( "Netlist..." ) );
    submenuExport->Add( EE_ACTIONS::exportSymbolsToLibrary, ACTION_MENU::NORMAL,
                        _( "Symbols to Library..." ) );
    submenuExport->Add( EE_ACTIONS::exportSymbolsToNewLibrary, ACTION_MENU::NORMAL,
                        _( "Symbols to New Library..." ) );
    fileMenu->Add( submenuExport );

    fileMenu->AppendSeparator();
    fileMenu->Add( EE_ACTIONS::schematicSetup );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::pageSettings );
    fileMenu->Add( ACTIONS::print );
    fileMenu->Add( ACTIONS::plot );

    fileMenu->AppendSeparator();
    fileMenu->AddQuitOrClose( &Kiface(), _( "Schematic Editor" ) );


    //-- Edit menu -----------------------------------------------------------
    //
    ACTION_MENU* editMenu = new ACTION_MENU( false, selTool );

    editMenu->Add( ACTIONS::undo );
    editMenu->Add( ACTIONS::redo );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::copyAsText );
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::pasteSpecial );
    editMenu->Add( ACTIONS::doDelete );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );
    editMenu->Add( ACTIONS::unselectAll );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::find );
    editMenu->Add( ACTIONS::findAndReplace );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::deleteTool );
    editMenu->Add( EE_ACTIONS::editTextAndGraphics );
    editMenu->Add( EE_ACTIONS::changeSymbols );
    editMenu->Add( EE_ACTIONS::editPageNumber );

    ACTION_MENU* submenuAttributes = new ACTION_MENU( false, selTool );
    submenuAttributes->SetTitle( _( "Attributes" ) );

    submenuAttributes->Add( EE_ACTIONS::setExcludeFromSimulation, ACTION_MENU::CHECK );
    submenuAttributes->Add( EE_ACTIONS::setExcludeFromBOM, ACTION_MENU::CHECK );
    submenuAttributes->Add( EE_ACTIONS::setExcludeFromBoard, ACTION_MENU::CHECK );
    submenuAttributes->Add( EE_ACTIONS::setDNP, ACTION_MENU::CHECK );

    editMenu->Add( submenuAttributes );

    //-- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    // Show / Hide Panels submenu
    ACTION_MENU* showHidePanels = new ACTION_MENU( false, selTool );
    showHidePanels->SetTitle( _( "Panels" ) );

    showHidePanels->Add( ACTIONS::showProperties, ACTION_MENU::CHECK );
    showHidePanels->Add( ACTIONS::showSearch, ACTION_MENU::CHECK );
    showHidePanels->Add( EE_ACTIONS::showHierarchy, ACTION_MENU::CHECK );

    if( ADVANCED_CFG::GetCfg().m_IncrementalConnectivity )
        showHidePanels->Add( EE_ACTIONS::showNetNavigator, ACTION_MENU::CHECK );

    if( ADVANCED_CFG::GetCfg().m_EnableDesignBlocks )
        showHidePanels->Add( EE_ACTIONS::showDesignBlockPanel, ACTION_MENU::CHECK,
                             _( "Design Blocks" ) );

    viewMenu->Add( showHidePanels );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::showSymbolBrowser );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomFitObjects );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::navigateBack );
    viewMenu->Add( EE_ACTIONS::navigateUp );
    viewMenu->Add( EE_ACTIONS::navigateForward );
    viewMenu->Add( EE_ACTIONS::navigatePrevious );
    viewMenu->Add( EE_ACTIONS::navigateNext );



    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::toggleHiddenPins,      ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleHiddenFields,    ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleDirectiveLabels, ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleERCErrors,       ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleERCWarnings,     ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleERCExclusions,   ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::markSimExclusions,     ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleOPVoltages,      ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::toggleOPCurrents,      ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::togglePinAltIcons,     ACTION_MENU::CHECK );

#ifdef __APPLE__
    viewMenu->AppendSeparator();
#endif

    //-- Place menu -----------------------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( EE_ACTIONS::placeSymbol );
    placeMenu->Add( EE_ACTIONS::placePower );
    placeMenu->Add( EE_ACTIONS::drawWire );
    placeMenu->Add( EE_ACTIONS::drawBus );
    placeMenu->Add( EE_ACTIONS::placeBusWireEntry );
    placeMenu->Add( EE_ACTIONS::placeNoConnect );
    placeMenu->Add( EE_ACTIONS::placeJunction );
    placeMenu->Add( EE_ACTIONS::placeLabel );
    placeMenu->Add( EE_ACTIONS::placeGlobalLabel );
    placeMenu->Add( EE_ACTIONS::placeClassLabel );
    placeMenu->Add( EE_ACTIONS::drawRuleArea );

    placeMenu->AppendSeparator();
    placeMenu->Add( EE_ACTIONS::placeHierLabel );
    placeMenu->Add( EE_ACTIONS::drawSheet );
    placeMenu->Add( EE_ACTIONS::placeSheetPin );
    placeMenu->Add( EE_ACTIONS::syncAllSheetsPins );
    placeMenu->Add( EE_ACTIONS::importSheet );

    placeMenu->AppendSeparator();
    placeMenu->Add( EE_ACTIONS::placeSchematicText );
    placeMenu->Add( EE_ACTIONS::drawTextBox );
    placeMenu->Add( EE_ACTIONS::drawTable );
    placeMenu->Add( EE_ACTIONS::drawRectangle );
    placeMenu->Add( EE_ACTIONS::drawCircle );
    placeMenu->Add( EE_ACTIONS::drawArc );
    placeMenu->Add( EE_ACTIONS::drawBezier );
    placeMenu->Add( EE_ACTIONS::drawLines );
    placeMenu->Add( EE_ACTIONS::placeImage );


    //-- Inspect menu -----------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( EE_ACTIONS::showBusSyntaxHelp );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( EE_ACTIONS::runERC );
    inspectMenu->Add( ACTIONS::prevMarker );
    inspectMenu->Add( ACTIONS::nextMarker );
    inspectMenu->Add( ACTIONS::excludeMarker );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( EE_ACTIONS::diffSymbol );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( EE_ACTIONS::showSimulator );


    //-- Tools menu -----------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    wxMenuItem* update = toolsMenu->Add( ACTIONS::updatePcbFromSchematic );
    update->Enable( !Kiface().IsSingle() );

    toolsMenu->Add( EE_ACTIONS::showPcbNew );

    if( !Kiface().IsSingle() )
        toolsMenu->Add( ACTIONS::showProjectManager );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( ACTIONS::showSymbolEditor );
    toolsMenu->Add( EE_ACTIONS::updateSymbols );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::rescueSymbols );
    toolsMenu->Add( EE_ACTIONS::remapSymbols );

    if( ADVANCED_CFG::GetCfg().m_ShowRepairSchematic )
        toolsMenu->Add( EE_ACTIONS::repairSchematic );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::editSymbolFields );
    toolsMenu->Add( EE_ACTIONS::editSymbolLibraryLinks );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::annotate );
    toolsMenu->Add( EE_ACTIONS::incrementAnnotations );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::assignFootprints );
    toolsMenu->Add( EE_ACTIONS::generateBOM );
    toolsMenu->Add( EE_ACTIONS::generateBOMLegacy );

    toolsMenu->AppendSeparator();
    update = toolsMenu->Add( ACTIONS::updateSchematicFromPcb );
    update->Enable( !Kiface().IsSingle() );

#ifdef KICAD_IPC_API
    toolsMenu->AppendSeparator();
    toolsMenu->Add( ACTIONS::pluginsReload );
#endif

    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showSymbolLibTable );
    if( ADVANCED_CFG::GetCfg().m_EnableDesignBlocks )
        prefsMenu->Add( ACTIONS::showDesignBlockLibTable );
    prefsMenu->Add( ACTIONS::openPreferences );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, selTool );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu,    _( "&File" ) );
    menuBar->Append( editMenu,    _( "&Edit" ) );
    menuBar->Append( viewMenu,    _( "&View" ) );
    menuBar->Append( placeMenu,   _( "&Place" ) );
    menuBar->Append( inspectMenu, _( "&Inspect" ) );
    menuBar->Append( toolsMenu,   _( "&Tools" ) );
    menuBar->Append( prefsMenu,   _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


