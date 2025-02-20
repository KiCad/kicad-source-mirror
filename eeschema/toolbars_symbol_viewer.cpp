/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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


std::optional<TOOLBAR_CONFIGURATION> SYMBOL_VIEWER_FRAME::DefaultTopMainToolbarConfig()
{
    TOOLBAR_CONFIGURATION config;

    // clang-format off
    /* TODO: Move these to actions
    m_tbTopMain->AddTool( ID_LIBVIEW_PREVIOUS, wxEmptyString,
        KiScaledBitmap( BITMAPS::lib_previous, this ),
        _( "Display previous symbol" ) );

    m_tbTopMain->AddTool( ID_LIBVIEW_NEXT, wxEmptyString,
            KiScaledBitmap( BITMAPS::lib_next, this ),
            _( "Display next symbol" ) );
    */

    config.AppendSeparator()
          .AppendAction( ACTIONS::zoomRedraw )
          .AppendAction( ACTIONS::zoomInCenter )
          .AppendAction( ACTIONS::zoomOutCenter )
          .AppendAction( ACTIONS::zoomFitScreen );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showElectricalTypes )
          .AppendAction( EE_ACTIONS::showPinNumbers );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::showDeMorganStandard )
          .AppendAction( EE_ACTIONS::showDeMorganAlternate );

    config.AppendSeparator()
          .AppendControl( "control.SymViewUnitSelector" );

    config.AppendSeparator()
          .AppendAction( ACTIONS::showDatasheet );

    config.AppendSeparator()
          .AppendAction( EE_ACTIONS::addSymbolToSchematic );

    // clang-format on
    return config;
}


void SYMBOL_VIEWER_FRAME::configureToolbars()
{
    SCH_BASE_FRAME::configureToolbars();

    // Toolbar widget for selecting the unit to show in the symbol viewer
    auto unitChoiceFactory =
        [this]( ACTION_TOOLBAR* aToolbar )
        {
            if( !m_unitChoice )
            {
                m_unitChoice = new wxChoice( m_tbTopMain, ID_LIBVIEW_SELECT_UNIT_NUMBER,
                    wxDefaultPosition, wxSize( 150, -1 ) );
            }

            aToolbar->Add( m_unitChoice );
        };

    RegisterCustomToolbarControlFactory("control.SymViewUnitSelector", _( "Symbol unit selector" ),
                                        _( "Displays the unit being currently edited" ),
                                         unitChoiceFactory );
}


void SYMBOL_VIEWER_FRAME::doReCreateMenuBar()
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

    viewMenu->AppendSeparator();
    viewMenu->Add( EE_ACTIONS::showElectricalTypes, ACTION_MENU::CHECK );
    viewMenu->Add( EE_ACTIONS::showPinNumbers,      ACTION_MENU::CHECK );


    //-- Menubar -------------------------------------------------------------
    //
    menuBar->Append( fileMenu, _( "&File" ) );
    menuBar->Append( viewMenu, _( "&View" ) );
    AddStandardHelpMenu( menuBar );

    SetMenuBar( menuBar );
    delete oldMenuBar;
}
