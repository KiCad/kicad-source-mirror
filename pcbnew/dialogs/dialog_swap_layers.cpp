/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <class_board.h>
#include <grid_layer_box_helpers.h>
#include <board_commit.h>
#include <class_drawsegment.h>
#include <class_drawpanel.h>
#include <class_track.h>
#include <view/view.h>
#include <widgets/wx_grid.h>
#include <class_zone.h>

#include "dialog_swap_layers_base.h"


class LAYER_GRID_TABLE : public wxGridTableBase
{
    int m_layers[MAX_CU_LAYERS][2];
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
        return m_layers[ row ][ col ];
    }

    void SetValueAsLong( int row, int col, long value ) override
    {
        m_layers[ row ][ col ] = value;
    }
};


class DIALOG_SWAP_LAYERS : public DIALOG_SWAP_LAYERS_BASE
{
private:
    PCB_EDIT_FRAME*   m_parent;
    PCB_LAYER_ID*     m_layerDestinations;

    LAYER_GRID_TABLE* m_gridTable;

public:
    DIALOG_SWAP_LAYERS( PCB_EDIT_FRAME* aParent, PCB_LAYER_ID* aArray );
    ~DIALOG_SWAP_LAYERS() override;

private:
    bool TransferDataToWindow() override;
    bool TransferDataFromWindow() override;

    void OnSize( wxSizeEvent& event ) override;

    void adjustGridColumns( int aWidth );
};


DIALOG_SWAP_LAYERS::DIALOG_SWAP_LAYERS( PCB_EDIT_FRAME* aParent, PCB_LAYER_ID* aArray ) :
    DIALOG_SWAP_LAYERS_BASE( aParent ),
    m_parent( aParent ),
    m_layerDestinations( aArray )
{
    m_gridTable = new LAYER_GRID_TABLE( m_parent->GetBoard()->GetCopperLayerCount() );
    m_grid->SetTable( m_gridTable );
    m_grid->SetDefaultRowSize( m_grid->GetDefaultRowSize() + 4 );
    m_grid->SetCellHighlightROPenWidth( 0 );

    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}


DIALOG_SWAP_LAYERS::~DIALOG_SWAP_LAYERS()
{
    m_grid->DestroyTable( m_gridTable );
}


bool DIALOG_SWAP_LAYERS::TransferDataToWindow()
{
    LSET enabledCopperLayers = LSET::AllCuMask( m_parent->GetBoard()->GetCopperLayerCount() );
    int row = 0;

    for( size_t layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( enabledCopperLayers.test( layer ) )
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

    for( size_t layer = 0; layer < PCB_LAYER_ID_COUNT; ++layer )
    {
        if( enabledCopperLayers.test( layer ) )
            m_layerDestinations[ layer ] = (PCB_LAYER_ID) table->GetValueAsLong( row++, 1 );
        else
            m_layerDestinations[ layer ] = (PCB_LAYER_ID) layer;
    }

    return true;
}


void DIALOG_SWAP_LAYERS::adjustGridColumns( int aWidth )
{
    // Account for scroll bars
    aWidth -= ( m_grid->GetSize().x - m_grid->GetClientSize().x );

    m_grid->SetColSize( 0, aWidth / 2 );
    m_grid->SetColSize( 1, aWidth - m_grid->GetColSize( 0 ) );
}


void DIALOG_SWAP_LAYERS::OnSize( wxSizeEvent& event )
{
    adjustGridColumns( event.GetSize().GetX() );

    event.Skip();
}


bool processBoardItem( PCB_EDIT_FRAME* aFrame, BOARD_COMMIT& commit, BOARD_ITEM* aItem,
                       PCB_LAYER_ID* new_layer )
{
    if( new_layer[ aItem->GetLayer() ] != aItem->GetLayer() )
    {
        commit.Modify( aItem );
        aItem->SetLayer( new_layer[ aItem->GetLayer() ] );
        aFrame->GetGalCanvas()->GetView()->Update( aItem, KIGFX::GEOMETRY );
        return true;
    }

    return false;
}


void PCB_EDIT_FRAME::Swap_Layers( wxCommandEvent& event )
{
    PCB_LAYER_ID new_layer[PCB_LAYER_ID_COUNT];

    DIALOG_SWAP_LAYERS dlg( this, new_layer );

    if( dlg.ShowModal() != wxID_OK )
        return;

    BOARD_COMMIT commit( this );
    bool hasChanges = false;

    // Change tracks.
    for( TRACK* segm = GetBoard()->m_Track;  segm;  segm = segm->Next() )
    {
        if( segm->Type() == PCB_VIA_T )
        {
            VIA*         via = (VIA*) segm;
            PCB_LAYER_ID top_layer, bottom_layer;

            if( via->GetViaType() == VIA_THROUGH )
                continue;

            via->LayerPair( &top_layer, &bottom_layer );

            if( new_layer[bottom_layer] != bottom_layer || new_layer[top_layer] != top_layer )
            {
                commit.Modify( via );
                via->SetLayerPair( new_layer[top_layer], new_layer[bottom_layer] );
                GetGalCanvas()->GetView()->Update( via, KIGFX::GEOMETRY );
                hasChanges = true;
            }
        }
        else
        {
            hasChanges |= processBoardItem( this, commit, segm, new_layer );
        }
    }

    for( TRACK* segm = GetBoard()->m_SegZoneDeprecated; segm; segm = segm->Next() )
    {
        // Note: deprecated zone segment fills only found in very old boards
        hasChanges |= processBoardItem( this, commit, segm, new_layer );
    }

    for( BOARD_ITEM* zone : GetBoard()->Zones() )
    {
        hasChanges |= processBoardItem( this, commit, zone, new_layer );
    }

    for( BOARD_ITEM* drawing : GetBoard()->Drawings() )
    {
        hasChanges |= processBoardItem( this, commit, drawing, new_layer );
    }

    if( hasChanges )
    {
        OnModify();
        commit.Push( "Layers moved" );
        GetCanvas()->Refresh();
    }
}
