/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Ethan Chien <liangtie.qian@gmail.com>
 * Copyright (C) 2019 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2012 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright (C) 1992-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include "zone_manager/zones_container.h"

#include <wx/radiobut.h>
#include <kiface_base.h>
#include <confirm.h>
#include <pcb_edit_frame.h>
#include <pcbnew_settings.h>
#include <wx/string.h>
#include <widgets/unit_binder.h>
#include <pad.h>
#include <trigo.h>

#include <dialog_copper_zones_base.h>
#include <string_utils.h>

wxDEFINE_EVENT( EVT_ZONE_NAME_UPDATE, wxCommandEvent );

PANEL_ZONE_PROPERTIES::PANEL_ZONE_PROPERTIES( wxWindow* aParent, PCB_BASE_FRAME* aPCB_FRAME,
                                              ZONES_CONTAINER& aZoneContainer ) :
        PANEL_ZONE_PROPERTIES_BASE( aParent ),
        m_ZoneContainer( aZoneContainer ),
        m_PCB_Frame( aPCB_FRAME ),
        m_cornerSmoothingType( ZONE_SETTINGS::SMOOTHING_UNDEFINED ),
        m_outlineHatchPitch( aPCB_FRAME, m_stBorderHatchPitchText, m_outlineHatchPitchCtrl,
                             m_outlineHatchUnits ),
        m_cornerRadius( aPCB_FRAME, m_cornerRadiusLabel, m_cornerRadiusCtrl, m_cornerRadiusUnits ),
        m_clearance( aPCB_FRAME, m_clearanceLabel, m_clearanceCtrl, m_clearanceUnits ),
        m_minThickness( aPCB_FRAME, m_minWidthLabel, m_minWidthCtrl, m_minWidthUnits ),
        m_antipadClearance( aPCB_FRAME, m_antipadLabel, m_antipadCtrl, m_antipadUnits ),
        m_spokeWidth( aPCB_FRAME, m_spokeWidthLabel, m_spokeWidthCtrl, m_spokeWidthUnits ),
        m_gridStyleRotation( aPCB_FRAME, m_staticTextGrindOrient, m_tcGridStyleOrientation,
                             m_staticTextRotUnits ),
        m_gridStyleThickness( aPCB_FRAME, m_staticTextStyleThickness, m_tcGridStyleThickness,
                              m_GridStyleThicknessUnits ),
        m_gridStyleGap( aPCB_FRAME, m_staticTextGridGap, m_tcGridStyleGap, m_GridStyleGapUnits ),
        m_islandThreshold( aPCB_FRAME, m_islandThresholdLabel, m_tcIslandThreshold,
                           m_islandThresholdUnits ),
        m_isTeardrop()
{
    m_cbRemoveIslands->Bind( wxEVT_CHOICE,
                             [&]( wxCommandEvent& )
                             {
                                 // Area mode is index 2
                                 m_islandThreshold.Enable( m_cbRemoveIslands->GetSelection() == 2 );
                             } );
}


void PANEL_ZONE_PROPERTIES::ActivateSelectedZone( ZONE* aZone )
{
    if( m_settings )
        TransferZoneSettingsFromWindow();

    m_settings = m_ZoneContainer.GetZoneSettings( aZone );
    m_isTeardrop = m_settings->m_TeardropType != TEARDROP_TYPE::TD_NONE;

    TransferZoneSettingsToWindow();
}

void PANEL_ZONE_PROPERTIES::OnUserConfirmChange()
{
    TransferZoneSettingsFromWindow();
}


bool PANEL_ZONE_PROPERTIES::TransferZoneSettingsToWindow()
{
    if( !m_settings )
        return false;

    m_cbLocked->SetValue( m_settings->m_Locked );
    m_cornerSmoothingChoice->SetSelection( m_settings->GetCornerSmoothingType() );
    m_cornerRadius.SetValue( m_settings->GetCornerRadius() );

    if( m_isTeardrop ) // outlines are never smoothed: they have already the right shape
    {
        m_cornerSmoothingChoice->SetSelection( 0 );
        m_cornerSmoothingChoice->Enable( false );
        m_cornerRadius.SetValue( 0 );
        m_cornerRadius.Enable( false );
    }

    switch( m_settings->m_ZoneBorderDisplayStyle )
    {
    case ZONE_BORDER_DISPLAY_STYLE::NO_HATCH: m_OutlineDisplayCtrl->SetSelection( 0 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_EDGE: m_OutlineDisplayCtrl->SetSelection( 1 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::DIAGONAL_FULL: m_OutlineDisplayCtrl->SetSelection( 2 ); break;
    case ZONE_BORDER_DISPLAY_STYLE::INVISIBLE_BORDER: break;
    }

    m_outlineHatchPitch.SetValue( m_settings->m_BorderHatchPitch );

    m_clearance.SetValue( m_settings->m_ZoneClearance );
    m_minThickness.SetValue( m_settings->m_ZoneMinThickness );

    switch( m_settings->GetPadConnection() )
    {
    default:
    case ZONE_CONNECTION::THERMAL: m_PadInZoneOpt->SetSelection( 1 ); break;
    case ZONE_CONNECTION::THT_THERMAL: m_PadInZoneOpt->SetSelection( 2 ); break;
    case ZONE_CONNECTION::NONE: m_PadInZoneOpt->SetSelection( 3 ); break;
    case ZONE_CONNECTION::FULL: m_PadInZoneOpt->SetSelection( 0 ); break;
    }

    if( m_isTeardrop )
    {
        m_PadInZoneOpt->SetSelection( 0 );
        m_PadInZoneOpt->Enable( false );
    }

    // Do not enable/disable antipad clearance and spoke width.  They might be needed if
    // a footprint or pad overrides the zone to specify a thermal connection.
    m_antipadClearance.SetValue( m_settings->m_ThermalReliefGap );
    m_spokeWidth.SetValue( m_settings->m_ThermalReliefSpokeWidth );

    m_islandThreshold.SetDataType( EDA_DATA_TYPE::AREA );
    m_islandThreshold.SetDoubleValue( static_cast<double>( m_settings->GetMinIslandArea() ) );

    m_cbRemoveIslands->SetSelection( static_cast<int>( m_settings->GetIslandRemovalMode() ) );

    m_islandThreshold.Enable( m_settings->GetIslandRemovalMode() == ISLAND_REMOVAL_MODE::AREA );


    if( !m_isTeardrop && m_settings->m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
        m_GridStyleCtrl->SetSelection( 1 );
    else
        m_GridStyleCtrl->SetSelection( 0 );

    m_GridStyleCtrl->Enable( !m_isTeardrop );

    m_gridStyleRotation.SetUnits( EDA_UNITS::DEGREES );
    m_gridStyleRotation.SetAngleValue( m_settings->m_HatchOrientation );
    m_gridStyleThickness.SetValue( m_settings->m_HatchThickness );
    m_gridStyleGap.SetValue( m_settings->m_HatchGap );

    m_spinCtrlSmoothLevel->SetValue( m_settings->m_HatchSmoothingLevel );
    m_spinCtrlSmoothValue->SetValue( m_settings->m_HatchSmoothingValue );

    m_tcZoneName->SetValue( m_settings->m_Name );


    // Enable/Disable some widgets
    wxCommandEvent aEvent;
    OnStyleSelection( aEvent );
    Fit();

    return true;
}


void PANEL_ZONE_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& )
{
    if( m_cornerSmoothingType != m_cornerSmoothingChoice->GetSelection() )
    {
        m_cornerSmoothingType = m_cornerSmoothingChoice->GetSelection();

        if( m_cornerSmoothingChoice->GetSelection() == ZONE_SETTINGS::SMOOTHING_CHAMFER )
            m_cornerRadiusLabel->SetLabel( _( "Chamfer distance:" ) );
        else
            m_cornerRadiusLabel->SetLabel( _( "Fillet radius:" ) );
    }

    m_cornerRadiusCtrl->Enable( m_cornerSmoothingType > ZONE_SETTINGS::SMOOTHING_NONE );
}


void PANEL_ZONE_PROPERTIES::OnRemoveIslandsSelection( wxCommandEvent& aEvent )
{
    m_islandThreshold.Enable( m_cbRemoveIslands->GetSelection() == 2 );
}

void PANEL_ZONE_PROPERTIES::OnZoneNameChanged( wxCommandEvent& aEvent )
{
    wxCommandEvent* evt = new wxCommandEvent( EVT_ZONE_NAME_UPDATE );
    evt->SetString( m_tcZoneName->GetValue() );
    wxQueueEvent( m_parent, evt );
}


bool PANEL_ZONE_PROPERTIES::TransferZoneSettingsFromWindow()
{
    if( !m_settings )
        return false;

    if( m_GridStyleCtrl->GetSelection() > 0 )
        m_settings->m_FillMode = ZONE_FILL_MODE::HATCH_PATTERN;
    else
        m_settings->m_FillMode = ZONE_FILL_MODE::POLYGONS;

    if( !AcceptOptions() )
        return false;

    m_settings->m_HatchOrientation = m_gridStyleRotation.GetAngleValue();
    m_settings->m_HatchThickness = m_gridStyleThickness.GetIntValue();
    m_settings->m_HatchGap = m_gridStyleGap.GetIntValue();
    m_settings->m_HatchSmoothingLevel = m_spinCtrlSmoothLevel->GetValue();
    m_settings->m_HatchSmoothingValue = m_spinCtrlSmoothValue->GetValue();

    return true;
}


bool PANEL_ZONE_PROPERTIES::AcceptOptions( bool aUseExportableSetupOnly )
{
    if( !m_clearance.Validate( 0, pcbIUScale.mmToIU( ZONE_CLEARANCE_MAX_VALUE_MM ) ) )
        return false;

    if( !m_minThickness.Validate( pcbIUScale.mmToIU( ZONE_THICKNESS_MIN_VALUE_MM ), INT_MAX ) )
        return false;

    if( !m_cornerRadius.Validate( 0, INT_MAX ) )
        return false;

    if( !m_spokeWidth.Validate( 0, INT_MAX ) )
        return false;

    m_gridStyleRotation.SetValue( NormalizeAngle180( m_gridStyleRotation.GetValue() ) );

    if( m_settings->m_FillMode == ZONE_FILL_MODE::HATCH_PATTERN )
    {
        int minThickness = m_minThickness.GetIntValue();

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
        return false;

    m_settings->m_BorderHatchPitch = m_outlineHatchPitch.GetIntValue();

    m_settings->m_ZoneClearance = m_clearance.GetIntValue();
    m_settings->m_ZoneMinThickness = m_minThickness.GetIntValue();

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
        DisplayError( this, _( "Thermal spoke width cannot be smaller than the minimum width." ) );
        return false;
    }

    m_settings->SetIslandRemovalMode( (ISLAND_REMOVAL_MODE) m_cbRemoveIslands->GetSelection() );
    m_settings->SetMinIslandArea( m_islandThreshold.GetValue() );

    // If we use only exportable to others zones parameters, exit here:
    if( aUseExportableSetupOnly )
        return true;

    m_settings->m_Name = m_tcZoneName->GetValue();

    return true;
}


void PANEL_ZONE_PROPERTIES::OnStyleSelection( wxCommandEvent& aEvent )
{
    bool enable = m_GridStyleCtrl->GetSelection() >= 1;
    m_tcGridStyleThickness->Enable( enable );
    m_tcGridStyleGap->Enable( enable );
    m_tcGridStyleOrientation->Enable( enable );
    m_spinCtrlSmoothLevel->Enable( enable );
    m_spinCtrlSmoothValue->Enable( enable );
}
