/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2018 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <dialogs/dialog_track_via_properties.h>
#include <pcb_layer_box_selector.h>
#include <tools/selection_tool.h>
#include <class_track.h>
#include <pcb_edit_frame.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <class_module.h>
#include <widgets/net_selector.h>
#include <board_commit.h>

DIALOG_TRACK_VIA_PROPERTIES::DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                          const PCBNEW_SELECTION& aItems,
                                                          COMMIT& aCommit ) :
    DIALOG_TRACK_VIA_PROPERTIES_BASE( aParent ),
    m_frame( aParent ),
    m_items( aItems ),
    m_commit( aCommit ),
    m_trackStartX( aParent, m_TrackStartXLabel, m_TrackStartXCtrl, m_TrackStartXUnit ),
    m_trackStartY( aParent, m_TrackStartYLabel, m_TrackStartYCtrl, m_TrackStartYUnit ),
    m_trackEndX( aParent, m_TrackEndXLabel, m_TrackEndXCtrl, m_TrackEndXUnit ),
    m_trackEndY( aParent, m_TrackEndYLabel, m_TrackEndYCtrl, m_TrackEndYUnit ),
    m_trackWidth( aParent, m_TrackWidthLabel, m_TrackWidthCtrl, m_TrackWidthUnit, true ),
    m_viaX( aParent, m_ViaXLabel, m_ViaXCtrl, m_ViaXUnit ),
    m_viaY( aParent, m_ViaYLabel, m_ViaYCtrl, m_ViaYUnit ),
    m_viaDiameter( aParent, m_ViaDiameterLabel, m_ViaDiameterCtrl, m_ViaDiameterUnit, true ),
    m_viaDrill( aParent, m_ViaDrillLabel, m_ViaDrillCtrl, m_ViaDrillUnit, true ),
    m_tracks( false ),
    m_vias( false )
{
    wxASSERT( !m_items.Empty() );

    VIATYPE_T    viaType = VIA_NOT_DEFINED;

    m_netSelector->SetNetInfo( &aParent->GetBoard()->GetNetInfo() );

    m_TrackLayerCtrl->SetLayersHotkeys( false );
    m_TrackLayerCtrl->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_TrackLayerCtrl->SetBoardFrame( aParent );
    m_TrackLayerCtrl->Resync();

    m_ViaStartLayer->SetLayersHotkeys( false );
    m_ViaStartLayer->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_ViaStartLayer->SetBoardFrame( aParent );
    m_ViaStartLayer->Resync();

    m_ViaEndLayer->SetLayersHotkeys( false );
    m_ViaEndLayer->SetNotAllowedLayerSet( LSET::AllNonCuMask() );
    m_ViaEndLayer->SetBoardFrame( aParent );
    m_ViaEndLayer->Resync();

    bool nets = false;
    int  net = 0;
    bool hasLocked = false;
    bool hasUnlocked = false;

    // Look for values that are common for every item that is selected
    for( auto& item : m_items )
    {
        if( !nets )
        {
            net = static_cast<BOARD_CONNECTED_ITEM*>( item )->GetNetCode();
            nets = true;
        }
        else if( net != static_cast<BOARD_CONNECTED_ITEM*>( item )->GetNetCode() )
        {
            net = -1;
        }

        switch( item->Type() )
        {
            case PCB_TRACE_T:
            {
                const TRACK* t = static_cast<const TRACK*>( item );

                if( !m_tracks )     // first track in the list
                {
                    m_trackStartX.SetValue( t->GetStart().x );
                    m_trackStartY.SetValue( t->GetStart().y );
                    m_trackEndX.SetValue( t->GetEnd().x );
                    m_trackEndY.SetValue( t->GetEnd().y );
                    m_trackWidth.SetValue( t->GetWidth() );
                    m_TrackLayerCtrl->SetLayerSelection( t->GetLayer() );
                    m_tracks = true;
                }
                else        // check if values are the same for every selected track
                {
                    if( m_trackStartX.GetValue() != t->GetStart().x )
                        m_trackStartX.SetValue( INDETERMINATE );

                    if( m_trackStartY.GetValue() != t->GetStart().y )
                        m_trackStartY.SetValue( INDETERMINATE );

                    if( m_trackEndX.GetValue() != t->GetEnd().x )
                        m_trackEndX.SetValue( INDETERMINATE );

                    if( m_trackEndY.GetValue() != t->GetEnd().y )
                        m_trackEndY.SetValue( INDETERMINATE );

                    if( m_trackWidth.GetValue() != t->GetWidth() )
                        m_trackWidth.SetValue( INDETERMINATE );

                    if( m_TrackLayerCtrl->GetLayerSelection() != t->GetLayer() )
                        m_TrackLayerCtrl->SetLayerSelection( UNDEFINED_LAYER );
                }

                if( t->IsLocked() )
                    hasLocked = true;
                else
                    hasUnlocked = true;

                break;
            }

            case PCB_VIA_T:
            {
                const VIA* v = static_cast<const VIA*>( item );

                if( !m_vias )       // first via in the list
                {
                    m_viaX.SetValue( v->GetPosition().x );
                    m_viaY.SetValue( v->GetPosition().y );
                    m_viaDiameter.SetValue( v->GetWidth() );
                    m_viaDrill.SetValue( v->GetDrillValue() );
                    m_vias = true;
                    viaType = v->GetViaType();
                    m_ViaStartLayer->SetLayerSelection( v->TopLayer() );
                    m_ViaEndLayer->SetLayerSelection( v->BottomLayer() );
                }
                else        // check if values are the same for every selected via
                {
                    if( m_viaX.GetValue() != v->GetPosition().x )
                        m_viaX.SetValue( INDETERMINATE );

                    if( m_viaY.GetValue() != v->GetPosition().y )
                        m_viaY.SetValue( INDETERMINATE );

                    if( m_viaDiameter.GetValue() != v->GetWidth() )
                        m_viaDiameter.SetValue( INDETERMINATE );

                    if( m_viaDrill.GetValue() != v->GetDrillValue() )
                        m_viaDrill.SetValue( INDETERMINATE );

                    if( viaType != v->GetViaType() )
                        viaType = VIA_NOT_DEFINED;

                    if( m_ViaStartLayer->GetLayerSelection() != v->TopLayer() )
                        m_ViaStartLayer->SetLayerSelection( UNDEFINED_LAYER );

                    if( m_ViaEndLayer->GetLayerSelection() != v->BottomLayer() )
                        m_ViaEndLayer->SetLayerSelection( UNDEFINED_LAYER );
                }

                if( v->IsLocked() )
                    hasLocked = true;
                else
                    hasUnlocked = true;

                break;
            }

            default:
            {
                wxASSERT( false );
                break;
            }
        }
    }

    if ( net >= 0 )
        m_netSelector->SetSelectedNetcode( net );
    else
        m_netSelector->SetIndeterminate();

    wxASSERT( m_tracks || m_vias );

    if( m_vias )
    {
        m_DesignRuleViasUnit->SetLabel( GetAbbreviatedUnitsLabel( m_units, true ) );

        int viaSelection = wxNOT_FOUND;

        for( unsigned ii = 0; ii < aParent->GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
        {
            VIA_DIMENSION* viaDimension = &aParent->GetDesignSettings().m_ViasDimensionsList[ii];
            wxString msg = StringFromValue( m_units, viaDimension->m_Diameter, false, true )
                + " / " + StringFromValue( m_units, viaDimension->m_Drill, false, true );
            m_DesignRuleViasCtrl->Append( msg, viaDimension );

            if( viaSelection == wxNOT_FOUND
                && m_viaDiameter.GetValue() == viaDimension->m_Diameter
                && m_viaDrill.GetValue() == viaDimension->m_Drill )
            {
                viaSelection = ii;
            }
        }

        m_DesignRuleViasCtrl->SetSelection( viaSelection );

        m_DesignRuleViasCtrl->Connect( wxEVT_CHOICE, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onViaSelect ), NULL, this );
        m_ViaDiameterCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onViaEdit ), NULL, this );
        m_ViaDrillCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onViaEdit ), NULL, this );
        m_ViaTypeChoice->Connect( wxEVT_CHOICE, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onViaEdit ), NULL, this );

        SetInitialFocus( m_ViaDiameterCtrl );

        m_ViaTypeChoice->Enable();

        switch( viaType )
        {
        case VIA_THROUGH:      m_ViaTypeChoice->SetSelection( 0 ); break;
        case VIA_MICROVIA:     m_ViaTypeChoice->SetSelection( 1 ); break;
        case VIA_BLIND_BURIED: m_ViaTypeChoice->SetSelection( 2 ); break;
        case VIA_NOT_DEFINED:  m_ViaTypeChoice->SetSelection( 3 ); break;
        }

        m_ViaStartLayer->Enable( viaType != VIA_THROUGH );
        m_ViaEndLayer->Enable( viaType != VIA_THROUGH );
    }
    else
    {
        m_MainSizer->Hide( m_sbViaSizer, true );
    }

    if( m_tracks )
    {
        m_DesignRuleWidthsUnits->SetLabel( GetAbbreviatedUnitsLabel( m_units, true ) );

        int widthSelection = wxNOT_FOUND;

        for( unsigned ii = 0; ii < aParent->GetDesignSettings().m_TrackWidthList.size(); ii++ )
        {
            int width = aParent->GetDesignSettings().m_TrackWidthList[ii];
            wxString msg = StringFromValue( m_units, width, false, true );
            m_DesignRuleWidthsCtrl->Append( msg );

            if( widthSelection == wxNOT_FOUND && m_trackWidth.GetValue() == width )
                widthSelection = ii;
        }

        m_DesignRuleWidthsCtrl->SetSelection( widthSelection );

        m_DesignRuleWidthsCtrl->Connect( wxEVT_CHOICE, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onWidthSelect ), NULL, this );
        m_TrackWidthCtrl->Connect( wxEVT_TEXT, wxCommandEventHandler( DIALOG_TRACK_VIA_PROPERTIES::onWidthEdit ), NULL, this );

        SetInitialFocus( m_TrackWidthCtrl );
    }
    else
    {
        m_MainSizer->Hide( m_sbTrackSizer, true );
    }

    if( hasLocked && hasUnlocked )
         m_lockedCbox->Set3StateValue( wxCHK_UNDETERMINED );
    else if( hasLocked )
        m_lockedCbox->Set3StateValue( wxCHK_CHECKED );
    else
        m_lockedCbox->Set3StateValue( wxCHK_UNCHECKED );

    SetInitialFocus( m_tracks ? m_TrackWidthCtrl : m_ViaDiameterCtrl );

    m_StdButtonsOK->SetDefault();

    // Now all widgets have the size fixed, call FinishDialogSettings
    FinishDialogSettings();
}


bool DIALOG_TRACK_VIA_PROPERTIES::confirmPadChange( const std::vector<D_PAD*>& changingPads )
{
    wxString msg;

    if( changingPads.size() == 1 )
    {
        D_PAD* pad = *changingPads.begin();
        msg.Printf( _( "This will change the net assigned to %s pad %s to %s.\n"
                       "Do you wish to continue?" ),
                    pad->GetParent()->GetReference(),
                    pad->GetName(),
                    m_netSelector->GetValue() );
    }
    else if( changingPads.size() == 2 )
    {
        D_PAD* pad1 = *changingPads.begin();
        D_PAD* pad2 = *( ++changingPads.begin() );
        msg.Printf( _( "This will change the net assigned to %s pad %s and %s pad %s to %s.\n"
                       "Do you wish to continue?" ),
                    pad1->GetParent()->GetReference(),
                    pad1->GetName(),
                    pad2->GetParent()->GetReference(),
                    pad2->GetName(),
                    m_netSelector->GetValue() );
    }
    else
    {
        msg.Printf( _( "This will change the net assigned to %lu connected pads to %s.\n"
                       "Do you wish to continue?" ),
                    static_cast<unsigned long>( changingPads.size() ),
                    m_netSelector->GetValue() );
    }

    KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetOKLabel( _( "Continue" ) );
    dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

    return dlg.ShowModal() == wxID_OK;
}


bool DIALOG_TRACK_VIA_PROPERTIES::TransferDataFromWindow()
{
    auto connectivity = m_frame->GetBoard()->GetConnectivity();
    int newNetCode = m_netSelector->GetSelectedNetcode();
    std::vector<D_PAD*> changingPads;

    if ( !m_netSelector->IsIndeterminate() )
    {
        std::set<D_PAD*> connectedPads;

        for( auto& item : m_items )
        {
            const KICAD_T ourTypes[] = { PCB_TRACE_T, PCB_PAD_T, PCB_VIA_T, PCB_MODULE_T, EOT };
            auto connectedItems = connectivity->GetConnectedItems( static_cast<BOARD_CONNECTED_ITEM*>( item ), ourTypes, true );
            for ( auto citem : connectedItems )
            {
                if( citem->Type() == PCB_PAD_T )
                {
                    connectedPads.insert( static_cast<D_PAD*>( citem ) );
                }
            }
        }

        for( D_PAD* pad : connectedPads )
        {
            if( pad->GetNetCode() != newNetCode )
                changingPads.push_back( pad );
        }
    }

    // Run validations:

    if( changingPads.size() )
    {
        if( !confirmPadChange( changingPads ) )
            return false;
    }

    if( m_vias )
    {
        if( !m_viaDiameter.Validate( GEOMETRY_MIN_SIZE, INT_MAX )
            || !m_viaDrill.Validate( GEOMETRY_MIN_SIZE, INT_MAX ) )
            return false;

        if( m_ViaDiameterCtrl->IsEnabled() && !m_viaDiameter.IsIndeterminate()
            && m_ViaDrillCtrl->IsEnabled() && !m_viaDrill.IsIndeterminate()
            && m_viaDiameter.GetValue() <= m_viaDrill.GetValue() )
        {
            DisplayError( GetParent(), _( "Via drill size must be smaller than via diameter" ) );
            m_ViaDrillCtrl->SelectAll();
            m_ViaDrillCtrl->SetFocus();
            return false;
        }

        if( m_ViaStartLayer->GetLayerSelection() != UNDEFINED_LAYER &&
            m_ViaStartLayer->GetLayerSelection() == m_ViaEndLayer->GetLayerSelection() )
        {
            DisplayError( GetParent(), _( "Via start layer and end layer cannot be the same" ) );
            return false;
        }
    }

    if( m_tracks )
    {
        if( !m_trackWidth.Validate( GEOMETRY_MIN_SIZE, INT_MAX ) )
            return false;
    }

    // If we survived that, then save the changes:

    bool changeLock = m_lockedCbox->Get3StateValue() != wxCHK_UNDETERMINED;
    bool setLock = m_lockedCbox->Get3StateValue() == wxCHK_CHECKED;

    for( auto item : m_items )
    {
        m_commit.Modify( item );

        switch( item->Type() )
        {
            case PCB_TRACE_T:
            {
                wxASSERT( m_tracks );
                TRACK* t = static_cast<TRACK*>( item );

                if( !m_trackStartX.IsIndeterminate() )
                    t->SetStart( wxPoint( m_trackStartX.GetValue(), t->GetStart().y ) );

                if( !m_trackStartY.IsIndeterminate() )
                    t->SetStart( wxPoint( t->GetStart().x, m_trackStartY.GetValue() ) );

                if( !m_trackEndX.IsIndeterminate() )
                    t->SetEnd( wxPoint( m_trackEndX.GetValue(), t->GetEnd().y ) );

                if( !m_trackEndY.IsIndeterminate() )
                    t->SetEnd( wxPoint( t->GetEnd().x, m_trackEndY.GetValue() ) );

                if( m_trackNetclass->IsChecked() )
                    t->SetWidth( t->GetNetClass()->GetTrackWidth() );
                else if( !m_trackWidth.IsIndeterminate() )
                    t->SetWidth( m_trackWidth.GetValue() );

                LAYER_NUM layer = m_TrackLayerCtrl->GetLayerSelection();

                if( layer != UNDEFINED_LAYER )
                    t->SetLayer( (PCB_LAYER_ID) layer );

                if( changeLock )
                    t->SetLocked( setLock );

                if ( !m_netSelector->IsIndeterminate() )
                    t->SetNetCode( m_netSelector->GetSelectedNetcode() );

                break;
            }

            case PCB_VIA_T:
            {
                wxASSERT( m_vias );
                VIA* v = static_cast<VIA*>( item );

                if( !m_viaX.IsIndeterminate() )
                    v->SetPosition( wxPoint( m_viaX.GetValue(), v->GetPosition().y ) );

                if( !m_viaY.IsIndeterminate() )
                    v->SetPosition( wxPoint( v->GetPosition().x, m_viaY.GetValue() ) );

                if( m_ViaTypeChoice->GetSelection() != 3)
                {
                    switch( m_ViaTypeChoice->GetSelection() )
                    {
                    default:
                    case 0: v->SetViaType( VIA_THROUGH ); v->SanitizeLayers(); break;
                    case 1: v->SetViaType( VIA_MICROVIA );                     break;
                    case 2: v->SetViaType( VIA_BLIND_BURIED );                 break;
                    }
                }

                auto startLayer = static_cast<PCB_LAYER_ID>( m_ViaStartLayer->GetLayerSelection() );
                auto endLayer = static_cast<PCB_LAYER_ID>( m_ViaEndLayer->GetLayerSelection() );

                if (startLayer != UNDEFINED_LAYER )
                    v->SetTopLayer( startLayer );

                if (endLayer != UNDEFINED_LAYER )
                    v->SetBottomLayer( endLayer );

                v->SanitizeLayers();

                if( m_viaNetclass->IsChecked() )
                {
                    switch( v->GetViaType() )
                    {
                    default:
                        wxFAIL_MSG("Unhandled via type");
                        // fall through

                    case VIA_THROUGH:
                    case VIA_BLIND_BURIED:
                        v->SetWidth( v->GetNetClass()->GetViaDiameter() );
                        v->SetDrill( v->GetNetClass()->GetViaDrill() );
                        break;

                    case VIA_MICROVIA:
                        v->SetWidth( v->GetNetClass()->GetuViaDiameter() );
                        v->SetDrill( v->GetNetClass()->GetuViaDrill() );
                        break;
                    }
                }
                else
                {
                    if( !m_viaDiameter.IsIndeterminate() )
                        v->SetWidth( m_viaDiameter.GetValue() );

                    if( !m_viaDrill.IsIndeterminate() )
                        v->SetDrill( m_viaDrill.GetValue() );
                }

                if ( !m_netSelector->IsIndeterminate() )
                    v->SetNetCode( m_netSelector->GetSelectedNetcode() );

                if( changeLock )
                    v->SetLocked( setLock );

                break;
            }

            default:
                wxASSERT( false );
                break;
        }
    }

    if ( !m_netSelector->IsIndeterminate() )
    {
        // Commit::Push() will rebuild connectivitiy propagating nets from connected pads
        // outwards.  We therefore have to update the connected pads in order for the net
        // change to "stick".
        for( D_PAD* pad : changingPads )
        {
            m_commit.Modify( pad );
            pad->SetNetCode( m_netSelector->GetSelectedNetcode() );
        }
    }

    m_commit.Push( _( "Edit track/via properties" ) );

    return true;
}


void DIALOG_TRACK_VIA_PROPERTIES::onTrackNetclassCheck( wxCommandEvent& aEvent )
{
    bool enableNC = aEvent.IsChecked();

    m_DesignRuleWidths->Enable( !enableNC );
    m_DesignRuleWidthsCtrl->Enable( !enableNC );
    m_DesignRuleWidthsUnits->Enable( !enableNC );

    m_trackWidth.Enable( !enableNC );
}


void DIALOG_TRACK_VIA_PROPERTIES::onWidthSelect( wxCommandEvent& aEvent )
{
    m_TrackWidthCtrl->ChangeValue( m_DesignRuleWidthsCtrl->GetStringSelection() );
}


void DIALOG_TRACK_VIA_PROPERTIES::onWidthEdit( wxCommandEvent& aEvent )
{
    m_DesignRuleWidthsCtrl->SetSelection( wxNOT_FOUND );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaNetclassCheck( wxCommandEvent& aEvent )
{
    bool enableNC = aEvent.IsChecked();

    m_DesignRuleVias->Enable( !enableNC );
    m_DesignRuleViasCtrl->Enable( !enableNC );
    m_DesignRuleViasUnit->Enable( !enableNC );

    m_viaDiameter.Enable( !enableNC );
    m_viaDrill.Enable( !enableNC );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaSelect( wxCommandEvent& aEvent )
{
    VIA_DIMENSION* viaDimension = static_cast<VIA_DIMENSION*> ( aEvent.GetClientData() );

    m_viaDiameter.ChangeValue( viaDimension->m_Diameter );
    m_viaDrill.ChangeValue( viaDimension->m_Drill );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaEdit( wxCommandEvent& aEvent )
{
    m_DesignRuleViasCtrl->SetSelection( wxNOT_FOUND );

    if( m_vias )
    {
        if( m_ViaTypeChoice->GetSelection() != 0 ) // check if selected type isnt through.
        {
            m_ViaStartLayer->Enable();
            m_ViaEndLayer->Enable();
        }
        else
        {
            m_ViaStartLayer->SetLayerSelection( F_Cu );
            m_ViaEndLayer->SetLayerSelection( B_Cu );

            m_ViaStartLayer->Enable( false );
            m_ViaEndLayer->Enable( false );
        }
    }
}
