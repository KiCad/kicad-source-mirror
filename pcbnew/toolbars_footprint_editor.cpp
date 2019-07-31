/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 2012 Wayne Stambaugh <stambaughw@verizon.net>
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <tool/actions.h>
#include <pcbnew.h>
#include <class_board.h>
#include <footprint_edit_frame.h>
#include <dialog_helpers.h>
#include <pcbnew_id.h>
#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tools/pcb_actions.h>

void FOOTPRINT_EDIT_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_zoomSelectBox and m_gridSelectBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    if( m_mainToolBar )
        m_mainToolBar->Clear();
    else
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT );

    wxString msg;

    // Set up toolbar
    m_mainToolBar->Add( PCB_ACTIONS::newFootprint );
#ifdef KICAD_SCRIPTING
    m_mainToolBar->Add( PCB_ACTIONS::createFootprint );
#endif
    
    if( IsCurrentFPFromBoard() )
        m_mainToolBar->Add( PCB_ACTIONS::saveToBoard );
    else
        m_mainToolBar->Add( PCB_ACTIONS::saveToLibrary );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::print );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( ACTIONS::undo );
    m_mainToolBar->Add( ACTIONS::redo );

    m_mainToolBar->AddSeparator();
    m_mainToolBar->Add( ACTIONS::zoomRedraw );
    m_mainToolBar->Add( ACTIONS::zoomInCenter );
    m_mainToolBar->Add( ACTIONS::zoomOutCenter );
    m_mainToolBar->Add( ACTIONS::zoomFitScreen );
    m_mainToolBar->Add( ACTIONS::zoomTool, ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->Add( PCB_ACTIONS::footprintProperties );
    m_mainToolBar->Add( PCB_ACTIONS::defaultPadProperties );

    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_LOAD_MODULE_FROM_BOARD, wxEmptyString,
                            KiScaledBitmap( load_module_board_xpm, this ),
                            _( "Load footprint from current board" ) );

    m_mainToolBar->AddTool( ID_ADD_FOOTPRINT_TO_BOARD, wxEmptyString,
                            KiScaledBitmap( export_xpm, this ),
                            _( "Insert footprint into current board" ) );

#if 0       // Currently there is no check footprint function defined, so do not show this tool
    KiScaledSeparator( m_mainToolBar, this );
    m_mainToolBar->AddTool( ID_MODEDIT_CHECK, wxEmptyString,
                            KiScaledBitmap( module_check_xpm, this ),
                            _( "Check footprint" ) );
#endif

    KiScaledSeparator( m_mainToolBar, this );

    // Grid selection choice box.
    if( m_gridSelectBox == nullptr )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    KiScaledSeparator( m_mainToolBar, this );

    // Zoom selection choice box.
    if( m_zoomSelectBox == nullptr )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    updateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateVToolbar()
{
    if( m_drawToolBar )
        m_drawToolBar->Clear();
    else
        m_drawToolBar = new ACTION_TOOLBAR( this, ID_V_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_drawToolBar->Add( ACTIONS::selectionTool,      ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( PCB_ACTIONS::placePad,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawLine,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawCircle,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawArc,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawPolygon,    ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::placeText,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::deleteTool,         ACTION_TOOLBAR::TOGGLE );

    KiScaledSeparator( m_drawToolBar, this );
    m_drawToolBar->Add( PCB_ACTIONS::setAnchor,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::gridSetOrigin,  ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::measureTool,        ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
        m_optionsToolBar->Clear();
    else
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );

    m_optionsToolBar->Add( ACTIONS::toggleGrid,              ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::togglePolarCoords,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::imperialUnits,           ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::metricUnits,             ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,       ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->Add( PCB_ACTIONS::padDisplayMode,      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::moduleEdgeOutlines,  ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::highContrastMode,        ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddSeparator();
    m_optionsToolBar->Add( PCB_ACTIONS::toggleFootprintTree, ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->Realize();
}


void FOOTPRINT_EDIT_FRAME::SyncToolbars()
{
#define TOGGLE_TOOL( toolbar, tool ) toolbar->Toggle( tool, IsCurrentTool( tool ) )

    PCB_DISPLAY_OPTIONS* opts = (PCB_DISPLAY_OPTIONS*) GetDisplayOptions();

    if( IsCurrentFPFromBoard() )
        m_mainToolBar->Toggle( PCB_ACTIONS::saveToBoard,   GetScreen() && GetScreen()->IsModify() );
    else
        m_mainToolBar->Toggle( PCB_ACTIONS::saveToLibrary, GetScreen() && GetScreen()->IsModify() );

    m_mainToolBar->Toggle( ACTIONS::undo, GetScreen() && GetScreen()->GetUndoCommandCount() > 0 );
    m_mainToolBar->Toggle( ACTIONS::redo, GetScreen() && GetScreen()->GetRedoCommandCount() > 0 );
    TOGGLE_TOOL( m_mainToolBar, ACTIONS::zoomTool );
    m_mainToolBar->Toggle( PCB_ACTIONS::footprintProperties, GetBoard()->GetFirstModule() );
    m_mainToolBar->Refresh();

    m_optionsToolBar->Toggle( ACTIONS::toggleGrid,              IsGridVisible() );
    m_optionsToolBar->Toggle( ACTIONS::metricUnits,             GetUserUnits() != INCHES );
    m_optionsToolBar->Toggle( ACTIONS::imperialUnits,           GetUserUnits() == INCHES );
    m_optionsToolBar->Toggle( ACTIONS::togglePolarCoords,       GetShowPolarCoords() );
    m_optionsToolBar->Toggle( PCB_ACTIONS::padDisplayMode,      !opts->m_DisplayPadFill );
    m_optionsToolBar->Toggle( PCB_ACTIONS::moduleEdgeOutlines,  !opts->m_DisplayModEdgeFill );
    m_optionsToolBar->Toggle( ACTIONS::highContrastMode,        opts->m_ContrastModeDisplay );
    m_optionsToolBar->Toggle( PCB_ACTIONS::toggleFootprintTree, IsSearchTreeShown() );
    m_optionsToolBar->Refresh();

    TOGGLE_TOOL( m_drawToolBar, ACTIONS::selectionTool );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::placePad );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::drawLine );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::drawCircle );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::drawArc );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::drawPolygon );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::placeText );
    TOGGLE_TOOL( m_drawToolBar, ACTIONS::deleteTool );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::setAnchor );
    TOGGLE_TOOL( m_drawToolBar, PCB_ACTIONS::gridSetOrigin );
    TOGGLE_TOOL( m_drawToolBar, ACTIONS::measureTool );
    m_drawToolBar->Refresh();
}
