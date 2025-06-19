/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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

#include <pcb_base_edit_frame.h>
#include <board.h>
#include <grid_layer_box_helpers.h>
#include <kiplatform/ui.h>
#include <widgets/wx_grid.h>
#include "dialog_swap_layers.h"


class LAYER_GRID_TABLE : public wxGridTableBase
{
    std::vector<std::pair<PCB_LAYER_ID, PCB_LAYER_ID>> m_layers;
    int m_layerCount;

public:
    LAYER_GRID_TABLE( int layerCount ) : m_layerCount( layerCount )
    { }

    int GetNumberRows() override { return m_layerCount; }
    int GetNumberCols() override { return 2; }

    wxString GetColLabelValue( int aCol ) override
    {
        switch( aCol )
        {
        case 0: return _( "Move items on:" );
        case 1: return _( "To layer:" );
        default: return wxEmptyString;
        }
    }

    wxString GetValue( int row, int col ) override { return "undefined"; }
    void SetValue( int row, int col, const wxString& value ) override { }

    long GetValueAsLong( int row, int col ) override
    {
        if( row < 0 || row >= m_layerCount )
            return -1;

        if( col < 0 || col >= 2 )
            return -1;

        return col == 0 ? m_layers[ row ].first : m_layers[ row ].second;
    }

    void SetValueAsLong( int row, int col, long value ) override
    {
        if( row < 0 || col < 0 || col >= 2 )
            return;

        // Ensure there is room in m_layers to store value
        while( row >= (int)m_layers.size() )
        {
            m_layers.emplace_back( F_Cu, F_Cu );
        }

        col == 0 ? m_layers[row].first = ToLAYER_ID( value )
                 : m_layers[row].second = ToLAYER_ID( value );
    }
};


DIALOG_SWAP_LAYERS::DIALOG_SWAP_LAYERS( PCB_BASE_EDIT_FRAME* aParent,
                                        std::map<PCB_LAYER_ID, PCB_LAYER_ID>& aLayerMap ) :
        DIALOG_SWAP_LAYERS_BASE( aParent ),
        m_parent( aParent ),
        m_layerMap( aLayerMap )
{
    m_gridTable = new LAYER_GRID_TABLE( m_parent->GetBoard()->GetCopperLayerCount() );
    m_grid->SetTable( m_gridTable );
    m_grid->SetMinSize( FromDIP( m_grid->GetMinSize() ) );
    m_grid->SetCellHighlightROPenWidth( 0 );
    m_grid->SetUseNativeColLabels();

    SetupStandardButtons();

    finishDialogSettings();
}


DIALOG_SWAP_LAYERS::~DIALOG_SWAP_LAYERS()
{
    m_grid->DestroyTable( m_gridTable );
}


bool DIALOG_SWAP_LAYERS::TransferDataToWindow()
{
    LSET enabledCopperLayers = LSET::AllCuMask( m_parent->GetBoard()->GetCopperLayerCount() );
    int row = 0;

    LSEQ enabledCopperLayerUIList = enabledCopperLayers.UIOrder();

    for( PCB_LAYER_ID layer : enabledCopperLayerUIList )
    {
        auto attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_parent ) );
        attr->SetBackgroundColour( wxSystemSettings::GetColour( wxSYS_COLOUR_MENU ) );
        attr->SetReadOnly();
        m_grid->SetAttr( row, 0, attr );

        attr = new wxGridCellAttr;
        attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( m_parent ) );
        attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( m_parent, LSET::AllNonCuMask() ) );
        m_grid->SetAttr( row, 1, attr );
        m_grid->GetTable()->SetValueAsLong( row, 0, (long) layer );
        m_grid->GetTable()->SetValueAsLong( row, 1, (long) layer );

        ++row;
    }

    return true;
}


bool DIALOG_SWAP_LAYERS::TransferDataFromWindow()
{
    if( !m_grid->CommitPendingChanges() )
        return false;

    LSET enabledCopperLayers = LSET::AllCuMask( m_parent->GetBoard()->GetCopperLayerCount() );
    wxGridTableBase* table = m_grid->GetTable();
    int row = 0;

    LSEQ enabledCopperLayerUIList = enabledCopperLayers.UIOrder();

    for( PCB_LAYER_ID layer : enabledCopperLayerUIList )
    {
        int dest = table->GetValueAsLong( row++, 1 );

        if( dest >= 0 && dest < PCB_LAYER_ID_COUNT && enabledCopperLayers.test( dest ) )
            m_layerMap[ layer ] = ToLAYER_ID( dest );
    }

    return true;
}


void DIALOG_SWAP_LAYERS::adjustGridColumns()
{
    // Account for scroll bars
    int width = KIPLATFORM::UI::GetUnobscuredSize( m_grid ).x;

    m_grid->SetColSize( 0, std::max( FromDIP( 40 ), width / 2 ) );
    m_grid->SetColSize( 1, std::max( FromDIP( 40 ), width - m_grid->GetColSize( 0 ) ) );
}


void DIALOG_SWAP_LAYERS::OnSize( wxSizeEvent& event )
{
    adjustGridColumns();

    event.Skip();
}


