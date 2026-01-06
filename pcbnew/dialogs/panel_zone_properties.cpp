/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include "panel_zone_properties.h"

#include <grid_tricks.h>
#include <wx/radiobut.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <dialog_shim.h>
#include <pcbnew_settings.h>
#include <zone_settings_bag.h>
#include <wx/string.h>
#include <widgets/unit_binder.h>
#include <widgets/std_bitmap_button.h>
#include <widgets/wx_grid.h>
#include <pad.h>
#include <grid_layer_box_helpers.h>


wxDEFINE_EVENT( EVT_ZONE_NAME_UPDATE, wxCommandEvent );
wxDEFINE_EVENT( EVT_ZONE_NET_UPDATE, wxCommandEvent );

PANEL_ZONE_PROPERTIES::PANEL_ZONE_PROPERTIES( wxWindow* aParent, PCB_BASE_FRAME* aFrame,
                                              ZONE_SETTINGS_BAG& aZonesSettingsBag, bool allowNetSpec ) :
        PANEL_ZONE_PROPERTIES_BASE( aParent ),
        m_frame( aFrame ),
        m_zonesSettingsBag( aZonesSettingsBag ),
        m_outlineHatchPitch( aFrame, m_stBorderHatchPitchText, m_outlineHatchPitchCtrl, m_outlineHatchUnits ),
        m_cornerRadius( aFrame, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_clearance( aFrame, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits ),
        m_minWidth( aFrame, m_minWidthLabel, m_minWidthCtrl, m_minWidthUnits ),
        m_antipadClearance( aFrame, m_antipadLabel, m_antipadCtrl, m_antipadUnits ),
        m_spokeWidth( aFrame, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits ),
        m_gridStyleRotation( aFrame, m_staticTextGrindOrient, m_tcGridStyleOrientation, m_staticTextRotUnits ),
        m_gridStyleThickness( aFrame, m_staticTextStyleThickness, m_tcGridStyleThickness, m_GridStyleThicknessUnits ),
        m_gridStyleGap( aFrame, m_staticTextGridGap, m_tcGridStyleGap, m_GridStyleGapUnits ),
        m_islandThreshold( aFrame, m_islandThresholdLabel, m_tcIslandThreshold, m_islandThresholdUnits )
{
    m_netSelector->SetNetInfo( &m_frame->GetBoard()->GetNetInfo() );

    if( !allowNetSpec )
    {
        m_netLabel->Hide();
        m_netSelector->Hide();
    }

    m_layerPropsTable = new LAYER_PROPERTIES_GRID_TABLE( m_frame,
            [&]() -> LSET
            {
                return m_settings->m_Layers;
            } );

    m_layerSpecificOverrides->SetTable( m_layerPropsTable, true );
    m_layerSpecificOverrides->PushEventHandler( new GRID_TRICKS( m_layerSpecificOverrides ) );
    m_layerSpecificOverrides->SetSelectionMode( wxGrid::wxGridSelectRows );

    wxGridCellAttr* attr = new wxGridCellAttr;
    attr->SetRenderer( new GRID_CELL_LAYER_RENDERER( nullptr ) );
    LSET forbiddenLayers = LSET::AllNonCuMask();

    for( PCB_LAYER_ID copper : LSET::AllCuMask() )
    {
        if( !m_frame->GetBoard()->IsLayerEnabled( copper ) )
            forbiddenLayers.set( copper );
    }

    attr->SetEditor( new GRID_CELL_LAYER_SELECTOR( nullptr, forbiddenLayers ) );
    m_layerSpecificOverrides->SetColAttr( 0, attr );
    m_layerSpecificOverrides->SetupColumnAutosizer( 0 );

    m_bpAddCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_plus ) );
    m_bpDeleteCustomLayer->SetBitmap( KiBitmapBundle( BITMAPS::small_trash ) );

    m_netSelector->Bind( FILTERED_ITEM_SELECTED, &PANEL_ZONE_PROPERTIES::onNetSelector, this );
}


PANEL_ZONE_PROPERTIES::~PANEL_ZONE_PROPERTIES()
{
    // we passed ownership of table to grid
    // delete m_layerPropsTable;

    m_layerSpecificOverrides->PopEventHandler( true );

    m_netSelector->Unbind( FILTERED_ITEM_SELECTED, &PANEL_ZONE_PROPERTIES::onNetSelector, this );
}


void PANEL_ZONE_PROPERTIES::SetZone( ZONE* aZone )
{
    if( m_settings )
        TransferZoneSettingsFromWindow();

    m_zone = aZone;
    m_settings = m_zonesSettingsBag.GetZoneSettings( aZone );
    m_isTeardrop = m_settings->m_TeardropType != TEARDROP_TYPE::TD_NONE;

    TransferZoneSettingsToWindow();
}


bool PANEL_ZONE_PROPERTIES::TransferZoneSettingsToWindow()
{
    if( !m_settings )
        return false;

    m_tcZoneName->ChangeValue( m_settings->m_Name );

    if( m_netSelector->IsShown() )
        m_netSelector->SetSelectedNetcode( std::max( 0, m_settings->m_Netcode ) );

    m_cbLocked->SetValue( m_settings->m_Locked );
    m_cornerSmoothingChoice->SetSelection( m_settings->GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings->GetCornerRadius() );

    if( m_isTeardrop ) // outlines are never smoothed: they have already the right shape
    {
        m_cornerSmoothingChoice->SetSelection( 0 );
        m_cornerSmoothingChoice->Enable( false );
        m_cornerRadius.Show( false );
    }

    switch( m_settings->m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH:         m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE:    m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL:    m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER: break;
    }

    m_outlineHatchPitch.SetValue( m_settings->m_BorderHatchPitch );

    m_clearance.SetValue( m_settings->m_ZoneClearance );
    m_minWidth.SetValue( m_settings->m_ZoneMinThickness );

    switch( m_settings->GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL:     m_PadInZoneOpt->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THT_THERMAL: m_PadInZoneOpt->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE:        m_PadInZoneOpt->SetSelection( 3 ); break;
    case ZONE_CONNECTION::FULL:        m_PadInZoneOpt->SetSelection( 0 ); break;
    }

    if( m_isTeardrop )
    {
        m_PadInZoneOpt->SetSelection( 0 );
        m_PadInZoneOpt->Enable( false );
        m_antipadClearance.Enable( false );
        m_spokeWidth.Enable( false );
    }

    // Do not enable/disable antipad clearance and spoke width.  They might be needed if
    // a footprint or pad overrides the zone to specify a thermal connection.
    m_antipadClearance.SetValue( m_settings->m_ThermalReliefGap );
    m_spokeWidth.SetValue( m_settings->m_ThermalReliefSpokeWidth );

    m_islandThreshold.SetDataType( EDA_DATA_TYPE::AREA );
    m_islandThreshold.SetDoubleValue( static_cast<double>( m_settings->GetMinIslandArea() ) );

    m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings->GetIslandRemovalMode() ) );

    m_cbHatched->SetValue( m_settings->m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN && !m_isTeardrop );

    m_gridStyleRotation.SetUnits( EDA_UNITS::DEGREES );
    m_gridStyleRotation.SetAngleValue( m_settings->m_HatchOrientation );
    m_gridStyleThickness.SetValue( m_settings->m_HatchThickness );
    m_gridStyleGap.SetValue( m_settings->m_HatchGap );

    m_spinCtrlSmoothLevel->SetValue( m_settings->m_HatchSmoothingLevel );
    m_spinCtrlSmoothValue->SetValue( m_settings->m_HatchSmoothingValue );

    m_layerPropsTable->Clear();

    for( PCB_LAYER_ID layer : LSET::AllCuMask().UIOrder() )
    {
        if( m_settings->m_LayerProperties.contains( layer ) )
        {
            ZONE_LAYER_PROPERTIES& props = m_settings->m_LayerProperties[layer];

            if( props.hatching_offset.has_value() )
                m_layerPropsTable->AddItem( layer, props );
        }
    }

    // Enable/Disable some widgets
    wxCommandEvent aEvent;
    onNetSelector( aEvent );
    OnCornerSmoothingSelection( aEvent );
    OnRemoveIslandsSelection( aEvent );
    onHatched( aEvent );

    return true;
}


void PANEL_ZONE_PROPERTIES::OnRemoveIslandsSelection( wxCommandEvent& aEvent )
{
    m_islandThreshold.Show( m_cbRemoveIslands->GetSelection() == 2 );

    if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
        dlg->Layout();
}


void PANEL_ZONE_PROPERTIES::OnCornerSmoothingSelection( wxCommandEvent& event )
{
    switch( m_cornerSmoothingChoice->GetSelection() )
    {
    case ZONE_SETTINGS::SMOOTHING_CHAMFER:
        m_cornerRadiusLabel->SetLabel( _( "Chamfer:" ) );
        m_cornerRadius.Show( true );
        break;

    case ZONE_SETTINGS::SMOOTHING_FILLET:
        m_cornerRadiusLabel->SetLabel( _( "Fillet:" ) );
        m_cornerRadius.Show( true );
        break;

    default:
        m_cornerRadius.Show( false );
        break;
    }

    if( DIALOG_SHIM* dlg = dynamic_cast<DIALOG_SHIM*>( wxGetTopLevelParent( this ) ) )
        dlg->Layout();
}


void PANEL_ZONE_PROPERTIES::OnZoneNameChanged( wxCommandEvent& aEvent )
{
    // Propagate all the way out so that the MODEL_ZONES_OVERVIEW can pick it up
    m_settings->m_Name = m_tcZoneName->GetValue();
    m_zonesSettingsBag.GetZoneSettings( m_zone )->m_Name = m_settings->m_Name;

    if( m_zone )
        m_zone->SetZoneName( m_settings->m_Name );

    wxCommandEvent* evt = new wxCommandEvent( EVT_ZONE_NAME_UPDATE );
    wxQueueEvent( m_parent, evt );
}


void PANEL_ZONE_PROPERTIES::onNetSelector( wxCommandEvent& aEvent )
{
    if( !m_netSelector->IsShown() )
        return;

    updateInfoBar();

    // Zones with no net never have islands removed
    if( m_netSelector->GetSelectedNetcode() == INVALID_NET_CODE )
    {
        if( m_cbRemoveIslands->IsEnabled() )
            m_settings->SetIslandRemovalMode( (ISLAND_REMOVAL_MODE) m_cbRemoveIslands->GetSelection() );

        m_cbRemoveIslands->SetSelection( 1 );
        m_removeIslandsLabel->Enable( false );
        m_cbRemoveIslands->Enable( false );
    }
    else if( !m_cbRemoveIslands->IsEnabled() )
    {
        m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings->GetIslandRemovalMode() ) );
        m_removeIslandsLabel->Enable( true );
        m_cbRemoveIslands->Enable( true );
    }

    // Propagate all the way out so that the MODEL_ZONES_OVERVIEW can pick it up
    m_settings->m_Netcode = m_netSelector->GetSelectedNetcode();
    m_zonesSettingsBag.GetZoneSettings( m_zone )->m_Netcode = m_settings->m_Netcode;

    if( m_zone )
        m_zone->SetNetCode( m_settings->m_Netcode, true );

    wxCommandEvent* evt = new wxCommandEvent( EVT_ZONE_NET_UPDATE );
    wxQueueEvent( m_parent, evt );
}


bool PANEL_ZONE_PROPERTIES::TransferZoneSettingsFromWindow()
{
    if( !CommitPendingChanges() )
        return false;

    if( !m_settings )
        return false;

    if( !AcceptOptions() )
        return false;

    return true;
}


bool PANEL_ZONE_PROPERTIES::CommitPendingChanges()
{
    return m_layerSpecificOverrides->CommitPendingChanges();
}


bool PANEL_ZONE_PROPERTIES::AcceptOptions( bool aUseExportableSetupOnly )
{
    if( !m_clearance.Validate( 0, pcbIUScale.mmToIU( ZONE_CLEARANCE_MAX_VALUE_MM ) ) )
        return false;

    if( !m_minWidth.Validate( pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM ), INT_MAX ) )
        return false;

    if( !m_cornerRadius.Validate( 0, INT_MAX ) )
        return false;

    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    m_gridStyleRotation.SetValue( NormalizeAngle180( m_gridStyleRotation.GetValue() ) );

    if( m_settings->m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        int minThickness = m_minWidth.GetIntValue();

        if( !m_gridStyleThickness.Validate( minThickness, INT_MAX ) )
            return false;

        if( !m_gridStyleGap.Validate( minThickness, INT_MAX ) )
            return false;
    }

    switch( m_PadInZoneOpt->GetSelection() )
    {
    case 3: m_settings->SetPadConnection( ZONE_CONNECTION::NONE ); break;
    case 2: m_settings->SetPadConnection( ZONE_CONNECTION::THT_THERMAL ); break;
    case 1: m_settings->SetPadConnection( ZONE_CONNECTION::THERMAL ); break;
    case 0: m_settings->SetPadConnection( ZONE_CONNECTION::FULL ); break;
    }

    switch( m_OutlineDisplayCtrl->GetSelection() )
    {
    case 0: m_settings->m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::NO_HATCH; break;
    case 1: m_settings->m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE; break;
    case 2: m_settings->m_ZoneBorderDisplayStyle = ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL; break;
    }

    if( !m_outlineHatchPitch.Validate( pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MINDIST_MM ),
                                       pcbIUScale.mmToIU( ZONE_BORDER_HATCH_MAXDIST_MM ) ) )
    {
        return false;
    }

    m_settings->m_BorderHatchPitch = m_outlineHatchPitch.GetIntValue();

    m_settings->m_ZoneClearance = m_clearance.GetIntValue();
    m_settings->m_ZoneMinThickness = m_minWidth.GetIntValue();

    m_settings->SetCornerSmoothingType( m_cornerSmoothingChoice->GetSelection() );

    if( m_settings->GetCornerSmoothingType() == ZONE_SETTINGS::SMOOTHING_NONE )
        m_settings->SetCornerRadius( 0 );
    else
        m_settings->SetCornerRadius( m_cornerRadius.GetIntValue() );

    m_settings->m_Locked = m_cbLocked->GetValue();

    m_settings->m_ThermalReliefGap = m_antipadClearance.GetValue();
    m_settings->m_ThermalReliefSpokeWidth = m_spokeWidth.GetValue();

    if( m_settings->m_ThermalReliefSpokeWidth < m_settings->m_ZoneMinThickness )
    {
        DisplayErrorMessage( this, _( "Thermal spoke width cannot be smaller than the minimum width." ) );
        return false;
    }

    m_settings->SetIslandRemovalMode( (ISLAND_REMOVAL_MODE) m_cbRemoveIslands->GetSelection() );
    m_settings->SetMinIslandArea( m_islandThreshold.GetValue() );

    // If we use only exportable to others zones parameters, exit here:
    if( aUseExportableSetupOnly )
        return true;

    if( m_netSelector->IsShown() )
        m_settings->m_Netcode = m_netSelector->GetSelectedNetcode();

    m_settings->m_Name = m_tcZoneName->GetValue();

    m_settings->m_FillMode = m_cbHatched->GetValue() ? ZONE_FILL_MODE::HATCH_PATTERN : ZONE_FILL_MODE::POLYGONS;
    m_settings->m_HatchOrientation = m_gridStyleRotation.GetAngleValue();
    m_settings->m_HatchThickness = m_gridStyleThickness.GetIntValue();
    m_settings->m_HatchGap = m_gridStyleGap.GetIntValue();
    m_settings->m_HatchSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings->m_HatchSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    for( auto& [layer, props] : m_settings->m_LayerProperties )
        props.hatching_offset = std::nullopt;

    for( const auto& [layer, props] : m_layerPropsTable->GetItems() )
        m_settings->m_LayerProperties[layer] = props;

    return true;
}


void PANEL_ZONE_PROPERTIES::onHatched( wxCommandEvent& event )
{
    bool enable = m_cbHatched->GetValue();
    m_gridStyleThickness.Enable( enable );
    m_gridStyleGap.Enable( enable );
    m_gridStyleRotation.Enable( enable );
    m_staticTextGridSmoothingLevel->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_staticTextGridSmootingVal->Enable( enable );
    m_spinCtrlSmoothValue->Enable( enable );
    m_offsetOverridesLabel->Enable( enable );
    m_layerSpecificOverrides->Enable( enable );
    m_bpAddCustomLayer->Enable( enable );
    m_bpDeleteCustomLayer->Enable( enable );
}


void PANEL_ZONE_PROPERTIES::OnAddLayerItem( wxCommandEvent& event )
{
    m_layerSpecificOverrides->OnAddRow(
            [&]() -> std::pair<int, int>
            {
                if( !m_layerSpecificOverrides->GetTable()->AppendRows( 1 ) )
                {
                    m_copperZoneInfoBar->ShowMessageFor( _( "All zone layers already overridden." ), 8000,
                                                         wxICON_WARNING );
                }

                return { m_layerSpecificOverrides->GetNumberRows() - 1, -1 };
            } );
}


void PANEL_ZONE_PROPERTIES::OnDeleteLayerItem( wxCommandEvent& event )
{
    m_layerSpecificOverrides->OnDeleteRows(
            [&]( int row )
            {
                m_layerSpecificOverrides->GetTable()->DeleteRows( row, 1 );
            } );
}


void PANEL_ZONE_PROPERTIES::updateInfoBar()
{
    if( m_netSelector->GetSelectedNetcode() <= INVALID_NET_CODE && !m_copperZoneInfoBar->IsShown() )
    {
        m_copperZoneInfoBar->ShowMessage( _( "<no net> will result in an isolated copper island." ), wxICON_WARNING );
    }
    else if( m_copperZoneInfoBar->IsShown() )
    {
        m_copperZoneInfoBar->Dismiss();
    }
}
