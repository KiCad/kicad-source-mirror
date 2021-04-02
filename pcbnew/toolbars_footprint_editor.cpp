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

#include <tool/actions.h>
#include <board.h>
#include <footprint_edit_frame.h>
#include <dialog_helpers.h>
#include <pcbnew_id.h>
#include <bitmaps.h>
#include <tool/action_toolbar.h>
#include <tool/tool_manager.h>
#include <tools/pcb_actions.h>
#include <tools/pcb_selection_tool.h>
#include <pcb_layer_box_selector.h>

void FOOTPRINT_EDIT_FRAME::ReCreateHToolbar()
{
    // Note:
    // To rebuild the aui toolbar, the more easy way is to clear ( calling m_mainToolBar.Clear() )
    // all wxAuiToolBarItems.
    // However the wxAuiToolBarItems are not the owners of controls managed by
    // them ( m_zoomSelectBox and m_gridSelectBox ), and therefore do not delete them
    // So we do not recreate them after clearing the tools.

    if( m_mainToolBar )
    {
        m_mainToolBar->ClearToolbar();
    }
    else
    {
        m_mainToolBar = new ACTION_TOOLBAR( this, ID_H_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                            KICAD_AUI_TB_STYLE | wxAUI_TB_HORZ_LAYOUT | wxAUI_TB_HORIZONTAL);
        m_mainToolBar->SetAuiManager( &m_auimgr );
    }

    wxString msg;

    // Set up toolbar
    m_mainToolBar->Add( PCB_ACTIONS::newFootprint );
#ifdef KICAD_SCRIPTING
    m_mainToolBar->Add( PCB_ACTIONS::createFootprint );
#endif
    m_mainToolBar->Add( ACTIONS::save );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( ACTIONS::print );

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
    m_mainToolBar->Add( PCB_ACTIONS::rotateCcw );
    m_mainToolBar->Add( PCB_ACTIONS::rotateCw );
    m_mainToolBar->Add( PCB_ACTIONS::mirror );
    m_mainToolBar->Add( PCB_ACTIONS::group );
    m_mainToolBar->Add( PCB_ACTIONS::ungroup );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->Add( PCB_ACTIONS::footprintProperties );
    m_mainToolBar->Add( PCB_ACTIONS::defaultPadProperties );
    m_mainToolBar->Add( PCB_ACTIONS::checkFootprint );

    m_mainToolBar->AddScaledSeparator( this );
    m_mainToolBar->AddTool( ID_LOAD_FOOTPRINT_FROM_BOARD, wxEmptyString,
                            KiScaledBitmap( BITMAPS::import_brd_file, this ),
                            _( "Load footprint from current board" ) );

    m_mainToolBar->AddTool( ID_ADD_FOOTPRINT_TO_BOARD, wxEmptyString,
                            KiScaledBitmap( BITMAPS::insert_module_board, this ),
                            _( "Insert footprint into current board" ) );

    m_mainToolBar->AddScaledSeparator( this );

    // Grid selection choice box.
    if( m_gridSelectBox == nullptr )
        m_gridSelectBox = new wxChoice( m_mainToolBar, ID_ON_GRID_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateGridSelectBox();
    m_mainToolBar->AddControl( m_gridSelectBox );

    m_mainToolBar->AddScaledSeparator( this );

    // Zoom selection choice box.
    if( m_zoomSelectBox == nullptr )
        m_zoomSelectBox = new wxChoice( m_mainToolBar, ID_ON_ZOOM_SELECT,
                                    wxDefaultPosition, wxDefaultSize, 0, NULL );

    UpdateZoomSelectBox();
    m_mainToolBar->AddControl( m_zoomSelectBox );

    m_mainToolBar->AddScaledSeparator( this );

    // Layer selection choice box.
    if( m_selLayerBox == nullptr )
    {
        m_selLayerBox = new PCB_LAYER_BOX_SELECTOR( m_mainToolBar, ID_TOOLBARH_PCB_SELECT_LAYER );
        m_selLayerBox->SetBoardFrame( this );

        // Some layers cannot be seclect (they are shown in the layer manager
        // only to set the color and visibility, but not for selection)
        // Disable them in layer box
        m_selLayerBox->SetNotAllowedLayerSet( LSET::ForbiddenFootprintLayers() );
        m_selLayerBox->Resync();
    }

    ReCreateLayerBox( false );
    m_mainToolBar->AddControl( m_selLayerBox );

    // Go through and ensure the comboboxes are the correct size, since the strings in the
    // box could have changed widths.
    m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_PCB_SELECT_LAYER );
    m_mainToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
    m_mainToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );

    // after adding the buttons to the toolbar, must call Realize() to reflect the changes
    m_mainToolBar->KiRealize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateVToolbar()
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

    m_drawToolBar->Add( ACTIONS::selectionTool,       ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( PCB_ACTIONS::placePad,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawLine,        ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawArc,         ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawRectangle,   ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawCircle,      ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawPolygon,     ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::drawRuleArea,    ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::placeText,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::deleteTool,          ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->AddScaledSeparator( this );
    m_drawToolBar->Add( PCB_ACTIONS::setAnchor,       ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( PCB_ACTIONS::gridSetOrigin,   ACTION_TOOLBAR::TOGGLE );
    m_drawToolBar->Add( ACTIONS::measureTool,         ACTION_TOOLBAR::TOGGLE );

    m_drawToolBar->KiRealize();
}


void FOOTPRINT_EDIT_FRAME::ReCreateOptToolbar()
{
    if( m_optionsToolBar )
    {
        m_optionsToolBar->ClearToolbar();
    }
    else
    {
        m_optionsToolBar = new ACTION_TOOLBAR( this, ID_OPT_TOOLBAR, wxDefaultPosition, wxDefaultSize,
                                               KICAD_AUI_TB_STYLE | wxAUI_TB_VERTICAL );
        m_optionsToolBar->SetAuiManager( &m_auimgr );
    }

    m_optionsToolBar->Add( ACTIONS::toggleGrid,              ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::togglePolarCoords,   ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::inchesUnits,             ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::milsUnits,               ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::millimetersUnits,        ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::toggleCursorStyle,       ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::padDisplayMode,      ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::graphicsOutlines,    ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( PCB_ACTIONS::textOutlines,        ACTION_TOOLBAR::TOGGLE );
    m_optionsToolBar->Add( ACTIONS::highContrastMode,        ACTION_TOOLBAR::TOGGLE );

    m_optionsToolBar->AddScaledSeparator( this );
    m_optionsToolBar->Add( PCB_ACTIONS::showFootprintTree,   ACTION_TOOLBAR::TOGGLE );

    PCB_SELECTION_TOOL*          selTool = m_toolManager->GetTool<PCB_SELECTION_TOOL>();
    std::unique_ptr<ACTION_MENU> gridMenu = std::make_unique<ACTION_MENU>( false, selTool );
    gridMenu->Add( ACTIONS::gridProperties );
    m_optionsToolBar->AddToolContextMenu( ACTIONS::toggleGrid, std::move( gridMenu ) );

    m_optionsToolBar->KiRealize();
}


void FOOTPRINT_EDIT_FRAME::UpdateToolbarControlSizes()
{
    if( m_mainToolBar )
    {
        // Update the item widths
        m_mainToolBar->UpdateControlWidth( ID_TOOLBARH_PCB_SELECT_LAYER );
        m_mainToolBar->UpdateControlWidth( ID_ON_ZOOM_SELECT );
        m_mainToolBar->UpdateControlWidth( ID_ON_GRID_SELECT );
    }
}


void FOOTPRINT_EDIT_FRAME::ReCreateLayerBox( bool aForceResizeToolbar )
{
    if( m_selLayerBox == NULL || m_mainToolBar == NULL )
        return;

    m_selLayerBox->SetToolTip( _( "+/- to switch" ) );
    m_selLayerBox->Resync();

    if( aForceResizeToolbar )
        UpdateToolbarControlSizes();
}


void FOOTPRINT_EDIT_FRAME::OnUpdateLayerSelectBox( wxUpdateUIEvent& aEvent )
{
    m_selLayerBox->SetLayerSelection( GetActiveLayer() );
}
