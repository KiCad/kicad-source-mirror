/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2009-2016 Jean-Pierre Charras, jean-pierre.charras at wanadoo.fr
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

#include <algorithm>

#include <pcb_edit_frame.h>
#include <widgets/unit_binder.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_track.h>
#include <pcb_group.h>
#include <connectivity/connectivity_data.h>
#include <pcb_layer_box_selector.h>
#include <tool/tool_manager.h>
#include <tools/pcb_selection_tool.h>
#include <tools/global_edit_tool.h>
#include "dialog_global_edit_tracks_and_vias.h"
#include "magic_enum.hpp"


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


// Globals to remember control settings during a session
static wxString     g_netclassFilter;
static wxString     g_netFilter;


DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS( PCB_EDIT_FRAME* aParent ) :
        DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS_BASE( aParent ),
        m_trackWidthFilter( aParent, nullptr, m_trackWidthFilterCtrl, m_trackWidthFilterUnits ),
        m_viaSizeFilter( aParent, nullptr, m_viaSizeFilterCtrl, m_viaSizeFilterUnits )
{
    m_parent = aParent;
    m_brd = m_parent->GetBoard();

    buildFilterLists();

    m_parent->UpdateTrackWidthSelectBox( m_trackWidthCtrl, false, false );
    m_trackWidthCtrl->Append( INDETERMINATE_ACTION );
    m_parent->UpdateViaSizeSelectBox( m_viaSizesCtrl, false, false );
    m_viaSizesCtrl->Append( INDETERMINATE_ACTION );
    m_annularRingsCtrl->Append( INDETERMINATE_ACTION );

    m_layerCtrl->SetBoardFrame( m_parent );
    m_layerCtrl->SetLayersHotkeys( false );
    m_layerCtrl->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_layerCtrl->SetUndefinedLayerName( INDETERMINATE_ACTION );
    m_layerCtrl->Resync();

    for( auto& preset : magic_enum::enum_values<IPC4761_PRESET>() )
    {
        if( preset >= IPC4761_PRESET::CUSTOM )
            continue;

        const auto& name_it = m_IPC4761Names.find( preset );

        wxString name = _( "Unknown choice" );

        if( name_it != m_IPC4761Names.end() )
            name = name_it->second;

        m_protectionFeatures->AppendString( name );
    }

    m_protectionFeatures->Append( INDETERMINATE_ACTION );

    SetupStandardButtons( { { wxID_OK, _( "Apply and Close" ) },
                            { wxID_CANCEL, _( "Close" ) } } );

    m_netFilter->Connect( FILTERED_ITEM_SELECTED,
                          wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnNetFilterSelect ),
                          nullptr, this );

    m_parent->Bind( EDA_EVT_UNITS_CHANGED, &DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onUnitsChanged, this );

    finishDialogSettings();
}


DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::~DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS()
{
    g_netclassFilter = m_netclassFilter->GetStringSelection();
    g_netFilter = m_netFilter->GetSelectedNetname();

    m_netFilter->Disconnect( FILTERED_ITEM_SELECTED,
                             wxCommandEventHandler( DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::OnNetFilterSelect ),
                             nullptr, this );

    m_parent->Unbind( EDA_EVT_UNITS_CHANGED, &DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onUnitsChanged, this );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onVias( wxCommandEvent& aEvent )
{
    m_throughVias->SetValue( aEvent.IsChecked() );
    m_microVias->SetValue( aEvent.IsChecked() );
    m_blindVias->SetValue( aEvent.IsChecked() );
    m_buriedVias->SetValue( aEvent.IsChecked() );

    aEvent.Skip();
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::updateViasCheckbox()
{
    int checked = 0;

    for( const wxCheckBox* cb : { m_throughVias, m_microVias, m_blindVias, m_buriedVias } )
    {
        if( cb->GetValue() )
            checked++;
    }

    if( checked == 0 )
        m_vias->SetValue( false );
    else if( checked == 4 )
        m_vias->SetValue( true );
    else
        m_vias->Set3StateValue( wxCHK_UNDETERMINED );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onViaType( wxCommandEvent& aEvent )
{
    CallAfter(
            [this]()
            {
                updateViasCheckbox();
            } );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onUnitsChanged( wxCommandEvent& aEvent )
{
    int trackSel = m_trackWidthCtrl->GetSelection();
    int viaSel = m_viaSizesCtrl->GetSelection();

    m_parent->UpdateTrackWidthSelectBox( m_trackWidthCtrl, false, false );
    m_trackWidthCtrl->Append( INDETERMINATE_ACTION );
    m_parent->UpdateViaSizeSelectBox( m_viaSizesCtrl, false, false );
    m_viaSizesCtrl->Append( INDETERMINATE_ACTION );

    m_trackWidthCtrl->SetSelection( trackSel );
    m_viaSizesCtrl->SetSelection( viaSel );

    aEvent.Skip();
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::buildFilterLists()
{
    // Populate the net filter list with net names
    m_netFilter->SetNetInfo( &m_brd->GetNetInfo() );

    if( !m_brd->GetHighLightNetCodes().empty() )
        m_netFilter->SetSelectedNetcode( *m_brd->GetHighLightNetCodes().begin() );

    // Populate the netclass filter list with netclass names
    wxArrayString                  netclassNames;
    std::shared_ptr<NET_SETTINGS>& settings = m_brd->GetDesignSettings().m_NetSettings;

    netclassNames.push_back( settings->GetDefaultNetclass()->GetName() );

    for( const auto& [name, netclass] : settings->GetNetclasses() )
        netclassNames.push_back( name );

    m_netclassFilter->Set( netclassNames );
    m_netclassFilter->SetStringSelection( m_brd->GetDesignSettings().GetCurrentNetClassName() );

    // Populate the layer filter list
    m_layerFilter->SetBoardFrame( m_parent );
    m_layerFilter->SetLayersHotkeys( false );
    m_layerFilter->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_layerFilter->Resync();
    m_layerFilter->SetLayerSelection( m_parent->GetActiveLayer() );
}


bool DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::TransferDataToWindow()
{
    m_netclassFilter->SetStringSelection( g_netclassFilter );

    if( m_brd->FindNet( g_netFilter ) != nullptr )
        m_netFilter->SetSelectedNet( g_netFilter );

    m_trackWidthCtrl->SetSelection( (int) m_trackWidthCtrl->GetCount() - 1 );
    m_viaSizesCtrl->SetSelection( (int) m_viaSizesCtrl->GetCount() - 1 );
    m_annularRingsCtrl->SetSelection( (int) m_annularRingsCtrl->GetCount() - 1 );
    m_layerCtrl->SetStringSelection( INDETERMINATE_ACTION );
    m_protectionFeatures->SetStringSelection( INDETERMINATE_ACTION );

    wxCommandEvent dummy;
    onActionButtonChange( dummy );

    updateViasCheckbox();

    return true;
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::onActionButtonChange( wxCommandEvent& event )
{
    // Enable the items in the use specified values section
    bool enable = m_setToSpecifiedValues->GetValue();

    m_trackWidthLabel->Enable( enable );
    m_trackWidthCtrl->Enable( enable );
    m_viaSizeLabel->Enable( enable );
    m_viaSizesCtrl->Enable( enable );
    m_annularRingsLabel->Enable( enable );
    m_annularRingsCtrl->Enable( enable );
    m_layerLabel->Enable( enable );
    m_layerCtrl->Enable( enable );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::processItem( PICKED_ITEMS_LIST* aUndoList, PCB_TRACK* aItem )
{
    BOARD_DESIGN_SETTINGS& brdSettings = m_brd->GetDesignSettings();
    bool                   isTrack = aItem->Type() == PCB_TRACE_T;
    bool                   isArc = aItem->Type() == PCB_ARC_T;
    bool                   isVia = aItem->Type() == PCB_VIA_T;

    if( m_setToSpecifiedValues->GetValue() )
    {
        if( ( isArc || isTrack ) && m_trackWidthCtrl->GetStringSelection() != INDETERMINATE_ACTION )
        {
            int prevTrackWidthIndex = brdSettings.GetTrackWidthIndex();
            int trackWidthIndex = m_trackWidthCtrl->GetSelection();

            if( trackWidthIndex >= 0 )
                brdSettings.SetTrackWidthIndex( trackWidthIndex + 1 );

            m_parent->SetTrackSegmentWidth( aItem, aUndoList, false );

            brdSettings.SetTrackWidthIndex( prevTrackWidthIndex );
        }

        if( isVia && m_viaSizesCtrl->GetStringSelection() != INDETERMINATE_ACTION )
        {
            int prevViaSizeIndex = brdSettings.GetViaSizeIndex();
            int viaSizeIndex = m_viaSizesCtrl->GetSelection();

            if( viaSizeIndex >= 0 )
                brdSettings.SetViaSizeIndex( viaSizeIndex + 1 );

            m_parent->SetTrackSegmentWidth( aItem, aUndoList, false );

            brdSettings.SetViaSizeIndex( prevViaSizeIndex );
        }

        if( isVia && m_annularRingsCtrl->GetStringSelection() != INDETERMINATE_ACTION )
        {
            PCB_VIA* v = static_cast<PCB_VIA*>( aItem );

            switch( m_annularRingsCtrl->GetSelection() )
            {
            case 0:
                v->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::KEEP_ALL );
                break;
            case 1:
                v->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
                break;
            case 2:
                v->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::REMOVE_ALL );
                break;
            case 3:
                v->Padstack().SetUnconnectedLayerMode( UNCONNECTED_LAYER_MODE::START_END_ONLY );
                break;
            default:
                break;
            }
        }

        if( isVia && m_protectionFeatures->GetStringSelection() != INDETERMINATE_ACTION )
        {
            PCB_VIA* v = static_cast<PCB_VIA*>( aItem );

            setViaConfiguration( v, static_cast<IPC4761_PRESET>( m_protectionFeatures->GetSelection() ) );
        }

        if( ( isArc || isTrack ) && m_layerCtrl->GetLayerSelection() != UNDEFINED_LAYER )
        {
            if( aUndoList->FindItem( aItem ) < 0 )
            {
                ITEM_PICKER picker( nullptr, aItem, UNDO_REDO::CHANGED );
                picker.SetLink( aItem->Clone() );
                aUndoList->PushItem( picker );
            }

            aItem->SetLayer( ToLAYER_ID( m_layerCtrl->GetLayerSelection() ) );
            m_parent->GetBoard()->GetConnectivity()->Update( aItem );
        }
    }
    else
    {
        m_parent->SetTrackSegmentWidth( aItem, aUndoList, true );
    }

    m_items_changed.push_back( aItem );
}


void DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::visitItem( PICKED_ITEMS_LIST* aUndoList, PCB_TRACK* aItem )
{
    if( m_selectedItemsFilter->GetValue() )
    {
        if( !aItem->IsSelected() )
        {
            EDA_GROUP* group = aItem->GetParentGroup();

            while( group && !group->AsEdaItem()->IsSelected() )
                group = group->AsEdaItem()->GetParentGroup();

            if( !group )
                return;
        }
    }

    if( m_netFilterOpt->GetValue() && m_netFilter->GetSelectedNetcode() >= 0 )
    {
        if( aItem->GetNetCode() != m_netFilter->GetSelectedNetcode() )
            return;
    }

    if( m_netclassFilterOpt->GetValue() && !m_netclassFilter->GetStringSelection().IsEmpty() )
    {
        wxString  filterNetclass = m_netclassFilter->GetStringSelection();
        NETCLASS* netclass = aItem->GetEffectiveNetClass();

        if( !netclass->ContainsNetclassWithName( filterNetclass ) )
            return;
    }

    if( m_layerFilterOpt->GetValue() && m_layerFilter->GetLayerSelection() != UNDEFINED_LAYER )
    {
        if( aItem->GetLayer() != m_layerFilter->GetLayerSelection() )
            return;
    }

    if( aItem->Type() == PCB_VIA_T )
    {
        if( m_filterByViaSize->GetValue() && aItem->GetWidth() != m_viaSizeFilter.GetValue() )
            return;
    }
    else
    {
        if( m_filterByTrackWidth->GetValue() && aItem->GetWidth() != m_trackWidthFilter.GetValue() )
            return;
    }

    processItem( aUndoList, aItem );
}


bool DIALOG_GLOBAL_EDIT_TRACKS_AND_VIAS::TransferDataFromWindow()
{
    PICKED_ITEMS_LIST itemsListPicker;
    wxBusyCursor      dummy;

    // Examine segments
    for( PCB_TRACK* track : m_brd->Tracks() )
    {
        if( track->Type() == PCB_TRACE_T && m_tracks->GetValue() )
        {
            visitItem( &itemsListPicker, track );
        }
        else if ( track->Type() == PCB_ARC_T && m_tracks->GetValue() )
        {
            visitItem( &itemsListPicker, track );
        }
        else if( track->Type() == PCB_VIA_T )
        {
            PCB_VIA* via = static_cast<PCB_VIA*>( track );

            if( via->GetViaType() == VIATYPE::THROUGH && m_throughVias->GetValue() )
                visitItem( &itemsListPicker, via );
            else if( via->GetViaType() == VIATYPE::MICROVIA && m_microVias->GetValue() )
                visitItem( &itemsListPicker, via );
            else if( via->GetViaType() == VIATYPE::BLIND && m_blindVias->GetValue() )
                visitItem( &itemsListPicker, via );
            else if( via->GetViaType() == VIATYPE::BURIED && m_buriedVias->GetValue() )
                visitItem( &itemsListPicker, via );
        }
    }

    if( itemsListPicker.GetCount() > 0 )
    {
        m_parent->SaveCopyInUndoList( itemsListPicker, UNDO_REDO::CHANGED );

        for( PCB_TRACK* track : m_brd->Tracks() )
            m_parent->GetCanvas()->GetView()->Update( track );
    }

    m_parent->GetCanvas()->ForceRefresh();

    if( m_items_changed.size() )
    {
        m_brd->OnItemsChanged( m_items_changed );
        m_parent->OnModify();
    }

    return true;
}
