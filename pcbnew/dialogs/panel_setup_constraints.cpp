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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <algorithm>
#include <map>

#include <board_design_settings.h>
#include <board.h>
#include <math/util.h>
#include <panel_setup_constraints.h>
#include <panel_setup_constraints_base.h>
#include <pcb_edit_frame.h>
#include <widgets/paged_dialog.h>
#include <wx/treebook.h>
#include <bitmaps.h>
#include <advanced_config.h>


PANEL_SETUP_CONSTRAINTS::PANEL_SETUP_CONSTRAINTS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_CONSTRAINTS_BASE( aParentWindow ),
        m_minClearance( aFrame, m_clearanceTitle, m_clearanceCtrl, m_clearanceUnits ),
        m_minConn( aFrame, m_MinConnTitle, m_MinConnCtrl, m_MinConnUnits ),
        m_trackMinWidth( aFrame, m_TrackMinWidthTitle, m_TrackMinWidthCtrl, m_TrackMinWidthUnits ),
        m_viaMinAnnulus( aFrame, m_ViaMinAnnulusTitle, m_ViaMinAnnulusCtrl, m_ViaMinAnnulusUnits ),
        m_viaMinSize( aFrame, m_ViaMinTitle, m_SetViasMinSizeCtrl, m_ViaMinUnits ),
        m_throughHoleMin( aFrame, m_MinDrillTitle, m_MinDrillCtrl, m_MinDrillUnits ),
        m_uviaMinSize( aFrame, m_uviaMinSizeLabel, m_uviaMinSizeCtrl, m_uviaMinSizeUnits ),
        m_uviaMinDrill( aFrame, m_uviaMinDrillLabel, m_uviaMinDrillCtrl, m_uviaMinDrillUnits ),
        m_holeToHoleMin( aFrame, m_HoleToHoleTitle, m_SetHoleToHoleCtrl, m_HoleToHoleUnits ),
        m_holeClearance( aFrame, m_HoleClearanceLabel, m_HoleClearanceCtrl, m_HoleClearanceUnits ),
        m_edgeClearance( aFrame, m_EdgeClearanceLabel, m_EdgeClearanceCtrl, m_EdgeClearanceUnits ),
        m_silkClearance( aFrame, m_silkClearanceLabel, m_silkClearanceCtrl, m_silkClearanceUnits ),
        m_minGrooveWidth( aFrame, m_minGrooveWidthLabel, m_minGrooveWidthCtrl, m_minGrooveWidthUnits ),
        m_minTextHeight( aFrame, m_textHeightLabel, m_textHeightCtrl, m_textHeightUnits ),
        m_minTextThickness( aFrame, m_textThicknessLabel, m_textThicknessCtrl, m_textThicknessUnits ),
        m_maxError( aFrame, m_maxErrorTitle, m_maxErrorCtrl, m_maxErrorUnits )
{
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();

    m_filletBitmap->SetBitmap( KiBitmapBundle( BITMAPS::zone_fillet, 24 ) );
    m_spokeBitmap->SetBitmap( KiBitmapBundle( BITMAPS::thermal_spokes, 24 ) );
    m_bitmapClearance->SetBitmap( KiBitmapBundle( BITMAPS::ps_diff_pair_gap, 24 ) );
    m_bitmapMinTrackWidth->SetBitmap( KiBitmapBundle( BITMAPS::width_track, 24 ) );
    m_bitmapMinConn->SetBitmap( KiBitmapBundle( BITMAPS::width_conn, 24 ) );
    m_bitmapMinViaAnnulus->SetBitmap( KiBitmapBundle( BITMAPS::via_annulus, 24 ) );
    m_bitmapMinViaDiameter->SetBitmap( KiBitmapBundle( BITMAPS::via_diameter, 24 ) );
    m_bitmapMinViaDrill->SetBitmap( KiBitmapBundle( BITMAPS::via_hole_diameter, 24 ) );
    m_bitmapMinuViaDiameter->SetBitmap( KiBitmapBundle( BITMAPS::via_diameter, 24 ) );
    m_bitmapMinuViaDrill->SetBitmap( KiBitmapBundle( BITMAPS::via_hole_diameter, 24 ) );
    m_bitmapHoleClearance->SetBitmap( KiBitmapBundle( BITMAPS::hole_to_copper_clearance, 24 ) );
    m_bitmapMinHoleClearance->SetBitmap( KiBitmapBundle( BITMAPS::hole_to_hole_clearance, 24 ) );
    m_bitmapEdgeClearance->SetBitmap( KiBitmapBundle( BITMAPS::edge_to_copper_clearance, 24 ) );

    m_stCircleToPolyWarning->SetFont( KIUI::GetSmallInfoFont( this ).Italic() );

    wxSize ctrlSize = m_minResolvedSpokeCountCtrl->GetSize();
    ctrlSize.x = KIUI::GetTextSize( wxT( "XXX" ), m_minResolvedSpokeCountCtrl ).x;
    m_minResolvedSpokeCountCtrl->SetSize( ctrlSize );

    if( !ADVANCED_CFG::GetCfg().m_EnableCreepageSlot )
    {
        m_bitmapMinGrooveWidth->Show( false );
        m_minGrooveWidthLabel->Show( false );
        m_minGrooveWidthCtrl->Show( false );
        m_minGrooveWidthUnits->Show( false );
    }
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataToWindow()
{
    wxString msg;
    msg.Printf( m_stCircleToPolyWarning->GetLabel(), m_Frame->StringFromValue( ARC_HIGH_DEF, true ) );
    m_stCircleToPolyWarning->SetLabel( msg );

    m_useHeightForLengthCalcs->SetValue( m_BrdSettings->m_UseHeightForLengthCalcs );

    m_maxError.SetValue( m_BrdSettings->m_MaxError );

    m_allowExternalFilletsOpt->SetValue( m_BrdSettings->m_ZoneKeepExternalFillets );
    m_minResolvedSpokeCountCtrl->SetValue( m_BrdSettings->m_MinResolvedSpokes );

    m_minClearance.SetValue( m_BrdSettings->m_MinClearance );
    m_minConn.SetValue( m_BrdSettings->m_MinConn );
    m_trackMinWidth.SetValue( m_BrdSettings->m_TrackMinWidth );
    m_viaMinAnnulus.SetValue( m_BrdSettings->m_ViasMinAnnularWidth );
    m_viaMinSize.SetValue(m_BrdSettings->m_ViasMinSize );
    m_holeClearance.SetValue( m_BrdSettings->m_HoleClearance );
    m_edgeClearance.SetValue( m_BrdSettings->m_CopperEdgeClearance );
    m_minGrooveWidth.SetValue( m_BrdSettings->m_MinGrooveWidth );

    m_throughHoleMin.SetValue( m_BrdSettings->m_MinThroughDrill );
    m_holeToHoleMin.SetValue( m_BrdSettings->m_HoleToHoleMin );

    m_uviaMinSize.SetValue( m_BrdSettings->m_MicroViasMinSize );
    m_uviaMinDrill.SetValue( m_BrdSettings->m_MicroViasMinDrill );

    m_silkClearance.SetValue( m_BrdSettings->m_SilkClearance );
    m_minTextHeight.SetValue( m_BrdSettings->m_MinSilkTextHeight );
    m_minTextThickness.SetValue( m_BrdSettings->m_MinSilkTextThickness );

    return true;
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataFromWindow()
{
    // These are all stored in project file, not board, so no need for OnModify()

    m_BrdSettings->m_UseHeightForLengthCalcs = m_useHeightForLengthCalcs->GetValue();

    m_BrdSettings->m_MaxError = m_maxError.GetValue();

    m_BrdSettings->m_ZoneKeepExternalFillets = m_allowExternalFilletsOpt->GetValue();
    m_BrdSettings->m_MinResolvedSpokes = m_minResolvedSpokeCountCtrl->GetValue();

    m_BrdSettings->m_MinClearance = m_minClearance.GetValue();
    m_BrdSettings->m_MinConn = m_minConn.GetValue();
    m_BrdSettings->m_TrackMinWidth = m_trackMinWidth.GetValue();
    m_BrdSettings->m_ViasMinAnnularWidth = m_viaMinAnnulus.GetValue();
    m_BrdSettings->m_ViasMinSize = m_viaMinSize.GetValue();
    m_BrdSettings->m_HoleClearance = m_holeClearance.GetValue();
    m_BrdSettings->m_CopperEdgeClearance = m_edgeClearance.GetValue();
    m_BrdSettings->m_MinGrooveWidth = m_minGrooveWidth.GetValue();

    m_BrdSettings->m_MinThroughDrill = m_throughHoleMin.GetValue();
    m_BrdSettings->m_HoleToHoleMin = m_holeToHoleMin.GetValue();

    m_BrdSettings->m_MicroViasMinSize = m_uviaMinSize.GetValue();
    m_BrdSettings->m_MicroViasMinDrill = m_uviaMinDrill.GetValue();

    m_BrdSettings->m_SilkClearance = m_silkClearance.GetValue();
    m_BrdSettings->m_MinSilkTextHeight = m_minTextHeight.GetValue();
    m_BrdSettings->m_MinSilkTextThickness = m_minTextThickness.GetValue();

    std::vector<BOARD_DESIGN_SETTINGS::VALIDATION_ERROR> errors =
            m_BrdSettings->ValidateDesignRules( m_Frame->GetUserUnits() );

    if( !errors.empty() )
    {
        const BOARD_DESIGN_SETTINGS::VALIDATION_ERROR& error = errors.front();

        const std::map<wxString, wxWindow*> fieldToControl = {
            { wxS( "min_clearance" ), m_clearanceCtrl },
            { wxS( "min_connection" ), m_MinConnCtrl },
            { wxS( "min_track_width" ), m_TrackMinWidthCtrl },
            { wxS( "min_via_annular_width" ), m_ViaMinAnnulusCtrl },
            { wxS( "min_via_diameter" ), m_SetViasMinSizeCtrl },
            { wxS( "min_through_hole_diameter" ), m_MinDrillCtrl },
            { wxS( "min_microvia_diameter" ), m_uviaMinSizeCtrl },
            { wxS( "min_microvia_drill" ), m_uviaMinDrillCtrl },
            { wxS( "min_hole_to_hole" ), m_SetHoleToHoleCtrl },
            { wxS( "min_hole_clearance" ), m_HoleClearanceCtrl },
            { wxS( "min_silk_clearance" ), m_silkClearanceCtrl },
            { wxS( "min_groove_width" ), m_minGrooveWidthCtrl },
            { wxS( "min_text_height" ), m_textHeightCtrl },
            { wxS( "min_text_thickness" ), m_textThicknessCtrl },
            { wxS( "min_copper_edge_clearance" ), m_EdgeClearanceCtrl },
            { wxS( "max_error" ), m_maxErrorCtrl },
            { wxS( "min_resolved_spokes" ), m_minResolvedSpokeCountCtrl }
        };

        wxWindow* control = nullptr;

        if( auto it = fieldToControl.find( error.setting_name ); it != fieldToControl.end() )
            control = it->second;

        PAGED_DIALOG::GetDialog( this )->SetError( error.error_message, this, control ? control : this );
        return false;
    }

    return true;
}


void PANEL_SETUP_CONSTRAINTS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}
