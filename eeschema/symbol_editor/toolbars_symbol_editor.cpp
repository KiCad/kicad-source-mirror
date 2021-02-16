/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004 Jean-Pierre Charras, jaen-pierre.charras@gipsa-lab.inpg.com
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <eeschema_id.h>
#include <symbol_edit_frame.h>
#include <sch_painter.h>
#include <dialog_helpers.h>
#include <bitmaps.h>
#include <symbol_library_manager.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>

#ifdef __UNIX__
#define LISTBOX_WIDTH 140
#else
#define LISTBOX_WIDTH 120
#endif


void SYMBOL_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
    {
        m_drawToolBar->ClearToolbar();
    }
    else
    {
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_drawToolBar->SetAuiManager( &m_auimgr );
    }

    // Set up toolbar
    m_drawToolBar->Add( ACTIONS::selectionTool,           ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolPin,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolText,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolRectangle,  ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolCircle,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolArc,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSymbolLines,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbolAnchor,    ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::deleteTool,              ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->Realize();
}


void SYMBOL_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    // Set up toolbar
    m_mainToolBar->Add( EE_ACTIONS::newSymbol );

    if( !IsSymbolFromSchematic() )
        m_mainToolBar->Add( ACTIONS::saveAll );
    else
        m_mainToolBar->Add( ACTIONS::save );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::rotateCCW );
    m_mainToolBar->Add( EE_ACTIONS::rotateCW );
    m_mainToolBar->Add( EE_ACTIONS::mirrorV );
    m_mainToolBar->Add( EE_ACTIONS::mirrorH );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::symbolProperties );
    m_mainToolBar->Add( EE_ACTIONS::pinTable );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::showDatasheet );
    m_mainToolBar->Add( EE_ACTIONS::checkSymbol );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::showDeMorganStandard,  ACTION_TOOLBAR::TOGGLE );
    m_mainToolBar->Add( EE_ACTIONS::showDeMorganAlternate, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );

    if( m_unitSelectBox == nullptr )
        m_unitSelectBox = new wxComboBox( m_mainToolBar, ID_LIBEDIT_SELECT_PART_NUMBER,
                wxEmptyString, wxDefaultPosition, wxSize( LISTBOX_WIDTH, -1 ), 0,
                nullptr, wxCB_READONLY );
    m_mainToolBar->AddControl( m_unitSelectBox );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::toggleSyncedPinsMode, ACTION_TOOLBAR::TOGGLE );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::addSymbolToSchematic );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void SYMBOL_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR,
                                               wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    m_optionsToolBar->Add( ACTIONS::toggleGrid,             ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,            ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,              ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,       ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::showElectricalTypes, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::showComponentTree,   ACTION_TOOLBAR::TOGGLE );

    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_optionsToolBar->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );

    m_optionsToolBar->Realize();
}
