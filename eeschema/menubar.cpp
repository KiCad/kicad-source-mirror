/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <filehistory.h>
#include <kiface_i.h>
#include <menus_helpers.h>
#include <pgm_base.h>
#include <schematic.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_selection_tool.h>
#include <tools/ee_actions.h>
#include "eeschema_id.h"
#include "sch_edit_frame.h"
#include <widgets/wx_menubar.h>


void SCH_EDIT_FRAME::ReCreateMenuBar()
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
            openRecentMenu = new ACTION_MENU( false );
            openRecentMenu->SetTool( selTool );
            openRecentMenu->SetTitle( _( "Open Recent" ) );
            openRecentMenu->SetIcon( recent_xpm );

            fileHistory.UseMenu( openRecentMenu );
            fileHistory.AddFilesToMenu( openRecentMenu );
        }

        fileMenu->Add( ACTIONS::doNew );
        fileMenu->Add( ACTIONS::open );

        wxMenuItem* item = fileMenu->Add( openRecentMenu );

        // Add the file menu condition here since it needs the item ID for the submenu
        ACTION_CONDITIONS cond;
        cond.Enable( FILE_HISTORY::FileHistoryNotEmpty( fileHistory ) );
        RegisterUIUpdateHandler( item->GetId(), cond );
        fileMenu->AppendSeparator();
    }

    fileMenu->Add( ACTIONS::save );
    fileMenu->Add( ACTIONS::saveAs );

    fileMenu->AppendSeparator();

    fileMenu->Add( _( "Append Schematic Sheet Content..." ),
                   _( "Append schematic sheet content from another project to the current sheet" ),
                   ID_APPEND_PROJECT,
                   add_document_xpm );

    fileMenu->AppendSeparator();

    // Import submenu
    ACTION_MENU* submenuImport = new ACTION_MENU( false );
    submenuImport->SetTool( selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( import_xpm );
    submenuImport->Add( _( "Import Non KiCad Schematic..." ),
                        _( "Replace current schematic sheet with one imported from another application" ),
                        ID_IMPORT_NON_KICAD_SCH,
                        import_document_xpm );

    submenuImport->Add( EE_ACTIONS::importFPAssignments );
    fileMenu->Add( submenuImport );


    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );
    submenuExport->Add( EE_ACTIONS::drawSheetOnClipboard );
    submenuExport->Add( EE_ACTIONS::exportNetlist );
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
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::pasteSpecial );
    editMenu->Add( ACTIONS::doDelete );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::find );
    editMenu->Add( ACTIONS::findAndReplace );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::deleteTool );
    editMenu->Add( EE_ACTIONS::editTextAndGraphics );
    editMenu->Add( EE_ACTIONS::changeSymbols );
    editMenu->Add( EE_ACTIONS::editPageNumber );

    //-- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( ACTIONS::showSymbolBrowser );
    viewMenu->Add( EE_ACTIONS::navigateHierarchy );
    viewMenu->Add( EE_ACTIONS::leaveSheet );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomFitObjects );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,          ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::gridProperties );

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->Add( ACTIONS::inchesUnits,      ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::milsUnits,        ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::millimetersUnits, ACTION_MENU::CHECK );
    viewMenu->Add( unitsSubMenu );

    viewMenu->Add( ACTIONS::toggleCursorStyle,   ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::toggleHiddenPins, ACTION_MENU::CHECK );

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

    placeMenu->AppendSeparator();
    placeMenu->Add( EE_ACTIONS::placeHierLabel );
    placeMenu->Add( EE_ACTIONS::drawSheet );
    placeMenu->Add( EE_ACTIONS::importSheetPin );

    placeMenu->AppendSeparator();
    placeMenu->Add( EE_ACTIONS::drawLines );
    placeMenu->Add( EE_ACTIONS::placeSchematicText );
    placeMenu->Add( EE_ACTIONS::placeImage );


    //-- Inspect menu -----------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( EE_ACTIONS::runERC );
    inspectMenu->Add( ACTIONS::prevMarker );
    inspectMenu->Add( ACTIONS::nextMarker );
    inspectMenu->Add( ACTIONS::excludeMarker );

#ifdef KICAD_SPICE
    inspectMenu->AppendSeparator();
    inspectMenu->Add( EE_ACTIONS::runSimulation );
#endif


    //-- Tools menu -----------------------------------------------
    //
    ACTION_MENU* toolsMenu = new ACTION_MENU( false, selTool );

    wxMenuItem* update = toolsMenu->Add( ACTIONS::updatePcbFromSchematic );
    update->Enable( !Kiface().IsSingle() );
    update = toolsMenu->Add( ACTIONS::updateSchematicFromPcb );
    update->Enable( !Kiface().IsSingle() );

    toolsMenu->Add( EE_ACTIONS::showPcbNew );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( ACTIONS::showSymbolEditor );
    toolsMenu->Add( EE_ACTIONS::updateSymbols );
    toolsMenu->Add( EE_ACTIONS::rescueSymbols );
    toolsMenu->Add( EE_ACTIONS::remapSymbols );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::editSymbolFields );
    toolsMenu->Add( EE_ACTIONS::editSymbolLibraryLinks );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::annotate );
    toolsMenu->Add( EE_ACTIONS::showBusManager );

    toolsMenu->AppendSeparator();
    toolsMenu->Add( EE_ACTIONS::assignFootprints );
    toolsMenu->Add( EE_ACTIONS::generateBOM );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showSymbolLibTable );
    prefsMenu->Add( _( "Preferences..." ) + "\tCtrl+,",
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    preference_xpm );

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


