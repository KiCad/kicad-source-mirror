/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
#include <confirm.h>
#include <kidialog.h>
#include <connectivity/connectivity_data.h>
#include <board_commit.h>
#include <magic_enum.hpp>
#include <macros.h>


bool DIALOG_TRACK_VIA_PROPERTIES::IPC4761_CONFIGURATION::operator==(
        const IPC4761_CONFIGURATION& aOther ) const
{
    return ( tent == aOther.tent ) && ( plug == aOther.plug ) && ( cover == aOther.cover )
           && ( cap == aOther.cap ) && ( fill == aOther.fill );
}


DIALOG_TRACK_VIA_PROPERTIES::DIALOG_TRACK_VIA_PROPERTIES( PCB_BASE_EDIT_FRAME* aParent,
                                                          const PCB_SELECTION& aItems ) :
        DIALOG_TRACK_VIA_PROPERTIES_BASE( aParent ),
        m_frame( aParent ),
        m_items( aItems ),
        m_trackStartX( aParent, m_TrackStartXLabel, m_TrackStartXCtrl, nullptr ),
        m_trackStartY( aParent, m_TrackStartYLabel, m_TrackStartYCtrl, m_TrackStartYUnit ),
        m_trackEndX( aParent, m_TrackEndXLabel, m_TrackEndXCtrl, nullptr ),
        m_trackEndY( aParent, m_TrackEndYLabel, m_TrackEndYCtrl, m_TrackEndYUnit ),
        m_trackWidth( aParent, m_TrackWidthLabel, m_TrackWidthCtrl, m_TrackWidthUnit ),
        m_trackMaskMargin( aParent, m_trackMaskMarginLabel, m_trackMaskMarginCtrl, m_trackMaskMarginUnit ),
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
        m_vias( false ),
        m_editLayer( PADSTACK::ALL_LAYERS )
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

    wxFont infoFont = KIUI::GetSmallInfoFont( this );
    m_techLayersLabel->SetFont( infoFont );

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

    auto getAnnularRingSelection = []( const PCB_VIA* via ) -> int
    {
        switch( via->Padstack().UnconnectedLayerMode() )
        {
        default:
        case PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL: return 0;
        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END: return 1;
        case PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL: return 2;
        case PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY: return 3;
        }
    };

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

    auto getProtectionSurface = []( const std::optional<bool>& front,
                                    const std::optional<bool>& back ) -> IPC4761_SURFACE
    {
        IPC4761_SURFACE value = IPC4761_SURFACE::CUSTOM;

        if( !front.has_value() )
            value = IPC4761_SURFACE::FROM_RULES;
        else if( front.value() )
            value = IPC4761_SURFACE::FRONT;
        else
            value = IPC4761_SURFACE::NONE;

        if( !back.has_value() )
        {
            if( value == IPC4761_SURFACE::FROM_RULES )
                return IPC4761_SURFACE::FROM_RULES;
        }
        else if( back.value() )
        {
            if( value == IPC4761_SURFACE::FRONT )
                return IPC4761_SURFACE::BOTH;
            else if( value == IPC4761_SURFACE::NONE )
                return IPC4761_SURFACE::BACK;
        }
        else
        {
            if( value == IPC4761_SURFACE::FRONT )
                return IPC4761_SURFACE::FRONT;
            else if( value == IPC4761_SURFACE::NONE )
                return IPC4761_SURFACE::NONE;
        }

        return IPC4761_SURFACE::CUSTOM;
    };

    auto getProtectionDrill = []( const std::optional<bool>& drill ) -> IPC4761_DRILL
    {
        if( !drill.has_value() )
            return IPC4761_DRILL::FROM_RULES;
        if( drill.value() )
            return IPC4761_DRILL::SET;

        return IPC4761_DRILL::NOT_SET;
    };

    auto getViaConfiguration = [&]( const PCB_VIA* via ) -> IPC4761_PRESET
    {
        IPC4761_CONFIGURATION config;
        config.tent = getProtectionSurface( via->Padstack().FrontOuterLayers().has_solder_mask,
                                            via->Padstack().BackOuterLayers().has_solder_mask );

        config.cover = getProtectionSurface( via->Padstack().FrontOuterLayers().has_covering,
                                             via->Padstack().BackOuterLayers().has_covering );

        config.plug = getProtectionSurface( via->Padstack().FrontOuterLayers().has_plugging,
                                            via->Padstack().BackOuterLayers().has_plugging );

        config.cap = getProtectionDrill( via->Padstack().Drill().is_capped );

        config.fill = getProtectionDrill( via->Padstack().Drill().is_filled );

        for( const auto& [preset, configuration] : m_IPC4761Presets )
        {
            if( configuration == config )
                return preset;
        }

        return IPC4761_PRESET::CUSTOM;
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
                    m_trackHasSolderMask->SetValue ( t->HasSolderMask() );

                    if( t->GetLocalSolderMaskMargin().has_value() )
                        m_trackMaskMargin.SetValue( t->GetLocalSolderMaskMargin().value() );
                    else
                        m_trackMaskMargin.SetValue( wxEmptyString );

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

                    if( m_trackHasSolderMask->GetValue() != t->HasSolderMask() )
                        m_trackHasSolderMask->Set3StateValue( wxCHK_UNDETERMINED );

                    if( m_trackMaskMargin.GetValue() != t->GetLocalSolderMaskMargin() )
                        m_trackMaskMargin.SetValue( INDETERMINATE_STATE );
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
                    m_viaStack = std::make_unique<PADSTACK>( v->Padstack() );
                    m_viaDiameter.SetValue( v->GetWidth( m_editLayer ) );
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
                    m_curvedEdges->SetValue( v->GetTeardropParams().m_CurvedEdges );

                    IPC4761_PRESET preset = getViaConfiguration( v );

                    if( preset >= IPC4761_PRESET::CUSTOM )
                    {
                        m_protectionFeatures->SetSelection(
                                m_protectionFeatures->Append( INDETERMINATE_ACTION ) );
                    }
                    else
                    {
                        m_protectionFeatures->SetSelection( static_cast<int>( preset ) );
                    }
                }
                else        // check if values are the same for every selected via
                {
                    if( m_viaX.GetValue() != v->GetPosition().x )
                        m_viaX.SetValue( INDETERMINATE_STATE );

                    if( m_viaY.GetValue() != v->GetPosition().y )
                        m_viaY.SetValue( INDETERMINATE_STATE );

                    if( m_viaDiameter.GetValue() != v->GetWidth( m_editLayer ) )
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


                    if( static_cast<int>( getViaConfiguration( v ) )
                        != m_protectionFeatures->GetSelection() )
                    {
                        m_protectionFeatures->SetSelection(
                                m_protectionFeatures->Append( INDETERMINATE_STATE ) );
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
                UNIMPLEMENTED_FOR( item->GetClass() );
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
        m_annularRingsCtrl->Enable( true );

        afterPadstackModeChanged();
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

        wxCommandEvent event;
        onTrackEdit( event );
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
    m_netSelector->Bind( FILTERED_ITEM_SELECTED, &DIALOG_TRACK_VIA_PROPERTIES::onNetSelector, this );

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


bool DIALOG_TRACK_VIA_PROPERTIES::confirmShortingNets( int aNet, const std::set<int>& shortingNets )
{
    wxString msg;

    if( shortingNets.size() == 1 )
    {
        msg.Printf( _( "Applying these changes will short net %s with %s." ),
                    m_netSelector->GetValue(),
                    m_frame->GetBoard()->FindNet( *shortingNets.begin() )->GetNetname() );
    }
    else
    {
        msg.Printf( _( "Applying these changes will short net %s with other nets." ),
                    m_netSelector->GetValue() );
    }

    KIDIALOG dlg( this, msg, _( "Confirmation" ), wxOK | wxCANCEL | wxICON_WARNING );
    dlg.SetOKCancelLabels( _( "Apply Anyway" ), _( "Cancel Changes" ) );
    dlg.DoNotShowCheckbox( __FILE__, __LINE__ );

    return dlg.ShowModal() == wxID_OK;
}


bool DIALOG_TRACK_VIA_PROPERTIES::confirmPadChange( const std::set<PAD*>& changingPads )
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
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = m_frame->GetBoard()->GetConnectivity();
    std::vector<PCB_TRACK*>            selected_tracks;
    std::set<PCB_TRACK*>               connected_tracks;

    for( EDA_ITEM* item : m_items )
    {
        if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( item ) )
            selected_tracks.push_back( track );
    }

    for( PCB_TRACK* selected_track : selected_tracks )
    {
        for( BOARD_CONNECTED_ITEM* connected_item : connectivity->GetConnectedItems( selected_track ) )
        {
            if( PCB_TRACK* track = dynamic_cast<PCB_TRACK*>( connected_item ) )
                connected_tracks.insert( track );
        }
    }

    // Check for malformed data ONLY; design rules and constraints are the business of DRC.

    if( m_vias )
    {
        // TODO: This needs to move into the via class, not the dialog

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

        if( !m_viaDiameter.IsIndeterminate() )
        {
            int diameter = m_viaDiameter.GetValue();
            m_viaStack->SetSize( { diameter, diameter }, m_editLayer );
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

    for( PCB_TRACK* track : selected_tracks )
    {
        commit.Modify( track );

        switch( track->Type() )
        {
            case PCB_TRACE_T:
            case PCB_ARC_T:
            {
                wxASSERT( m_tracks );

                if( !m_trackStartX.IsIndeterminate() )
                    track->SetStartX( m_trackStartX.GetIntValue() );

                if( !m_trackStartY.IsIndeterminate() )
                    track->SetStartY( m_trackStartY.GetIntValue() );

                if( !m_trackEndX.IsIndeterminate() )
                    track->SetEndX( m_trackEndX.GetIntValue() );

                if( !m_trackEndY.IsIndeterminate() )
                    track->SetEndY( m_trackEndY.GetIntValue() );

                if( !m_trackWidth.IsIndeterminate() )
                    track->SetWidth( m_trackWidth.GetIntValue() );

                int layer = m_TrackLayerCtrl->GetLayerSelection();

                if( layer != UNDEFINED_LAYER )
                    track->SetLayer( (PCB_LAYER_ID) layer );

                if ( m_trackHasSolderMask->Get3StateValue() != wxCHK_UNDETERMINED )
                    track->SetHasSolderMask( m_trackHasSolderMask->GetValue() );

                if( !m_trackMaskMargin.IsIndeterminate() )
                {
                    if( m_trackMaskMargin.IsNull() )
                        track->SetLocalSolderMaskMargin( {} );
                    else
                        track->SetLocalSolderMaskMargin( m_trackMaskMargin.GetIntValue() );
                }

                if( changeLock )
                    track->SetLocked( setLock );

                break;
            }

            case PCB_VIA_T:
            {
                wxASSERT( m_vias );
                PCB_VIA* via = static_cast<PCB_VIA*>( track );

                if( !m_viaX.IsIndeterminate() )
                    via->SetPosition( VECTOR2I( m_viaX.GetIntValue(), via->GetPosition().y ) );

                if( !m_viaY.IsIndeterminate() )
                    via->SetPosition( VECTOR2I( via->GetPosition().x, m_viaY.GetIntValue() ) );

                if( m_viaNotFree->Get3StateValue() != wxCHK_UNDETERMINED )
                    via->SetIsFree( !m_viaNotFree->GetValue() );

                if( !m_viaDiameter.IsIndeterminate() )
                    via->SetPadstack( *m_viaStack );

                switch( m_ViaTypeChoice->GetSelection() )
                {
                case 0: via->SetViaType( VIATYPE::THROUGH );      break;
                case 1: via->SetViaType( VIATYPE::MICROVIA );     break;
                case 2: via->SetViaType( VIATYPE::BLIND_BURIED ); break;
                default:                                          break;
                }

                PCB_LAYER_ID startLayer = static_cast<PCB_LAYER_ID>( m_ViaStartLayer->GetLayerSelection() );
                PCB_LAYER_ID endLayer = static_cast<PCB_LAYER_ID>( m_ViaEndLayer->GetLayerSelection() );

                if( startLayer != UNDEFINED_LAYER )
                {
                    m_viaStack->Drill().start = startLayer;
                    via->SetTopLayer( startLayer );
                }

                if( endLayer != UNDEFINED_LAYER )
                {
                    m_viaStack->Drill().end = endLayer;
                    via->SetBottomLayer( endLayer );
                }

                via->SanitizeLayers();

                switch( m_annularRingsCtrl->GetSelection() )
                {
                case 0:
                    via->Padstack().SetUnconnectedLayerMode(
                            PADSTACK::UNCONNECTED_LAYER_MODE::KEEP_ALL );
                    break;
                case 1:
                    via->Padstack().SetUnconnectedLayerMode(
                            PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_EXCEPT_START_AND_END );
                    break;
                case 2:
                    via->Padstack().SetUnconnectedLayerMode(
                            PADSTACK::UNCONNECTED_LAYER_MODE::REMOVE_ALL );
                    break;
                case 3:
                    via->Padstack().SetUnconnectedLayerMode(
                            PADSTACK::UNCONNECTED_LAYER_MODE::START_END_ONLY );
                    break;
                default:
                    break;
                }


                if( !m_viaDrill.IsIndeterminate() )
                    via->SetDrill( m_viaDrill.GetIntValue() );

                TEARDROP_PARAMETERS* targetParams = &via->GetTeardropParams();

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
                    targetParams->m_CurvedEdges = m_curvedEdges->GetValue();

                if( changeLock )
                    via->SetLocked( setLock );

                auto setSurfaceProtection =
                        [&]( std::optional<bool>& aFront, std::optional<bool>& aBack, IPC4761_SURFACE aProtection )
                        {
                            switch( aProtection )
                            {
                            case IPC4761_SURFACE::FROM_RULES:
                                aFront.reset();
                                aBack.reset();
                                break;
                            case IPC4761_SURFACE::NONE:
                                aFront = false;
                                aBack = false;
                                break;
                            case IPC4761_SURFACE::FRONT:
                                aFront = true;
                                aBack = false;
                                break;
                            case IPC4761_SURFACE::BACK:
                                aFront = false;
                                aBack = true;
                                break;
                            case IPC4761_SURFACE::BOTH:
                                aFront = true;
                                aBack = true;
                                break;
                            case IPC4761_SURFACE::CUSTOM: return;
                            }
                        };

                auto setDrillProtection =
                        [&]( std::optional<bool>& aDrill, IPC4761_DRILL aProtection )
                        {
                            switch( aProtection )
                            {
                            case IPC4761_DRILL::FROM_RULES: aDrill.reset(); break;
                            case IPC4761_DRILL::NOT_SET:    aDrill = false; break;
                            case IPC4761_DRILL::SET:        aDrill = true;  break;
                            }
                        };

                IPC4761_PRESET selectedPreset = static_cast<IPC4761_PRESET>( m_protectionFeatures->GetSelection() );

                if( selectedPreset < IPC4761_PRESET::CUSTOM ) // Do not change custom feaure list.
                {
                    const IPC4761_CONFIGURATION config = m_IPC4761Presets.at( selectedPreset );

                    setSurfaceProtection( via->Padstack().FrontOuterLayers().has_solder_mask,
                                          via->Padstack().BackOuterLayers().has_solder_mask,
                                          config.tent );

                    setSurfaceProtection( via->Padstack().FrontOuterLayers().has_plugging,
                                          via->Padstack().BackOuterLayers().has_plugging,
                                          config.plug );

                    setSurfaceProtection( via->Padstack().FrontOuterLayers().has_covering,
                                          via->Padstack().BackOuterLayers().has_covering,
                                          config.cover );

                    setDrillProtection( via->Padstack().Drill().is_filled, config.fill );

                    setDrillProtection( via->Padstack().Drill().is_capped, config.cap );
                }

                break;
            }

            default:
                UNIMPLEMENTED_FOR( track->GetClass() );
                break;
        }
    }

    std::set<int>  shortingNets;
    int            newNetCode = m_netSelector->GetSelectedNetcode();
    std::set<PAD*> changingPads;

    // Do NOT use the connectivity code here.  It will propagate through zones, and we haven't
    // refilled those yet so it's going to pick up a whole bunch of other nets any time the track
    // width was increased.
    auto collide =
            [&]( BOARD_CONNECTED_ITEM* a, BOARD_CONNECTED_ITEM* b )
            {
                for( PCB_LAYER_ID layer : LSET( a->GetLayerSet() & b->GetLayerSet() ) )
                {
                    if( a->GetEffectiveShape( layer )->Collide( b->GetEffectiveShape( layer ).get() ) )
                        return true;
                }

                return false;
            };

    for( PCB_TRACK* track : connected_tracks )
    {
        for( PCB_TRACK* other : m_frame->GetBoard()->Tracks() )
        {
            if( other->GetNetCode() == track->GetNetCode() || other->GetNetCode() == newNetCode )
                continue;

            if( collide( track, other ) )
                shortingNets.insert( other->GetNetCode() );
        }

        for( FOOTPRINT* footprint : m_frame->GetBoard()->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( pad->GetNetCode() == newNetCode )
                    continue;

                if( collide( track, pad ) )
                {
                    if( pad->GetNetCode() == track->GetNetCode() )
                        changingPads.insert( pad );
                    else
                        shortingNets.insert( pad->GetNetCode() );
                }
            }
        }
    }

    if( shortingNets.size() && !confirmShortingNets( newNetCode, shortingNets ) )
    {
        commit.Revert();
        return true;
    }

    if( !m_netSelector->IsIndeterminate() )
    {
        if( changingPads.empty() || confirmPadChange( changingPads ) )
        {
            for( PCB_TRACK* track : selected_tracks )
                track->SetNetCode( newNetCode );

            for( PAD* pad : changingPads )
            {
                commit.Modify( pad );
                pad->SetNetCode( newNetCode );
            }
        }
    }

    commit.Push( _( "Edit Track/Via Properties" ) );
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


void DIALOG_TRACK_VIA_PROPERTIES::onPadstackModeChanged( wxCommandEvent& aEvent )
{
    wxCHECK_MSG( m_viaStack, /* void */, "Expected valid via stack in onPadstackModeChanged" );

    switch( m_cbPadstackMode->GetSelection() )
    {
    default:
    case 0: m_viaStack->SetMode( PADSTACK::MODE::NORMAL );           break;
    case 1: m_viaStack->SetMode( PADSTACK::MODE::FRONT_INNER_BACK ); break;
    case 2: m_viaStack->SetMode( PADSTACK::MODE::CUSTOM );           break;
    }

    afterPadstackModeChanged();
}


void DIALOG_TRACK_VIA_PROPERTIES::onEditLayerChanged( wxCommandEvent& aEvent )
{
    wxCHECK_MSG( m_viaStack, /* void */, "Expected valid via stack in onEditLayerChanged" );

    // Save data from the previous layer
    if( !m_viaDiameter.IsIndeterminate() )
    {
        int diameter = m_viaDiameter.GetValue();
        m_viaStack->SetSize( { diameter, diameter }, m_editLayer );
    }

    switch( m_viaStack->Mode() )
    {
    default:
    case PADSTACK::MODE::NORMAL:
        m_editLayer = PADSTACK::ALL_LAYERS;
        break;

    case PADSTACK::MODE::FRONT_INNER_BACK:
        switch( m_cbEditLayer->GetSelection() )
        {
    default:
    case 0: m_editLayer = F_Cu;                   break;
    case 1: m_editLayer = PADSTACK::INNER_LAYERS; break;
    case 2: m_editLayer = B_Cu;                   break;
        }
        break;

    case PADSTACK::MODE::CUSTOM:
    {
        int layer = m_cbEditLayer->GetSelection();

        if( layer < 0 )
            layer = 0;

        if( m_editLayerCtrlMap.contains( layer ) )
            m_editLayer = m_editLayerCtrlMap.at( layer );
        else
            m_editLayer = F_Cu;
    }
    }

    // Load controls with the current layer
    m_viaDiameter.SetValue( m_viaStack->Size( m_editLayer ).x );
}


void DIALOG_TRACK_VIA_PROPERTIES::afterPadstackModeChanged()
{
    // NOTE: synchronize changes here with DIALOG_PAD_PROPERTIES::afterPadstackModeChanged

    wxCHECK_MSG( m_viaStack, /* void */, "Expected valid via stack in afterPadstackModeChanged" );
    m_cbEditLayer->Clear();

    BOARD* board = m_frame->GetBoard();

    switch( m_viaStack->Mode() )
    {
    case PADSTACK::MODE::NORMAL:
        m_cbPadstackMode->SetSelection( 0 );
        m_cbEditLayer->Append( _( "All layers" ) );
        m_cbEditLayer->Disable();
        m_editLayer = PADSTACK::ALL_LAYERS;
        m_editLayerCtrlMap = { { 0, PADSTACK::ALL_LAYERS } };
        break;

    case PADSTACK::MODE::FRONT_INNER_BACK:
    {
        m_cbPadstackMode->SetSelection( 1 );
        m_cbEditLayer->Enable();

        std::vector choices = {
            board->GetLayerName( F_Cu ),
            _( "Inner Layers" ),
            board->GetLayerName( B_Cu )
        };

        m_cbEditLayer->Append( choices );

        m_editLayerCtrlMap = {
            { 0, F_Cu },
            { 1, PADSTACK::INNER_LAYERS },
            { 2, B_Cu }
        };

        if( m_editLayer != F_Cu && m_editLayer != B_Cu )
            m_editLayer = PADSTACK::INNER_LAYERS;

        break;
    }

    case PADSTACK::MODE::CUSTOM:
    {
        m_cbPadstackMode->SetSelection( 2 );
        m_cbEditLayer->Enable();
        LSET layers = LSET::AllCuMask() & board->GetEnabledLayers();

        for( PCB_LAYER_ID layer : layers.UIOrder() )
        {
            int idx = m_cbEditLayer->Append( board->GetLayerName( layer ) );
            m_editLayerCtrlMap[idx] = layer;
        }

        break;
    }
    }

    for( const auto& [idx, layer] : m_editLayerCtrlMap )
    {
        if( layer == m_editLayer )
        {
            m_cbEditLayer->SetSelection( idx );
            break;
        }
    }
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
        m_annularRingsCtrl->Enable( true );
    }
}


void DIALOG_TRACK_VIA_PROPERTIES::onTrackEdit( wxCommandEvent& aEvent )
{
    bool externalCuLayer = m_TrackLayerCtrl->GetLayerSelection() == F_Cu
                               || m_TrackLayerCtrl->GetLayerSelection() == B_Cu;

    m_techLayersLabel->Enable( externalCuLayer );
    m_trackHasSolderMask->Enable( externalCuLayer );

    bool showMaskMargin = externalCuLayer && m_trackHasSolderMask->GetValue();

    m_trackMaskMarginCtrl->Enable( showMaskMargin );
    m_trackMaskMarginLabel->Enable( showMaskMargin );
    m_trackMaskMarginUnit->Enable( showMaskMargin );
}


void DIALOG_TRACK_VIA_PROPERTIES::onTeardropsUpdateUi( wxUpdateUIEvent& event )
{
    event.Enable( !m_frame->GetBoard()->LegacyTeardrops() );
}

