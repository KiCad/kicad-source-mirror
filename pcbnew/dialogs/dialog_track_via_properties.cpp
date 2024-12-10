/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright (C) 2018-2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <core/kicad_algo.h>
#include <dialogs/dialog_track_via_properties.h>
#include <pcb_layer_box_selector.h>
#include <tools/pcb_selection_tool.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <pcb_edit_frame.h>
#include <confirm.h>
#include <connectivity/connectivity_data.h>
#include <board_commit.h>
#include <macros.h>

DIALOG_TRACK_VIA_PROPERTIES::DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_FRAME* aParent,
                                                          const PCB_SELECTION& aItems ) :
        DIALOG_TRACK_VIA_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_items( aItems ),
        m_trackStartX( aParent, m_TrackStartXLabel, m_TrackStartXCtrl, nullptr ),
        m_trackStartY( aParent, m_TrackStartYLabel, m_TrackStartYCtrl, m_TrackStartYUnit ),
        m_trackEndX( aParent, m_TrackEndXLabel, m_TrackEndXCtrl, nullptr ),
        m_trackEndY( aParent, m_TrackEndYLabel, m_TrackEndYCtrl, m_TrackEndYUnit ),
        m_trackWidth( aParent, m_TrackWidthLabel, m_TrackWidthCtrl, m_TrackWidthUnit ),
        m_viaX( aParent, m_ViaXLabel, m_ViaXCtrl, nullptr ),
        m_viaY( aParent, m_ViaYLabel, m_ViaYCtrl, m_ViaYUnit ),
        m_viaDiameter( aParent, m_ViaDiameterLabel, m_ViaDiameterCtrl, m_ViaDiameterUnit ),
        m_viaDrill( aParent, m_ViaDrillLabel, m_ViaDrillCtrl, m_ViaDrillUnit ),
        m_teardropHDPercent( aParent, m_stHDRatio, m_tcHDRatio, m_stHDRatioUnits ),
        m_teardropLenPercent( aParent, m_stLenPercentLabel, m_tcLenPercent, nullptr ),
        m_teardropMaxLen( aParent, m_stMaxLen, m_tcTdMaxLen, m_stMaxLenUnits ),
        m_teardropWidthPercent( aParent, m_stWidthPercentLabel, m_tcWidthPercent, nullptr ),
        m_teardropMaxWidth( aParent, m_stMaxWidthLabel, m_tcMaxWidth, m_stMaxWidthUnits ),
        m_tracks( false ),
        m_vias( false )
{
    m_useCalculatedSize = true;

    wxASSERT( !m_items.Empty() );

    m_legacyTeardropsIcon->SetBitmap( KiBitmapBundle( BITMAPS::dialog_warning ) );
    m_legacyTeardropsWarning->Show( m_frame->GetBoard()->LegacyTeardrops() );

    m_bitmapTeardrop->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_sizes ) );

    m_teardropHDPercent.SetUnits( EDA_UNITS::PERCENT );
    m_teardropLenPercent.SetUnits( EDA_UNITS::PERCENT );
    m_teardropWidthPercent.SetUnits( EDA_UNITS::PERCENT );

    m_minTrackWidthHint->SetFont( KIUI::GetInfoFont( this ).Italic() );

    // Configure display origin transforms
    m_trackStartX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_trackStartY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_trackEndX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_trackEndY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );
    m_viaX.SetCoordType( ORIGIN_TRANSFORMS::ABS_X_COORD );
    m_viaY.SetCoordType( ORIGIN_TRANSFORMS::ABS_Y_COORD );

    VIATYPE viaType = VIATYPE::NOT_DEFINED;

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

    // Start and end layers of vias
    // if at least 2 vias do not have the same start or the same end layer
    // the layers will be set as undefined
    int selection_first_layer = -1;
    int selection_last_layer = -1;

    // The selection layer for tracks
    int track_selection_layer = -1;

    auto getAnnularRingSelection =
            []( const PCB_VIA* via ) -> int
            {
                if( !via->GetRemoveUnconnected() )
                    return 0;
                else if( via->GetKeepStartEnd() )
                    return 1;
                else
                    return 2;
            };

    // Look for values that are common for every item that is selected
    for( EDA_ITEM* item : m_items )
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
            case PCB_ARC_T:
            {
                const PCB_TRACK* t = static_cast<const PCB_TRACK*>( item );

                if( !m_tracks )     // first track in the list
                {
                    m_trackStartX.SetValue( t->GetStartX() );
                    m_trackStartY.SetValue( t->GetStartY() );
                    m_trackEndX.SetValue( t->GetEndX() );
                    m_trackEndY.SetValue( t->GetEndY() );
                    m_trackWidth.SetValue( t->GetWidth() );
                    track_selection_layer = t->GetLayer();
                    m_tracks = true;
                }
                else        // check if values are the same for every selected track
                {
                    if( m_trackStartX.GetValue() != t->GetStartX() )
                        m_trackStartX.SetValue( INDETERMINATE_STATE );

                    if( m_trackStartY.GetValue() != t->GetStartY() )
                        m_trackStartY.SetValue( INDETERMINATE_STATE );

                    if( m_trackEndX.GetValue() != t->GetEndX() )
                        m_trackEndX.SetValue( INDETERMINATE_STATE );

                    if( m_trackEndY.GetValue() != t->GetEndY() )
                        m_trackEndY.SetValue( INDETERMINATE_STATE );

                    if( m_trackWidth.GetValue() != t->GetWidth() )
                        m_trackWidth.SetValue( INDETERMINATE_STATE );

                    if( track_selection_layer != t->GetLayer() )
                        track_selection_layer = UNDEFINED_LAYER;
                }

                if( t->IsLocked() )
                    hasLocked = true;
                else
                    hasUnlocked = true;

                break;
            }

            case PCB_VIA_T:
            {
                PCB_VIA* v = static_cast<PCB_VIA*>( item );

                if( !m_vias )       // first via in the list
                {
                    m_viaX.SetValue( v->GetPosition().x );
                    m_viaY.SetValue( v->GetPosition().y );
                    m_viaDiameter.SetValue( v->GetWidth() );
                    m_viaDrill.SetValue( v->GetDrillValue() );
                    m_vias = true;
                    viaType = v->GetViaType();
                    m_viaNotFree->SetValue( !v->GetIsFree() );
                    m_annularRingsCtrl->SetSelection( getAnnularRingSelection( v ) );
                    selection_first_layer = v->TopLayer();
                    selection_last_layer = v->BottomLayer();

                    m_cbTeardrops->SetValue( v->GetTeardropParams().m_Enabled );
                    m_cbTeardropsUseNextTrack->SetValue( v->GetTeardropParams().m_AllowUseTwoTracks );
                    m_teardropMaxLen.SetValue( v->GetTeardropParams().m_TdMaxLen );
                    m_teardropMaxWidth.SetValue( v->GetTeardropParams().m_TdMaxWidth );
                    m_teardropLenPercent.SetDoubleValue( v->GetTeardropParams().m_BestLengthRatio*100.0 );
                    m_teardropWidthPercent.SetDoubleValue( v->GetTeardropParams().m_BestWidthRatio*100.0 );
                    m_teardropHDPercent.SetDoubleValue( v->GetTeardropParams().m_WidthtoSizeFilterRatio*100.0 );
                    m_curvedEdges->SetValue( v->GetTeardropParams().IsCurved() );
                    m_curvePointsCtrl->SetValue( v->GetTeardropParams().m_CurveSegCount );
                }
                else        // check if values are the same for every selected via
                {
                    if( m_viaX.GetValue() != v->GetPosition().x )
                        m_viaX.SetValue( INDETERMINATE_STATE );

                    if( m_viaY.GetValue() != v->GetPosition().y )
                        m_viaY.SetValue( INDETERMINATE_STATE );

                    if( m_viaDiameter.GetValue() != v->GetWidth() )
                        m_viaDiameter.SetValue( INDETERMINATE_STATE );

                    if( m_viaDrill.GetValue() != v->GetDrillValue() )
                        m_viaDrill.SetValue( INDETERMINATE_STATE );

                    if( viaType != v->GetViaType() )
                        viaType = VIATYPE::NOT_DEFINED;

                    if( v->GetIsFree() != !m_viaNotFree->GetValue() )
                        m_viaNotFree->Set3StateValue( wxCHK_UNDETERMINED );

                    if( selection_first_layer != v->TopLayer() )
                        selection_first_layer = UNDEFINED_LAYER;

                    if( selection_last_layer != v->BottomLayer() )
                        selection_last_layer = UNDEFINED_LAYER;

                    if( m_annularRingsCtrl->GetSelection() != getAnnularRingSelection( v ) )
                    {
                        if( m_annularRingsCtrl->GetStrings().size() < 4 )
                            m_annularRingsCtrl->AppendString( INDETERMINATE_STATE );

                        m_annularRingsCtrl->SetSelection( 3 );
                    }

                    if( m_cbTeardrops->GetValue() != v->GetTeardropParams().m_Enabled )
                        m_cbTeardrops->Set3StateValue( wxCHK_UNDETERMINED );

                    if( m_cbTeardropsUseNextTrack->GetValue() != v->GetTeardropParams().m_AllowUseTwoTracks )
                        m_cbTeardropsUseNextTrack->Set3StateValue( wxCHK_UNDETERMINED );

                    if( m_teardropMaxLen.GetValue() != v->GetTeardropParams().m_TdMaxLen )
                        m_teardropMaxLen.SetValue( INDETERMINATE_STATE );

                    if( m_teardropMaxWidth.GetValue() != v->GetTeardropParams().m_TdMaxWidth )
                        m_teardropMaxWidth.SetValue( INDETERMINATE_STATE );

                    if( m_teardropLenPercent.GetDoubleValue() != v->GetTeardropParams().m_BestLengthRatio *100.0 )
                        m_teardropLenPercent.SetValue( INDETERMINATE_STATE );

                    if( m_teardropWidthPercent.GetDoubleValue() != v->GetTeardropParams().m_BestWidthRatio *100.0 )
                        m_teardropWidthPercent.SetValue( INDETERMINATE_STATE );

                    if( m_teardropHDPercent.GetDoubleValue() != v->GetTeardropParams().m_WidthtoSizeFilterRatio*100.0 )
                        m_teardropHDPercent.SetValue( INDETERMINATE_STATE );

                    if( m_curvePointsCtrl->GetValue() != v->GetTeardropParams().m_CurveSegCount )
                    {
                        m_curvedEdges->Set3StateValue( wxCHK_UNDETERMINED );
                        m_curvePointsCtrl->SetValue( 5 );
                    }
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

    if( m_tracks )
    {
        // Set the track layer selection state:
        if( track_selection_layer == UNDEFINED_LAYER )
        {
            m_TrackLayerCtrl->SetUndefinedLayerName( INDETERMINATE_STATE );
            m_TrackLayerCtrl->Resync();
        }

        m_TrackLayerCtrl->SetLayerSelection( track_selection_layer );
    }

    // Set the vias layers selections state:
    if( m_vias )
    {
        if( selection_first_layer == UNDEFINED_LAYER )
        {
            m_ViaStartLayer->SetUndefinedLayerName( INDETERMINATE_STATE );
            m_ViaStartLayer->Resync();
        }

        m_ViaStartLayer->SetLayerSelection( selection_first_layer );

        if( selection_last_layer == UNDEFINED_LAYER )
        {
            m_ViaEndLayer->SetUndefinedLayerName( INDETERMINATE_STATE );
            m_ViaEndLayer->Resync();
        }

        m_ViaEndLayer->SetLayerSelection( selection_last_layer );
    }

    m_netSelector->SetBoard( aParent->GetBoard() );
    m_netSelector->SetNetInfo( &aParent->GetBoard()->GetNetInfo() );

    if ( net >= 0 )
    {
        m_netSelector->SetSelectedNetcode( net );
    }
    else
    {
        m_netSelector->SetIndeterminateString( INDETERMINATE_STATE );
        m_netSelector->SetIndeterminate();
    }

    wxASSERT( m_tracks || m_vias );

    if( m_vias )
    {
        if( m_viaNotFree->GetValue() && !m_tracks )
        {
            // Disable net selector to re-inforce meaning of "Automatically update via nets",
            // but not when tracks are also selected as then things get harder if you want to
            // update all the nets to match.
            m_netSelectorLabel->Disable();
            m_netSelector->Disable();
        }

        int viaSelection = wxNOT_FOUND;

        // 0 is the netclass place-holder
        for( unsigned ii = 1; ii < aParent->GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
        {
            VIA_DIMENSION* viaDimension = &aParent->GetDesignSettings().m_ViasDimensionsList[ii];
            wxString       msg = m_frame->StringFromValue( viaDimension->m_Diameter )
                                    + wxT( " / " )
                                    + m_frame->StringFromValue( viaDimension->m_Drill );
            m_predefinedViaSizesCtrl->Append( msg, viaDimension );

            if( viaSelection == wxNOT_FOUND
                && m_viaDiameter.GetValue() == viaDimension->m_Diameter
                && m_viaDrill.GetValue() == viaDimension->m_Drill )
            {
                viaSelection = ii - 1;
            }
        }

        m_predefinedViaSizesCtrl->SetSelection( viaSelection );
        m_predefinedViaSizesUnits->SetLabel( EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );

        m_ViaTypeChoice->Enable();

        switch( viaType )
        {
        case VIATYPE::THROUGH:      m_ViaTypeChoice->SetSelection( 0 );           break;
        case VIATYPE::MICROVIA:     m_ViaTypeChoice->SetSelection( 1 );           break;
        case VIATYPE::BLIND_BURIED: m_ViaTypeChoice->SetSelection( 2 );           break;
        case VIATYPE::NOT_DEFINED:  m_ViaTypeChoice->SetSelection( wxNOT_FOUND ); break;
        }

        m_ViaStartLayer->Enable( viaType != VIATYPE::THROUGH );
        m_ViaEndLayer->Enable( viaType != VIATYPE::THROUGH );

        m_annularRingsLabel->Show( getLayerDepth() > 1 );
        m_annularRingsCtrl->Show( getLayerDepth() > 1 );
    }
    else
    {
        m_viaNotFree->Hide();
        m_MainSizer->Hide( m_sbViaSizer, true );
    }

    if( m_tracks )
    {
        int widthSelection = wxNOT_FOUND;

        // 0 is the netclass place-holder
        for( unsigned ii = 1; ii < aParent->GetDesignSettings().m_TrackWidthList.size(); ii++ )
        {
            int      width = aParent->GetDesignSettings().m_TrackWidthList[ii];
            wxString msg = m_frame->StringFromValue( width );
            m_predefinedTrackWidthsCtrl->Append( msg );

            if( widthSelection == wxNOT_FOUND && m_trackWidth.GetValue() == width )
                widthSelection = ii - 1;
        }

        m_predefinedTrackWidthsCtrl->SetSelection( widthSelection );
        m_predefinedTrackWidthsUnits->SetLabel( EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );
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

    if( m_tracks )
        SetInitialFocus( m_TrackWidthCtrl );
    else if( m_netSelector->IsEnabled() )
        SetInitialFocus( m_netSelector );
    else
        SetInitialFocus( m_ViaDiameterCtrl );

    SetupStandardButtons();

    m_frame->Bind( EDA_EVT_UNITS_CHANGED, &DIALOG_TRACK_VIA_PROPERTIES::onUnitsChanged, this );
    m_netSelector->Bind( NET_SELECTED, &DIALOG_TRACK_VIA_PROPERTIES::onNetSelector, this );

    // Now all widgets have the size fixed, call FinishDialogSettings
    finishDialogSettings();
}


DIALOG_TRACK_VIA_PROPERTIES::~DIALOG_TRACK_VIA_PROPERTIES()
{
    m_frame->Unbind( EDA_EVT_UNITS_CHANGED, &DIALOG_TRACK_VIA_PROPERTIES::onUnitsChanged, this );
}


void DIALOG_TRACK_VIA_PROPERTIES::onUnitsChanged( wxCommandEvent& aEvent )
{
    if( m_vias )
    {
        int viaSel = m_predefinedViaSizesCtrl->GetSelection();

        m_predefinedViaSizesCtrl->Clear();

        // 0 is the netclass place-holder
        for( unsigned ii = 1; ii < m_frame->GetDesignSettings().m_ViasDimensionsList.size(); ii++ )
        {
            VIA_DIMENSION* viaDimension = &m_frame->GetDesignSettings().m_ViasDimensionsList[ii];
            wxString       msg = m_frame->StringFromValue( viaDimension->m_Diameter )
                                    + wxT( " / " )
                                    + m_frame->StringFromValue( viaDimension->m_Drill );
            m_predefinedViaSizesCtrl->Append( msg, viaDimension );
        }

        m_predefinedViaSizesCtrl->SetSelection( viaSel );
        m_predefinedViaSizesUnits->SetLabel( EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );
    }

    if( m_tracks )
    {
        int trackSel = m_predefinedTrackWidthsCtrl->GetSelection();

        m_predefinedTrackWidthsCtrl->Clear();

        // 0 is the netclass place-holder
        for( unsigned ii = 1; ii < m_frame->GetDesignSettings().m_TrackWidthList.size(); ii++ )
        {
            int      width = m_frame->GetDesignSettings().m_TrackWidthList[ii];
            wxString msg = m_frame->StringFromValue( width );
            m_predefinedTrackWidthsCtrl->Append( msg );
        }

        m_predefinedTrackWidthsCtrl->SetSelection( trackSel );
        m_predefinedTrackWidthsUnits->SetLabel( EDA_UNIT_UTILS::GetLabel( m_frame->GetUserUnits() ) );
    }

    aEvent.Skip();
}


bool DIALOG_TRACK_VIA_PROPERTIES::confirmPadChange( const std::vector<PAD*>& changingPads )
{
    wxString msg;

    if( changingPads.size() == 1 )
    {
        PAD* pad = *changingPads.begin();
        msg.Printf( _( "Changing the net will also update %s pad %s to %s." ),
                    pad->GetParentFootprint()->GetReference(),
                    pad->GetNumber(),
                    m_netSelector->GetValue() );
    }
    else if( changingPads.size() == 2 )
    {
        PAD* pad1 = *changingPads.begin();
        PAD* pad2 = *( ++changingPads.begin() );
        msg.Printf( _( "Changing the net will also update %s pad %s and %s pad %s to %s." ),
                    pad1->GetParentFootprint()->GetReference(),
                    pad1->GetNumber(),
                    pad2->GetParentFootprint()->GetReference(),
                    pad2->GetNumber(),
                    m_netSelector->GetValue() );
    }
    else
    {
        msg.Printf( _( "Changing the net will also update %lu connected pads to %s." ),
                    static_cast<unsigned long>( changingPads.size() ),
                    m_netSelector->GetValue() );
    }

    KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetOKCancelLabels( _( "Change Nets" ), _( "Leave Nets Unchanged" ) );
    dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

    return dlg.ShowModal() == wxID_OK;
}


bool DIALOG_TRACK_VIA_PROPERTIES::TransferDataFromWindow()
{
    // Check for malformed data ONLY; design rules and constraints are the business of DRC.

    if( m_vias )
    {
        if( !m_viaDiameter.Validate( GEOMETRY_MIN_SIZE, INT_MAX )
            || !m_viaDrill.Validate( GEOMETRY_MIN_SIZE, INT_MAX ) )
        {
            return false;
        }

        if( m_ViaDiameterCtrl->IsEnabled() && !m_viaDiameter.IsIndeterminate()
            && m_ViaDrillCtrl->IsEnabled() && !m_viaDrill.IsIndeterminate()
            && m_viaDiameter.GetValue() <= m_viaDrill.GetValue() )
        {
            DisplayError( GetParent(), _( "Via hole size must be smaller than via diameter" ) );
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
    //
    // We don't bother with updating the nets at this point as it will be useless (any connected
    // pads will simply drive their existing nets back onto the track segments and vias).

    BOARD_COMMIT commit( m_frame );
    bool         changeLock = m_lockedCbox->Get3StateValue() != wxCHK_UNDETERMINED;
    bool         setLock = m_lockedCbox->Get3StateValue() == wxCHK_CHECKED;

    for( EDA_ITEM* item : m_items )
    {
        commit.Modify( item );

        switch( item->Type() )
        {
            case PCB_TRACE_T:
            case PCB_ARC_T:
            {
                wxASSERT( m_tracks );
                PCB_TRACK* t = static_cast<PCB_TRACK*>( item );

                if( !m_trackStartX.IsIndeterminate() )
                    t->SetStartX( m_trackStartX.GetIntValue() );

                if( !m_trackStartY.IsIndeterminate() )
                    t->SetStartY( m_trackStartY.GetIntValue() );

                if( !m_trackEndX.IsIndeterminate() )
                    t->SetEndX( m_trackEndX.GetIntValue() );

                if( !m_trackEndY.IsIndeterminate() )
                    t->SetEndY( m_trackEndY.GetIntValue() );

                if( !m_trackWidth.IsIndeterminate() )
                    t->SetWidth( m_trackWidth.GetIntValue() );

                int layer = m_TrackLayerCtrl->GetLayerSelection();

                if( layer != UNDEFINED_LAYER )
                    t->SetLayer( (PCB_LAYER_ID) layer );

                if( changeLock )
                    t->SetLocked( setLock );

                break;
            }

            case PCB_VIA_T:
            {
                wxASSERT( m_vias );
                PCB_VIA* v = static_cast<PCB_VIA*>( item );

                if( !m_viaX.IsIndeterminate() )
                    v->SetPosition( VECTOR2I( m_viaX.GetIntValue(), v->GetPosition().y ) );

                if( !m_viaY.IsIndeterminate() )
                    v->SetPosition( VECTOR2I( v->GetPosition().x, m_viaY.GetIntValue() ) );

                if( m_viaNotFree->Get3StateValue() != wxCHK_UNDETERMINED )
                    v->SetIsFree( !m_viaNotFree->GetValue() );

                switch( m_ViaTypeChoice->GetSelection() )
                {
                case 0:
                    v->SetViaType( VIATYPE::THROUGH );
                    v->SanitizeLayers();
                    break;
                case 1:
                    v->SetViaType( VIATYPE::MICROVIA );
                    break;
                case 2:
                    v->SetViaType( VIATYPE::BLIND_BURIED );
                    break;
                default:
                    break;
                }

                auto startLayer = static_cast<PCB_LAYER_ID>( m_ViaStartLayer->GetLayerSelection() );
                auto endLayer = static_cast<PCB_LAYER_ID>( m_ViaEndLayer->GetLayerSelection() );

                if (startLayer != UNDEFINED_LAYER )
                    v->SetTopLayer( startLayer );

                if (endLayer != UNDEFINED_LAYER )
                    v->SetBottomLayer( endLayer );

                switch( m_annularRingsCtrl->GetSelection() )
                {
                case 0:
                    v->SetRemoveUnconnected( false );
                    break;
                case 1:
                    v->SetRemoveUnconnected( true );
                    v->SetKeepStartEnd( true );
                    break;
                case 2:
                    v->SetRemoveUnconnected( true );
                    v->SetKeepStartEnd( false );
                    break;
                default:
                    break;
                }

                v->SanitizeLayers();

                if( !m_viaDiameter.IsIndeterminate() )
                    v->SetWidth( m_viaDiameter.GetIntValue() );

                if( !m_viaDrill.IsIndeterminate() )
                    v->SetDrill( m_viaDrill.GetIntValue() );

                TEARDROP_PARAMETERS* targetParams = &v->GetTeardropParams();

                if( m_cbTeardrops->Get3StateValue() != wxCHK_UNDETERMINED )
                    targetParams->m_Enabled = m_cbTeardrops->GetValue();

                if( m_cbTeardropsUseNextTrack->Get3StateValue() != wxCHK_UNDETERMINED )
                    targetParams->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack->GetValue();

                if( !m_teardropMaxLen.IsIndeterminate() )
                    targetParams->m_TdMaxLen = m_teardropMaxLen.GetIntValue();

                if( !m_teardropMaxWidth.IsIndeterminate() )
                    targetParams->m_TdMaxWidth = m_teardropMaxWidth.GetIntValue();

                if( !m_teardropLenPercent.IsIndeterminate() )
                    targetParams->m_BestLengthRatio = m_teardropLenPercent.GetDoubleValue() / 100.0;

                if( !m_teardropWidthPercent.IsIndeterminate() )
                    targetParams->m_BestWidthRatio =
                            m_teardropWidthPercent.GetDoubleValue() / 100.0;

                if( !m_teardropHDPercent.IsIndeterminate() )
                    targetParams->m_WidthtoSizeFilterRatio = m_teardropHDPercent.GetDoubleValue() / 100.0;

                if( m_curvedEdges->Get3StateValue() != wxCHK_UNDETERMINED )
                {
                    if( m_curvedEdges->GetValue() )
                        targetParams->m_CurveSegCount = m_curvePointsCtrl->GetValue();
                    else
                        targetParams->m_CurveSegCount = 0;
                }

                if( changeLock )
                    v->SetLocked( setLock );

                break;
            }

            default:
                wxASSERT( false );
                break;
        }
    }

    commit.Push( _( "Edit track/via properties" ) );

    // Pushing the commit will have updated the connectivity so we can now test to see if we
    // need to update any pad nets.

    auto              connectivity = m_frame->GetBoard()->GetConnectivity();
    int               newNetCode = m_netSelector->GetSelectedNetcode();
    bool              updateNets = false;
    std::vector<PAD*> changingPads;

    if ( !m_netSelector->IsIndeterminate() )
    {
        updateNets = true;

        for( EDA_ITEM* item : m_items )
        {
            BOARD_CONNECTED_ITEM* boardItem = static_cast<BOARD_CONNECTED_ITEM*>( item );
            auto connectedItems = connectivity->GetConnectedItems( boardItem,
                    { PCB_TRACE_T, PCB_ARC_T, PCB_PAD_T, PCB_VIA_T, PCB_FOOTPRINT_T }, true );

            for ( BOARD_CONNECTED_ITEM* citem : connectedItems )
            {
                if( citem->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( citem );

                    if( pad->GetNetCode() != newNetCode && !alg::contains( changingPads, citem ) )
                        changingPads.push_back( pad );
                }
            }
        }
    }

    if( changingPads.size() && !confirmPadChange( changingPads ) )
        updateNets = false;

    if( updateNets )
    {
        for( EDA_ITEM* item : m_items )
        {
            commit.Modify( item );

            switch( item->Type() )
            {
                case PCB_TRACE_T:
                case PCB_ARC_T:
                    static_cast<PCB_TRACK*>( item )->SetNetCode( newNetCode );
                    break;

                case PCB_VIA_T:
                    static_cast<PCB_VIA*>( item )->SetNetCode( newNetCode );
                    break;

                default:
                    wxASSERT( false );
                    break;
            }
        }

        for( PAD* pad : changingPads )
        {
            commit.Modify( pad );
            pad->SetNetCode( newNetCode );
        }

        commit.Push( _( "Updating nets" ) );
    }

    return true;
}


void DIALOG_TRACK_VIA_PROPERTIES::onNetSelector( wxCommandEvent& aEvent )
{
    m_viaNotFree->SetValue( false );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaNotFreeClicked( wxCommandEvent& aEvent )
{
    if( !m_tracks )
    {
        m_netSelectorLabel->Enable( !m_viaNotFree->GetValue() );
        m_netSelector->Enable( !m_viaNotFree->GetValue() );
    }
}


void DIALOG_TRACK_VIA_PROPERTIES::onWidthSelect( wxCommandEvent& aEvent )
{
    m_TrackWidthCtrl->ChangeValue( m_predefinedTrackWidthsCtrl->GetStringSelection() );
    m_TrackWidthCtrl->SelectAll();
}


void DIALOG_TRACK_VIA_PROPERTIES::onWidthEdit( wxCommandEvent& aEvent )
{
    m_predefinedTrackWidthsCtrl->SetStringSelection( m_TrackWidthCtrl->GetValue() );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaSelect( wxCommandEvent& aEvent )
{
    VIA_DIMENSION* viaDimension = static_cast<VIA_DIMENSION*> ( aEvent.GetClientData() );

    m_viaDiameter.ChangeValue( viaDimension->m_Diameter );
    m_viaDrill.ChangeValue( viaDimension->m_Drill );
}


int DIALOG_TRACK_VIA_PROPERTIES::getLayerDepth()
{
    int viaType = m_ViaTypeChoice->GetSelection();

    if( viaType <= 0 )
        return m_frame->GetBoard()->GetCopperLayerCount() - 1;

    int startLayer = m_ViaStartLayer->GetLayerSelection();
    int endLayer = m_ViaEndLayer->GetLayerSelection();

    if( startLayer < 0 || endLayer < 0 )
        return m_frame->GetBoard()->GetCopperLayerCount() - 1;
    else
        return m_frame->GetBoard()->LayerDepth( ToLAYER_ID( startLayer ), ToLAYER_ID( endLayer ) );
}


void DIALOG_TRACK_VIA_PROPERTIES::onViaEdit( wxCommandEvent& aEvent )
{
    m_predefinedViaSizesCtrl->SetSelection( wxNOT_FOUND );

    if( m_vias )
    {
        if( m_ViaTypeChoice->GetSelection() != 0 ) // check if selected type isn't through.
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

        m_annularRingsLabel->Show( getLayerDepth() > 1 );
        m_annularRingsCtrl->Show( getLayerDepth() > 1 );
    }
}


void DIALOG_TRACK_VIA_PROPERTIES::onTeardropsUpdateUi( wxUpdateUIEvent& event )
{
    event.Enable( !m_frame->GetBoard()->LegacyTeardrops() );
}


void DIALOG_TRACK_VIA_PROPERTIES::onCurvedEdgesUpdateUi( wxUpdateUIEvent& event )
{
    event.Enable( !m_frame->GetBoard()->LegacyTeardrops()
                  && m_curvedEdges->Get3StateValue() == wxCHK_CHECKED );
}
