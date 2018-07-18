/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
 * Copyright (C) 1992-2018 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <fctsys.h>
#include <pcb_edit_frame.h>
#include <class_drawpanel.h>
#include <class_board.h>
#include <connectivity_data.h>
#include <view/view.h>
#include <pcb_layer_box_selector.h>

#include "dialog_global_edit_tracks_and_vias_base.h"

// Columns of netclasses grid
enum {
    GRID_NAME = 0,
    GRID_TRACKSIZE,
    GRID_VIASIZE,
    GRID_VIADRILL,
    GRID_uVIASIZE,
    GRID_uVIADRILL,
    GRID_DIFF_PAIR_WIDTH,        // not currently included in grid
    GRID_DIFF_PAIR_GAP,          // not currently included in grid
    GRID_DIFF_PAIR_VIA_GAP       // not currently included in grid
};


class DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS : public DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE
{
private:
    PCB_EDIT_FRAME* m_parent;
    BOARD*          m_brd;
    int*            m_originalColWidths;

public:
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent );
    ~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS() override;

private:
    void visitItem( PICKED_ITEMS_LIST* aUndoList, TRACK* aItem );
    void processItem( PICKED_ITEMS_LIST* aUndoList, TRACK* aItem );

    bool TransferDataFromWindow() override;

    void OnUpdateUI( wxUpdateUIEvent& event ) override;
    void OnSizeNetclassGrid( wxSizeEvent& event ) override;
    void AdjustNetclassGridColumns( int aWidth );

    void OnNetFilterSelect( wxCommandEvent& event ) override
    {
        m_netFilterOpt->SetValue( true );
    }
    void OnNetclassFilterSelect( wxCommandEvent& event ) override
    {
        m_netclassFilterOpt->SetValue( true );
    }
    void OnLayerFilterSelect( wxCommandEvent& event ) override
    {
        m_layerFilterOpt->SetValue( true );
    }

    void buildNetclassesGrid();
    void buildFilterLists();
};


DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent ) :
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( aParent )
{
    m_parent  = aParent;
    m_brd = m_parent->GetBoard();

    m_originalColWidths = new int[ m_netclassGrid->GetNumberCols() ];

    for( int i = 0; i < m_netclassGrid->GetNumberCols(); ++i )
        m_originalColWidths[ i ] = m_netclassGrid->GetColSize( i );

    buildFilterLists();

    m_parent->UpdateTrackWidthSelectBox( m_trackWidthSelectBox );
    m_parent->UpdateViaSizeSelectBox( m_viaSizesSelectBox );

    m_layerBox->SetBoardFrame( m_parent );
    m_layerBox->SetLayersHotkeys( false );
    m_layerBox->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_layerBox->Resync();

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_netclassGrid->SetDefaultCellFont( infoFont );
    buildNetclassesGrid();

    m_netclassGrid->SetCellHighlightPenWidth( 0 );
    m_sdbSizerOK->SetDefault();

    FinishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS()
{
    delete[] m_originalColWidths;
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::buildFilterLists()
{
    int currentNet = m_brd->GetHighLightNetCode();

    if( currentNet < 0 && m_parent->GetCurItem() && m_parent->GetCurItem()->IsConnected() )
        currentNet = static_cast<BOARD_CONNECTED_ITEM*>( m_parent->GetCurItem() )->GetNetCode();

    wxString currentNetClass = m_brd->GetDesignSettings().GetCurrentNetClassName();
    LAYER_NUM currentLayer = m_parent->GetActiveLayer();

    // Populate the net filter list with net names
    for( unsigned netcode = 0; netcode < m_brd->GetNetCount(); netcode++ )
    {
        wxString netname;

        if( netcode == 0 )  // netcode 0 is the netcode of not connected items
            netname = "<no net>";
        else
            netname = m_brd->GetNetInfo().GetNetItem( netcode )->GetNetname();

        m_netFilter->Append( netname );

        if( (int) netcode == currentNet )
            m_netFilter->SetSelection( m_netFilter->GetCount() - 1 );
    }

    // Populate the netclass filter list with netclass names
    NETCLASSES& netclasses = m_brd->GetDesignSettings().m_NetClasses;

    m_netclassFilter->Append( netclasses.GetDefault()->GetName() );

    for( NETCLASSES::const_iterator nc = netclasses.begin(); nc != netclasses.end(); ++nc )
        m_netclassFilter->Append( nc->second->GetName() );

    m_netclassFilter->SetSelection( m_netclassFilter->FindString( currentNetClass ) );

    // Populate the layer filter list
    m_layerFilter->SetBoardFrame( m_parent );
    m_layerFilter->SetLayersHotkeys( false );
    m_layerFilter->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_layerFilter->Resync();

    m_layerFilter->SetLayerSelection( currentLayer );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::buildNetclassesGrid()
{
#define SET_NETCLASS_VALUE( aRow, aGrid, aCol, aValue ) \
        aGrid->SetCellValue( aRow, aCol, StringFromValue( GetUserUnits(), aValue, true, true ) )

    m_netclassGrid->SetCellValue( 0, GRID_NAME, wxEmptyString );
    m_netclassGrid->SetCellValue( 0, GRID_TRACKSIZE, _( "Track width" ) );
    m_netclassGrid->SetCellValue( 0, GRID_VIASIZE, _( "Via size" ) );
    m_netclassGrid->SetCellValue( 0, GRID_VIADRILL, _( "Via drill" ) );
    m_netclassGrid->SetCellValue( 0, GRID_uVIASIZE, _( "uVia size" ) );
    m_netclassGrid->SetCellValue( 0, GRID_uVIADRILL, _( "uVia drill" ) );

    NETCLASSES& netclasses = m_brd->GetDesignSettings().m_NetClasses;
    m_netclassGrid->AppendRows( netclasses.GetCount() );
    int row = 1;

    for( NETCLASSES::const_iterator nc = netclasses.begin(); nc != netclasses.end(); ++nc, ++row )
    {
        NETCLASSPTR netclass = nc->second;
        m_netclassGrid->SetCellValue( row, GRID_NAME, netclass->GetName() );
        SET_NETCLASS_VALUE( row, m_netclassGrid, GRID_TRACKSIZE, netclass->GetTrackWidth() );
        SET_NETCLASS_VALUE( row, m_netclassGrid, GRID_VIASIZE, netclass->GetViaDiameter() );
        SET_NETCLASS_VALUE( row, m_netclassGrid, GRID_VIADRILL, netclass->GetViaDrill() );
        SET_NETCLASS_VALUE( row, m_netclassGrid, GRID_uVIASIZE, netclass->GetuViaDiameter() );
        SET_NETCLASS_VALUE( row, m_netclassGrid, GRID_uVIADRILL, netclass->GetuViaDrill() );
    }
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnUpdateUI( wxUpdateUIEvent&  )
{
    m_trackWidthSelectBox->Enable( m_setToSpecifiedValues->GetValue() );
    m_viaSizesSelectBox->Enable( m_setToSpecifiedValues->GetValue() );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::processItem( PICKED_ITEMS_LIST* aUndoList, TRACK* aItem )
{
    BOARD_DESIGN_SETTINGS& brdSettings = m_brd->GetDesignSettings();

    if( m_setToSpecifiedValues->GetValue() )
    {
        unsigned int prevTrackWidthIndex = brdSettings.GetTrackWidthIndex();
        unsigned int prevViaSizeIndex = brdSettings.GetViaSizeIndex();

        brdSettings.SetTrackWidthIndex( (unsigned) m_trackWidthSelectBox->GetSelection() );
        brdSettings.SetViaSizeIndex( (unsigned) m_viaSizesSelectBox->GetSelection() );

        m_parent->SetTrackSegmentWidth( aItem, aUndoList, false );

        if( m_layerBox->GetLayerSelection() != UNDEFINED_LAYER && aItem->Type() == PCB_TRACE_T )
        {
            if( aUndoList->FindItem( aItem ) < 0 )
            {
                ITEM_PICKER picker( aItem, UR_CHANGED );
                picker.SetLink( aItem->Clone() );
                aUndoList->PushItem( picker );
            }

            aItem->SetLayer( ToLAYER_ID( m_layerBox->GetLayerSelection() ) );
            m_parent->GetBoard()->GetConnectivity()->Update( aItem );
        }

        brdSettings.SetTrackWidthIndex( prevTrackWidthIndex );
        brdSettings.SetViaSizeIndex( prevViaSizeIndex );
    }
    else
    {
        m_parent->SetTrackSegmentWidth( aItem, aUndoList, true );
    }
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::visitItem( PICKED_ITEMS_LIST* aUndoList, TRACK* aItem )
{
    if( m_netFilterOpt->GetValue() )
    {
        if( aItem->GetNetCode() != m_netFilter->GetSelection() )
            return;
    }

    if( m_netclassFilterOpt->GetValue() )
    {
        if( aItem->GetNetClassName() != m_netclassFilter->GetStringSelection() )
            return;
    }

    if( m_layerFilterOpt->GetValue() && m_layerFilter->GetLayerSelection() != UNDEFINED_LAYER )
    {
        if( aItem->GetLayer() != m_layerFilter->GetLayerSelection() )
            return;
    }

    processItem( aUndoList, aItem );
}


bool DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    PICKED_ITEMS_LIST itemsListPicker;
    wxBusyCursor dummy;

    // Examine segments
    for( TRACK* segment = m_brd->m_Track; segment != nullptr; segment = segment->Next() )
    {
        if( m_tracks->GetValue() && segment->Type() == PCB_TRACE_T )
            visitItem( &itemsListPicker, segment );
        else if (m_vias->GetValue() && segment->Type() == PCB_VIA_T )
            visitItem( &itemsListPicker, segment );
    }

    if( itemsListPicker.GetCount() > 0 )
    {
        m_parent->SaveCopyInUndoList( itemsListPicker, UR_CHANGED );

        if( m_parent->IsGalCanvasActive() )
        {
            for( TRACK* segment = m_brd->m_Track; segment != nullptr; segment = segment->Next() )
                m_parent->GetGalCanvas()->GetView()->Update( segment );
        }
        else
        {
            m_parent->GetCanvas()->Refresh();
        }
    }

    return true;
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::AdjustNetclassGridColumns( int aWidth )
{
    for( int i = 1; i < m_netclassGrid->GetNumberCols(); i++ )
    {
        m_netclassGrid->SetColSize( i, m_originalColWidths[ i ] );
        aWidth -= m_originalColWidths[ i ];
    }

    m_netclassGrid->SetColSize( 0, aWidth );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnSizeNetclassGrid( wxSizeEvent& event )
{
    AdjustNetclassGridColumns( event.GetSize().GetX() );

    event.Skip();
}


void PCB_EDIT_FRAME::OnEditTracksAndVias( wxCommandEvent& event )
{
    DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS dlg( this );
    dlg.ShowModal();
}

