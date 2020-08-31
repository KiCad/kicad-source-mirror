/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2007 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2009 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <menus_helpers.h>
#include <pgm_base.h>
#include <tool/action_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>
#include <lib_manager.h>
#include "lib_edit_frame.h"
#include <widgets/wx_menubar.h>


void LIB_EDIT_FRAME::ReCreateMenuBar()
{
    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    // wxWidgets handles the Mac Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, selTool );

    fileMenu->Add( ACTIONS::newLibrary );
    fileMenu->Add( ACTIONS::addLibrary );
    fileMenu->Add( EE_ACTIONS::newSymbol );

    fileMenu->AppendSeparator();
    fileMenu->Add( ACTIONS::save );
    fileMenu->Add( ACTIONS::saveCopyAs );
    fileMenu->Add( ACTIONS::saveAll );
    fileMenu->Add( ACTIONS::revert );

    fileMenu->AppendSeparator();
    fileMenu->Add( EE_ACTIONS::importSymbol );

    // Export submenu
    ACTION_MENU* submenuExport = new ACTION_MENU( false );
    submenuExport->SetTool( selTool );
    submenuExport->SetTitle( _( "Export" ) );
    submenuExport->SetIcon( export_xpm );
    submenuExport->Add( EE_ACTIONS::exportSymbol );
    submenuExport->Add( EE_ACTIONS::exportSymbolView );
    submenuExport->Add( EE_ACTIONS::exportSymbolAsSVG );
    fileMenu->Add( submenuExport );

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
    editMenu->Add( ACTIONS::paste );
    editMenu->Add( ACTIONS::doDelete );
    editMenu->Add( ACTIONS::duplicate );

    editMenu->AppendSeparator();
    editMenu->Add( ACTIONS::selectAll );

    editMenu->AppendSeparator();
    editMenu->Add( EE_ACTIONS::symbolProperties );
    editMenu->Add( EE_ACTIONS::pinTable );


    //-- View menu -----------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, selTool );

    viewMenu->Add( ACTIONS::showSymbolBrowser );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomTool );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,           ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::gridProperties );

    // Units submenu
    ACTION_MENU* unitsSubMenu = new ACTION_MENU( false, selTool );
    unitsSubMenu->SetTitle( _( "&Units" ) );
    unitsSubMenu->SetIcon( unit_mm_xpm );
    unitsSubMenu->Add( ACTIONS::imperialUnits,    ACTION_MENU::CHECK );
    unitsSubMenu->Add( ACTIONS::metricUnits,      ACTION_MENU::CHECK );
    viewMenu->Add( unitsSubMenu );

    viewMenu->Add( ACTIONS::toggleCursorStyle,    ACTION_MENU::CHECK );

    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::showComponentTree, ACTION_MENU::CHECK );


    //-- Place menu -----------------------------------------------
    //
    ACTION_MENU* placeMenu = new ACTION_MENU( false, selTool );

    placeMenu->Add( EE_ACTIONS::placeSymbolPin );
    placeMenu->Add( EE_ACTIONS::placeSymbolText );
    placeMenu->Add( EE_ACTIONS::drawSymbolRectangle );
    placeMenu->Add( EE_ACTIONS::drawSymbolCircle );
    placeMenu->Add( EE_ACTIONS::drawSymbolArc );
    placeMenu->Add( EE_ACTIONS::drawSymbolLines );


    //-- Inspect menu -----------------------------------------------
    //
    ACTION_MENU* inspectMenu = new ACTION_MENU( false, selTool );

    inspectMenu->Add( EE_ACTIONS::showDatasheet );
    inspectMenu->Add( EE_ACTIONS::runERC );


    //-- Preferences menu -----------------------------------------------
    //
    ACTION_MENU* prefsMenu = new ACTION_MENU( false, selTool );

    prefsMenu->Add( ACTIONS::configurePaths );
    prefsMenu->Add( ACTIONS::showSymbolLibTable );
    prefsMenu->Add( _( "Preferences...\tCTRL+," ),
                    _( "Show preferences for all open tools" ),
                    wxID_PREFERENCES,
                    preference_xpm );

    prefsMenu->AppendSeparator();
    AddMenuLanguageList( prefsMenu, selTool );

#ifndef __WXMAC__
    prefsMenu->AppendSeparator();
    prefsMenu->Add( ACTIONS::acceleratedGraphics, ACTION_MENU::CHECK );
    prefsMenu->Add( ACTIONS::standardGraphics,    ACTION_MENU::CHECK );
#endif


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
