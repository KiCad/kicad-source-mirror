/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
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

#include <bitmaps.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/sch_actions.h>
#include <tools/sch_selection_tool.h>
#include <symbol_library_manager.h>
#include "symbol_edit_frame.h"
#include <widgets/wx_menubar.h>


void SYMBOL_EDIT_FRAME::doReCreateMenuBar()
{
    SCH_SELECTION_TOOL* selTool = m_toolManager->GetTool<SCH_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->Add( ACTIONS::newLibrary );
    fileMenu->Add( ACTIONS::addLibrary );
    fileMenu->Add( SCH_ACTIONS::saveLibraryAs );
    fileMenu->Add( SCH_ACTIONS::newSymbol );
    fileMenu->Add( SCH_ACTIONS::editLibSymbolWithLibEdit );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::save );
    fileMenu->Add( SCH_ACTIONS::saveSymbolAs );
    fileMenu->Add( SCH_ACTIONS::saveSymbolCopyAs );

    if( !IsSymbolFromSchematic() )
        fileMenu->Add( ACTIONS::saveAll );

    fileMenu->Add( ACTIONS::revert );

    fileMenu->AppendSeparator();

    ACTION_MENU* submenuImport = new ACTION_MENU( false, selTool );
    submenuImport->SetTitle( _( "Import" ) );
    submenuImport->SetIcon( BITMAPS::import );

    submenuImport->Add( SCH_ACTIONS::importSymbol,   ACTION_MENU::NORMAL, _( "Symbol..." ) );
    submenuImport->Add( SCH_ACTIONS::importGraphics, ACTION_MENU::NORMAL, _( "Graphics..." ) );

    fileMenu->Add( submenuImport );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false, selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( BITMAPS::export_file );
    submenuExport->Add( SCH_ACTIONS::exportSymbol,      ACTION_MENU::NORMAL, _( "Symbol..." ) );
    submenuExport->Add( SCH_ACTIONS::exportSymbolView,  ACTION_MENU::NORMAL, _( "View as PNG..." ) );
    submenuExport->Add( SCH_ACTIONS::exportSymbolAsSVG, ACTION_MENU::NORMAL, _( "Symbol as SVG..." ) );
    fileMenu->Add( submenuExport );

    fileMenu->AppendSeparator();
    fileMenu->Add( SCH_ACTIONS::symbolProperties );

    fileMenu->AppendSeparator();
    fileMenu->AddClose( _( "Library Editor" ) );


    //-- Edit menu -----------------------------------------------
    //
    ACTION_MENU* editMenu = new ACTION_MENU( false, selTool );

    editMenu->Add( ACTIONS::undo );
    editMenu->Add( ACTIONS::redo );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::cut );
    editMenu->Add( ACTIONS::copy );
    editMenu->Add( ACTIONS::copyAsText );
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::doDelete );
    editMenu->Add( ACTIONS::duplicate );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );
    editMenu->Add( ACTIONS::unselectAll );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::find );
    editMenu->Add( ACTIONS::findAndReplace );

    editMenu->AppendSeparator();
    editMenu->Add( SCH_ACTIONS::pinTable );
    editMenu->Add( SCH_ACTIONS::updateSymbolFields );


    //-- View menu -----------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    ACTION_MENU* showHidePanels = new ACTION_MENU( false, selTool );
    showHidePanels->SetTitle( _( "Panels" ) );
    showHidePanels->Add( ACTIONS::showProperties,  ACTION_MENU::CHECK );
    showHidePanels->Add( ACTIONS::showLibraryTree, ACTION_MENU::CHECK );
    viewMenu->Add( showHidePanels );
    viewMenu->AppendSeparator();

    viewMenu->Add( ACTIONS::showSymbolBrowser );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( SCH_ACTIONS::showHiddenPins,    ACTION_MENU::CHECK );
    viewMenu->Add( SCH_ACTIONS::showHiddenFields,  ACTION_MENU::CHECK );
    viewMenu->Add( SCH_ACTIONS::togglePinAltIcons, ACTION_MENU::CHECK );


    //-- Place menu -----------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( SCH_ACTIONS::placeSymbolPin );
    placeMenu->Add( SCH_ACTIONS::placeSymbolText );
    placeMenu->Add( SCH_ACTIONS::drawSymbolTextBox );
    placeMenu->Add( SCH_ACTIONS::drawRectangle );
    placeMenu->Add( SCH_ACTIONS::drawCircle );
    placeMenu->Add( SCH_ACTIONS::drawArc );
    placeMenu->Add( SCH_ACTIONS::drawBezier );
    placeMenu->Add( SCH_ACTIONS::drawSymbolLines );
    placeMenu->Add( SCH_ACTIONS::drawSymbolPolygon );


    //-- Inspect menu -----------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( ACTIONS::showDatasheet );

    inspectMenu->AppendSeparator();
    inspectMenu->Add( SCH_ACTIONS::checkSymbol );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showSymbolLibTable );
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
    menuBar->Append( prefsMenu,   _( "P&references" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
