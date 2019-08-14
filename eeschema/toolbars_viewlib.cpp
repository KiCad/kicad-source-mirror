/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
 * Copyright (C) 2019 CERN
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */


#include <macros.h>
#include "class_library.h"
#include "eeschema_id.h"
#include "general.h"
#include "viewlib_frame.h"
#include <symbol_lib_table.h>
#include <tool/action_toolbar.h>
#include <tool/conditional_menu.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/lib_control.h>

void LIB_VIEW_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR,
                                            wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    ACTION_TOOLBAR* toolbar = static_cast<ACTION_TOOLBAR*>( m_mainToolBar );
    wxString msg;

    m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
                            KiScaledBitmap( add_component_xpm, this ),
                            _( "Select symbol to browse" ) );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
                            KiScaledBitmap( lib_previous_xpm, this ),
                            _( "Display previous symbol" ) );

    m_mainToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
                            KiScaledBitmap( lib_next_xpm, this ),
                            _( "Display next symbol" ) );

    toolbar->AddSeparator();
    toolbar->Add( ACTIONS::zoomRedraw );
    toolbar->Add( ACTIONS::zoomInCenter );
    toolbar->Add( ACTIONS::zoomOutCenter );
    toolbar->Add( ACTIONS::zoomFitScreen );

    KiScaledSeparator( m_mainToolBar, this );
    toolbar->Add( EE_ACTIONS::showDeMorganStandard, ACTION_TOOLBAR::TOGGLE );
    toolbar->Add( EE_ACTIONS::showDeMorganAlternate, ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_mainToolBar, this );
    m_unitChoice = new wxChoice( m_mainToolBar, ID_LIBVIEW_SELECT_PART_NUMBER,
                                 wxDefaultPosition, wxSize( 150, -1 ) );
    m_mainToolBar->AddControl( m_unitChoice );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( EE_ACTIONS::showDatasheet );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( EE_ACTIONS::addSymbolToSchematic );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();

    m_mainToolBar->Refresh();
}


void LIB_VIEW_FRAME::ReCreateVToolbar()
{
}


void LIB_VIEW_FRAME::ReCreateMenuBar()
{
    LIB_CONTROL* libControl = m_toolManager->GetTool<LIB_CONTROL>();
    // wxWidgets handles the OSX Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar* oldMenuBar = GetMenuBar();
    wxMenuBar* menuBar = new wxMenuBar();

    //-- File menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* fileMenu = new CONDITIONAL_MENU( false, libControl );

    fileMenu->AddClose( _( "Footprint Viewer" ) );

    fileMenu->Resolve();


    //-- View menu -----------------------------------------------------------
    //
    CONDITIONAL_MENU* viewMenu = new CONDITIONAL_MENU( false, libControl );

    auto gridShownCondition = [ this ] ( const SELECTION& aSel ) {
        return IsGridVisible();
    };
    auto electricalTypesShownCondition = [ this ] ( const SELECTION& aSel ) {
        return GetShowElectricalType();
    };

    viewMenu->AddItem( ACTIONS::zoomInCenter,             EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomOutCenter,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomFitScreen,            EE_CONDITIONS::ShowAlways );
    viewMenu->AddItem( ACTIONS::zoomRedraw,               EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( ACTIONS::toggleGrid,          gridShownCondition );
    viewMenu->AddItem( ACTIONS::gridProperties,           EE_CONDITIONS::ShowAlways );

    viewMenu->AddSeparator();
    viewMenu->AddCheckItem( EE_ACTIONS::showElectricalTypes, electricalTypesShownCondition );

    viewMenu->Resolve();

    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}


void LIB_VIEW_FRAME::SyncToolbars()
{
    LIB_PART*  symbol = GetSelectedSymbol();
    LIB_ALIAS* alias = GetSelectedAlias();

    m_mainToolBar->Toggle( EE_ACTIONS::showDatasheet, alias && !alias->GetDocFileName().IsEmpty() );
    m_mainToolBar->Toggle( EE_ACTIONS::showDeMorganStandard, symbol && symbol->HasConversion(),
                           m_convert == LIB_FIELD::LIB_CONVERT::BASE );
    m_mainToolBar->Toggle( EE_ACTIONS::showDeMorganAlternate, symbol && symbol->HasConversion(),
                           m_convert == LIB_FIELD::LIB_CONVERT::DEMORGAN );
    m_mainToolBar->Refresh();
}
