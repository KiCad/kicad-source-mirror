/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <bitmaps.h>
#include <macros.h>
#include <symbol_library.h>
#include <eeschema_id.h>
#include <symbol_viewer_frame.h>
#include <sch_painter.h>
#include <tool/action_menu.h>
#include <tool/action_toolbar.h>
#include <tools/ee_actions.h>
#include <tools/symbol_editor_control.h>
#include <widgets/wx_menubar.h>


void SYMBOL_VIEWER_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    m_mainToolBar->AddTool( ID_LIBVIEW_SELECT_PART, wxEmptyString,
                            KiScaledBitmap( BITMAPS::library_browser, this ),
                            _( "Choose symbol" ) );

    m_mainToolBar->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
                            KiScaledBitmap( BITMAPS::lib_previous, this ),
                            _( "Display previous symbol" ) );

    m_mainToolBar->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
                            KiScaledBitmap( BITMAPS::lib_next, this ),
                            _( "Display next symbol" ) );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::showDeMorganStandard,  ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EE_ACTIONS::showDeMorganAlternate, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );

    if( m_unitChoice == nullptr )
        m_unitChoice = new wxChoice( m_mainToolBar, ID_LIBVIEW_SELECT_UNIT_NUMBER,
                                     wxDefaultPosition, wxSize( 150, -1 ) );
    m_mainToolBar->AddControl( m_unitChoice );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::showDatasheet );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::addSymbolToSchematic );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();

    m_mainToolBar->Refresh();
}


void SYMBOL_VIEWER_FRAME::ReCreateVToolbar()
{
}


void SYMBOL_VIEWER_FRAME::ReCreateMenuBar()
{
    SYMBOL_EDITOR_CONTROL* libControl = m_toolManager->GetTool<SYMBOL_EDITOR_CONTROL>();
    // wxWidgets handles the OSX Application menu behind the scenes, but that means
    // we always have to start from scratch with a new wxMenuBar.
    wxMenuBar*  oldMenuBar = GetMenuBar();
    WX_MENUBAR* menuBar    = new WX_MENUBAR();

    //-- File menu -----------------------------------------------------------
    //
    ACTION_MENU* fileMenu = new ACTION_MENU( false, libControl );

    fileMenu->AddClose( _( "Symbol Viewer" ) );


    //-- View menu -----------------------------------------------------------
    //
    ACTION_MENU* viewMenu = new ACTION_MENU( false, libControl );

    viewMenu->Add( ACTIONS::zoomInCenter );
    viewMenu->Add( ACTIONS::zoomOutCenter );
    viewMenu->Add( ACTIONS::zoomFitScreen );
    viewMenu->Add( ACTIONS::zoomRedraw );

    viewMenu->AppendSeparator();
    viewMenu->Add( ACTIONS::toggleGrid,             ACTION_MENU::CHECK );
    viewMenu->Add( ACTIONS::gridProperties );

    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::showElectricalTypes, ACTION_MENU::CHECK );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
