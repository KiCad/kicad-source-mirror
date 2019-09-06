/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board_design_settings.h>

#include <panel_setup_feature_constraints.h>

#include <advanced_config.h>    // To be removed later, when the zone fill option will be always allowed


PANEL_SETUP_FEATURE_CONSTRAINTS::PANEL_SETUP_FEATURE_CONSTRAINTS( PAGED_DIALOG* aParent,
                                                                  PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_FEATURE_CONSTRAINTS_BASE( aParent->GetTreebook() ),
        m_trackMinWidth( aFrame, m_TrackMinWidthTitle, m_TrackMinWidthCtrl, m_TrackMinWidthUnits, true ),
        m_viaMinSize( aFrame, m_ViaMinTitle, m_SetViasMinSizeCtrl, m_ViaMinUnits, true ),
        m_viaMinDrill( aFrame, m_ViaMinDrillTitle, m_SetViasMinDrillCtrl, m_ViaMinDrillUnits, true ),
        m_uviaMinSize( aFrame, m_uviaMinSizeLabel, m_uviaMinSizeCtrl, m_uviaMinSizeUnits, true ),
        m_uviaMinDrill( aFrame, m_uviaMinDrillLabel, m_uviaMinDrillCtrl, m_uviaMinDrillUnits, true ),
        m_holeToHoleMin( aFrame, m_HoleToHoleTitle, m_SetHoleToHoleCtrl, m_HoleToHoleUnits, true ),
        m_edgeClearance( aFrame, m_EdgeClearanceLabel, m_EdgeClearanceCtrl, m_EdgeClearanceUnits, true ),
        m_maxError( aFrame, m_maxErrorTitle, m_maxErrorCtrl, m_maxErrorUnits, true )
{
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();
}


bool PANEL_SETUP_FEATURE_CONSTRAINTS::TransferDataToWindow()
{
    m_trackMinWidth.SetValue( m_BrdSettings->m_TrackMinWidth );
    m_viaMinSize.SetValue(m_BrdSettings->m_ViasMinSize );
    m_viaMinDrill.SetValue( m_BrdSettings->m_ViasMinDrill );

    m_OptAllowBlindBuriedVias->SetValue( m_BrdSettings->m_BlindBuriedViaAllowed );
    m_OptAllowMicroVias->SetValue( m_BrdSettings->m_MicroViasAllowed );

    m_uviaMinSize.SetValue( m_BrdSettings->m_MicroViasMinSize );
    m_uviaMinDrill.SetValue( m_BrdSettings->m_MicroViasMinDrill );

    m_holeToHoleMin.SetValue( m_BrdSettings->m_HoleToHoleMin );

    m_edgeClearance.SetValue( m_BrdSettings->m_CopperEdgeClearance );

    m_OptRequireCourtyards->SetValue( m_BrdSettings->m_RequireCourtyards );
    m_OptOverlappingCourtyards->SetValue( m_BrdSettings->m_ProhibitOverlappingCourtyards );

    m_maxError.SetValue( m_BrdSettings->m_MaxError );

    m_cbOutlinePolygonFastest->SetValue( m_BrdSettings->m_ZoneUseNoOutlineInFill );
    m_cbOutlinePolygonBestQ->SetValue( !m_BrdSettings->m_ZoneUseNoOutlineInFill );

    return true;
}


bool PANEL_SETUP_FEATURE_CONSTRAINTS::TransferDataFromWindow()
{
    // Update tracks minimum values for DRC
    m_BrdSettings->m_TrackMinWidth = m_trackMinWidth.GetValue();

    // Update vias minimum values for DRC
    m_BrdSettings->m_ViasMinSize = m_viaMinSize.GetValue();
    m_BrdSettings->m_ViasMinDrill = m_viaMinDrill.GetValue();

    m_BrdSettings->m_BlindBuriedViaAllowed = m_OptAllowBlindBuriedVias->GetValue();
    m_BrdSettings->m_MicroViasAllowed = m_OptAllowMicroVias->GetValue();

    // Update microvias minimum values for DRC
    m_BrdSettings->m_MicroViasMinSize = m_uviaMinSize.GetValue();
    m_BrdSettings->m_MicroViasMinDrill = m_uviaMinDrill.GetValue();

    m_BrdSettings->SetMinHoleSeparation( m_holeToHoleMin.GetValue() );

    m_BrdSettings->SetCopperEdgeClearance( m_edgeClearance.GetValue() );

    m_BrdSettings->SetRequireCourtyardDefinitions( m_OptRequireCourtyards->GetValue() );
    m_BrdSettings->SetProhibitOverlappingCourtyards( m_OptOverlappingCourtyards->GetValue() );

    m_BrdSettings->m_MaxError = Clamp<int>( IU_PER_MM * MINIMUM_ERROR_SIZE_MM,
            m_maxError.GetValue(), IU_PER_MM * MAXIMUM_ERROR_SIZE_MM );

    m_BrdSettings->m_ZoneUseNoOutlineInFill = m_cbOutlinePolygonFastest->GetValue();

    return true;
}


void PANEL_SETUP_FEATURE_CONSTRAINTS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}


void PANEL_SETUP_FEATURE_CONSTRAINTS::onChangeOutlineOpt( wxCommandEvent& event )
{
    wxObject* item =event.GetEventObject();

    if( item == m_cbOutlinePolygonBestQ )
        m_cbOutlinePolygonFastest->SetValue( not m_cbOutlinePolygonBestQ->GetValue() );
    else
        m_cbOutlinePolygonBestQ->SetValue( not m_cbOutlinePolygonFastest->GetValue() );
}
