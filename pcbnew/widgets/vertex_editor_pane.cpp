/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "vertex_editor_pane.h"

#include <limits>

#include <wx/aui/aui.h>
#include <wx/grid.h>
#include <wx/settings.h>
#include <wx/sizer.h>
#include <wx/translation.h>

#include <board_item.h>
#include <class_draw_panel_gal.h>
#include <eda_units.h>
#include <pcb_base_edit_frame.h>
#include <pcb_shape.h>
#include <tool/edit_points.h>
#include <tool/point_editor_behavior.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include <zone.h>

#include <view/view.h>

PCB_VERTEX_EDITOR_PANE::PCB_VERTEX_EDITOR_PANE( PCB_BASE_EDIT_FRAME* aFrame ) :
        wxPanel( aFrame ),
        m_frame( aFrame ),
        m_item( nullptr ),
        m_zone( nullptr ),
        m_shape( nullptr ),
        m_grid( nullptr ),
        m_updatingGrid( false )
{
    wxBoxSizer* topSizer = new wxBoxSizer( wxVERTICAL );
    m_grid = new wxGrid( this, wxID_ANY );
    m_grid->CreateGrid( 0, 2 );
    m_grid->SetRowLabelSize( 0 );
    m_grid->SetColLabelValue( 0, _( "X coord" ) );
    m_grid->SetColLabelValue( 1, _( "Y coord" ) );
    m_grid->EnableDragGridSize( false );
    m_grid->EnableDragRowSize( false );
    m_grid->EnableDragColSize( false );
    m_grid->SetSelectionMode( wxGrid::wxGridSelectRows );
    m_grid->SetDefaultCellAlignment( wxALIGN_LEFT, wxALIGN_CENTER );
    m_grid->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_ALWAYS );
    m_grid->EnableScrolling( false, true );
    m_grid->SetColMinimalWidth( 0, FromDIP( 120 ) );
    m_grid->SetColMinimalWidth( 1, FromDIP( 120 ) );

    topSizer->Add( m_grid, 1, wxEXPAND | wxALL, FromDIP( 5 ) );

    SetSizer( topSizer );

    Bind( wxEVT_SIZE, &PCB_VERTEX_EDITOR_PANE::OnSize, this );
    m_grid->Bind( wxEVT_GRID_CELL_CHANGED, &PCB_VERTEX_EDITOR_PANE::OnGridCellChange, this );
    m_grid->Bind( wxEVT_GRID_SELECT_CELL, &PCB_VERTEX_EDITOR_PANE::OnGridSelectCell, this );

    // Initial column sizing
    resizeColumns();
}

PCB_VERTEX_EDITOR_PANE::~PCB_VERTEX_EDITOR_PANE()
{
    if( m_frame )
        m_frame->OnVertexEditorPaneClosed( this );
}


void PCB_VERTEX_EDITOR_PANE::SetItem( BOARD_ITEM* aItem )
{
    m_item = aItem;
    m_zone = dynamic_cast<ZONE*>( aItem );
    m_shape = dynamic_cast<PCB_SHAPE*>( aItem );

    refreshGrid();

    if( m_grid->GetNumberRows() > 0 )
    {
        m_grid->SetGridCursor( 0, 0 );
        m_grid->SelectRow( 0 );
        updateHighlight( 0 );
    }

    resizeColumns();
}


void PCB_VERTEX_EDITOR_PANE::ClearItem()
{
    m_item = nullptr;
    m_zone = nullptr;
    m_shape = nullptr;

    m_updatingGrid = true;
    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );
    m_rows.clear();
    m_updatingGrid = false;
}


void PCB_VERTEX_EDITOR_PANE::OnSelectionChanged( BOARD_ITEM* aNewItem )
{
    // Check if the new item is a polygon or zone
    if( aNewItem )
    {
        ZONE* zone = dynamic_cast<ZONE*>( aNewItem );
        PCB_SHAPE* shape = dynamic_cast<PCB_SHAPE*>( aNewItem );

        bool isPolygon = ( zone != nullptr ) || ( shape && shape->GetShape() == SHAPE_T::POLY );

        if( isPolygon )
        {
            // Update to the new item
            SetItem( aNewItem );
        }
        else
        {
            // Not a polygon/zone, clear the editor
            ClearItem();
        }
    }
    else
    {
        // Nothing selected, clear the editor
        ClearItem();
    }
}

void PCB_VERTEX_EDITOR_PANE::refreshGrid()
{
    m_updatingGrid = true;

    if( m_grid->GetNumberRows() > 0 )
        m_grid->DeleteRows( 0, m_grid->GetNumberRows() );

    m_rows.clear();

    SHAPE_POLY_SET* poly = getPoly();

    if( poly )
    {
        for( auto it = poly->CIterateWithHoles(); it; ++it )
        {
            m_rows.push_back( it.GetIndex() );
            m_grid->AppendRows( 1 );
            updateRow( m_grid->GetNumberRows() - 1 );
        }
    }

    m_updatingGrid = false;
}

void PCB_VERTEX_EDITOR_PANE::updateRow( int aRow )
{
    if( aRow < 0 || aRow >= m_grid->GetNumberRows() )
        return;

    SHAPE_POLY_SET* poly = getPoly();

    if( !poly || aRow >= (int) m_rows.size() )
        return;

    const SHAPE_POLY_SET::VERTEX_INDEX& idx = m_rows[aRow];
    VECTOR2I vertex = poly->CVertex( idx );

    m_updatingGrid = true;
    m_grid->SetCellValue( aRow, 0, formatCoord( vertex.x, ORIGIN_TRANSFORMS::ABS_X_COORD ) );
    m_grid->SetCellValue( aRow, 1, formatCoord( vertex.y, ORIGIN_TRANSFORMS::ABS_Y_COORD ) );
    m_updatingGrid = false;
}

void PCB_VERTEX_EDITOR_PANE::updateHighlight( int aRow )
{
    // Highlighting is now handled by the point editor tool
    // We just refresh the view
    if( m_frame && m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->GetView()->Update( m_item );
        m_frame->GetCanvas()->Refresh();
    }
}


void PCB_VERTEX_EDITOR_PANE::onPolygonModified()
{
    if( m_zone )
    {
        m_zone->UnFill();
        m_zone->SetNeedRefill( true );
        m_zone->HatchBorder();
    }

    if( m_frame && m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->GetView()->Update( m_item );
        m_frame->GetCanvas()->Refresh();
    }
}

bool PCB_VERTEX_EDITOR_PANE::parseCellValue( const wxString& aText,
                                             ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType,
                                             int& aResult ) const
{
    long long displayValue = EDA_UNIT_UTILS::UI::ValueFromString( pcbIUScale, m_frame->GetUserUnits(), aText );

    if( !displayValue )
        return false;

    long long internal = m_frame->GetOriginTransforms().FromDisplay( displayValue, aCoordType );

    if( internal < std::numeric_limits<int>::min() || internal > std::numeric_limits<int>::max() )
        return false;

    aResult = static_cast<int>( internal );
    return true;
}

wxString PCB_VERTEX_EDITOR_PANE::formatCoord( int aValue,
                                              ORIGIN_TRANSFORMS::COORD_TYPES_T aCoordType ) const
{
    int displayValue = m_frame->GetOriginTransforms().ToDisplay( aValue, aCoordType );
    return m_frame->MessageTextFromValue( displayValue );
}

SHAPE_POLY_SET* PCB_VERTEX_EDITOR_PANE::getPoly()
{
    if( m_zone )
        return m_zone->Outline();

    if( m_shape && m_shape->GetShape() == SHAPE_T::POLY )
        return &m_shape->GetPolyShape();

    return nullptr;
}

const SHAPE_POLY_SET* PCB_VERTEX_EDITOR_PANE::getPoly() const
{
    if( m_zone )
        return m_zone->Outline();

    if( m_shape && m_shape->GetShape() == SHAPE_T::POLY )
        return &m_shape->GetPolyShape();

    return nullptr;
}


void PCB_VERTEX_EDITOR_PANE::OnGridCellChange( wxGridEvent& aEvent )
{
    if( m_updatingGrid )
        return;

    int row = aEvent.GetRow();
    int col = aEvent.GetCol();

    if( row < 0 || row >= (int) m_rows.size() )
    {
        updateHighlight( -1 );
        return;
    }

    SHAPE_POLY_SET* poly = getPoly();

    if( !poly || !m_item )
        return;

    const SHAPE_POLY_SET::VERTEX_INDEX& idx = m_rows[row];
    VECTOR2I vertex = poly->CVertex( idx );
    int newValue;

    ORIGIN_TRANSFORMS::COORD_TYPES_T coordType =
            ( col == 0 ) ? ORIGIN_TRANSFORMS::ABS_X_COORD : ORIGIN_TRANSFORMS::ABS_Y_COORD;

    if( !parseCellValue( m_grid->GetCellValue( row, col ), coordType, newValue ) )
    {
        updateRow( row );
        return;
    }

    int& target = ( col == 0 ) ? vertex.x : vertex.y;

    if( target == newValue )
    {
        updateRow( row );
        return;
    }

    // Create a new commit for each edit
    BOARD_COMMIT commit( m_frame );
    commit.Modify( m_item );

    target = newValue;
    poly->SetVertex( idx, vertex );

    if( m_zone )
    {
        m_zone->UnFill();
        m_zone->SetNeedRefill( true );
        m_zone->HatchBorder();
    }

    commit.Push( _( "Edit Vertex" ) );

    // Update the view after the commit
    if( m_frame && m_frame->GetCanvas() )
    {
        m_frame->GetCanvas()->GetView()->Update( m_item );
        m_frame->GetCanvas()->Refresh();
    }

    updateRow( row );
    updateHighlight( row );
}

void PCB_VERTEX_EDITOR_PANE::OnGridSelectCell( wxGridEvent& aEvent )
{
    updateHighlight( aEvent.GetRow() );
    aEvent.Skip();
}


void PCB_VERTEX_EDITOR_PANE::resizeColumns()
{
    if( !m_grid || m_grid->GetNumberCols() != 2 )
        return;

    // Get the available width (excluding row labels and scrollbar)
    int width = m_grid->GetClientSize().GetWidth() - m_grid->GetRowLabelSize();

    // Reserve space for vertical scrollbar if it's shown
    if( m_grid->GetScrollRange( wxVERTICAL ) > 0 )
        width -= wxSystemSettings::GetMetric( wxSYS_VSCROLL_X );

    // Split the width equally between the two columns
    int colWidth = width / 2;

    if( colWidth > m_grid->GetColMinimalAcceptableWidth() )
    {
        m_grid->SetColSize( 0, colWidth );
        m_grid->SetColSize( 1, colWidth );
    }
}


void PCB_VERTEX_EDITOR_PANE::OnSize( wxSizeEvent& aEvent )
{
    resizeColumns();
    aEvent.Skip();
}
