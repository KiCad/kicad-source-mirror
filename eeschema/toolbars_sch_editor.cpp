/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2008 Wayne Stambaugh <stambaughw@gmail.com>
 * Copyright (C) 2004-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <sch_draw_panel.h>
#include <sch_edit_frame.h>
#include <kiface_i.h>
#include <bitmaps.h>
#include <eeschema_id.h>
#include <tool/tool_manager.h>
#include <tool/action_toolbar.h>
#include <tools/ee_actions.h>
#include <tools/ee_selection_tool.h>

/* Create  the main Horizontal Toolbar for the schematic editor
 */
void SCH_EDIT_FRAME::ReCreateHToolbar()
{
    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR,
                                            wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL );
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    // Set up toolbar
    if( Kiface().IsSingle() )   // not when under a project mgr
    {
        m_mainToolBar->Add( ACTIONS::doNew );
        m_mainToolBar->Add( ACTIONS::open );
    }

    m_mainToolBar->Add( ACTIONS::save );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::schematicSetup );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::pageSettings );
    m_mainToolBar->Add( ACTIONS::print );
    m_mainToolBar->Add( ACTIONS::plot );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::paste );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::find );
    m_mainToolBar->Add( ACTIONS::findAndReplace );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomFitObjects );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE, ACTION_TOOLBAR::CANCEL );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::navigateHierarchy );
    m_mainToolBar->Add( EE_ACTIONS::leaveSheet );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::rotateCCW );
    m_mainToolBar->Add( EE_ACTIONS::rotateCW );
    m_mainToolBar->Add( EE_ACTIONS::mirrorV );
    m_mainToolBar->Add( EE_ACTIONS::mirrorH );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::showSymbolEditor );
    m_mainToolBar->Add( ACTIONS::showSymbolBrowser );
    m_mainToolBar->Add( ACTIONS::showFootprintEditor );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::annotate );
    m_mainToolBar->Add( EE_ACTIONS::runERC );
    m_mainToolBar->Add( EE_ACTIONS::assignFootprints );
    m_mainToolBar->Add( EE_ACTIONS::editSymbolFields );
    m_mainToolBar->Add( EE_ACTIONS::generateBOM );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( EE_ACTIONS::showPcbNew );

    // after adding the tools to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();
}


/* Create Vertical Right Toolbar
 */
void SCH_EDIT_FRAME::ReCreateVToolbar()
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
    m_drawToolBar->Add( ACTIONS::selectionTool,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::highlightNetTool,       ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( EE_ACTIONS::placeSymbol,            ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placePower,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawWire,               ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawBus,                ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeBusWireEntry,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeNoConnect,         ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeJunction,          ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeLabel,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeGlobalLabel,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeHierLabel,         ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::drawSheet,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::importSheetPin,         ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( EE_ACTIONS::drawLines,              ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeSchematicText,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( EE_ACTIONS::placeImage,             ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::deleteTool,                ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->KiRealize();
}


/* Create Vertical Left Toolbar (Option Toolbar)
 */
void SCH_EDIT_FRAME::ReCreateOptToolbar()
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

    m_optionsToolBar->Add( ACTIONS::toggleGrid,          ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,         ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,           ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::toggleHiddenPins, ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( EE_ACTIONS::toggleForceHV,    ACTION_TOOLBAR::TOGGLE );

    EE_SELECTION_TOOL* selTool = m_toolManager->GetTool<EE_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_optionsToolBar->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );

    m_optionsToolBar->KiRealize();
}
