/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <board_design_settings.h>
#include <board.h>
#include <convert_to_biu.h>
#include <math/util.h>
#include <panel_setup_constraints.h>
#include <panel_setup_constraints_base.h>
#include <pcb_edit_frame.h>
#include <widgets/paged_dialog.h>
#include <wx/treebook.h>
#include <bitmaps.h>


PANEL_SETUP_CONSTRAINTS::PANEL_SETUP_CONSTRAINTS( PAGED_DIALOG* aParent, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_CONSTRAINTS_BASE( aParent->GetTreebook() ),
        m_minClearance( aFrame, m_clearanceTitle, m_clearanceCtrl, m_clearanceUnits, true ),
        m_trackMinWidth( aFrame, m_TrackMinWidthTitle, m_TrackMinWidthCtrl, m_TrackMinWidthUnits, true ),
        m_viaMinAnnulus( aFrame, m_ViaMinAnnulusTitle, m_ViaMinAnnulusCtrl, m_ViaMinAnnulusUnits, true ),
        m_viaMinSize( aFrame, m_ViaMinTitle, m_SetViasMinSizeCtrl, m_ViaMinUnits, true ),
        m_throughHoleMin( aFrame, m_MinDrillTitle, m_MinDrillCtrl, m_MinDrillUnits, true ),
        m_uviaMinSize( aFrame, m_uviaMinSizeLabel, m_uviaMinSizeCtrl, m_uviaMinSizeUnits, true ),
        m_uviaMinDrill( aFrame, m_uviaMinDrillLabel, m_uviaMinDrillCtrl, m_uviaMinDrillUnits, true ),
        m_holeToHoleMin( aFrame, m_HoleToHoleTitle, m_SetHoleToHoleCtrl, m_HoleToHoleUnits, true ),
        m_holeClearance( aFrame, m_HoleClearanceLabel, m_HoleClearanceCtrl, m_HoleClearanceUnits, true ),
        m_edgeClearance( aFrame, m_EdgeClearanceLabel, m_EdgeClearanceCtrl, m_EdgeClearanceUnits, true ),
        m_silkClearance( aFrame, m_silkClearanceLabel, m_silkClearanceCtrl, m_silkClearanceUnits, true ),
        m_maxError( aFrame, m_maxErrorTitle, m_maxErrorCtrl, m_maxErrorUnits, true )
{
    m_Frame = aFrame;
    m_BrdSettings = &m_Frame->GetBoard()->GetDesignSettings();

    wxFont infoFont = wxSystemSettings::GetFont( wxSYS_DEFAULT_GUI_FONT );
    infoFont.SetSymbolicSize( wxFONTSIZE_SMALL );
    m_stCircleToPolyWarning->SetFont( infoFont );
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataToWindow()
{
    wxString msg;
    msg.Printf( m_stCircleToPolyWarning->GetLabel(),
                StringFromValue( m_Frame->GetUserUnits(), ARC_HIGH_DEF, true ) );
    m_stCircleToPolyWarning->SetLabel( msg );

    m_OptAllowBlindBuriedVias->SetValue( m_BrdSettings->m_BlindBuriedViaAllowed );
    m_OptAllowMicroVias->SetValue( m_BrdSettings->m_MicroViasAllowed );

    m_maxError.SetValue( m_BrdSettings->m_MaxError );

    m_rbOutlinePolygonFastest->SetValue( m_BrdSettings->m_ZoneFillVersion == 6 );
    m_rbOutlinePolygonBestQ->SetValue( m_BrdSettings->m_ZoneFillVersion == 5 );
    m_allowExternalFilletsOpt->SetValue( m_BrdSettings->m_ZoneKeepExternalFillets );

    m_minClearance.SetValue( m_BrdSettings->m_MinClearance );
    m_trackMinWidth.SetValue( m_BrdSettings->m_TrackMinWidth );
    m_viaMinAnnulus.SetValue( m_BrdSettings->m_ViasMinAnnulus );
    m_viaMinSize.SetValue(m_BrdSettings->m_ViasMinSize );
    m_holeClearance.SetValue( m_BrdSettings->m_HoleClearance );
    m_edgeClearance.SetValue( m_BrdSettings->m_CopperEdgeClearance );

    m_throughHoleMin.SetValue( m_BrdSettings->m_MinThroughDrill );
    m_holeToHoleMin.SetValue( m_BrdSettings->m_HoleToHoleMin );

    m_uviaMinSize.SetValue( m_BrdSettings->m_MicroViasMinSize );
    m_uviaMinDrill.SetValue( m_BrdSettings->m_MicroViasMinDrill );

    m_silkClearance.SetValue( m_BrdSettings->m_SilkClearance );

    return true;
}


bool PANEL_SETUP_CONSTRAINTS::TransferDataFromWindow()
{
    if( !m_minClearance.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_trackMinWidth.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_viaMinAnnulus.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_viaMinSize.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_holeClearance.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_edgeClearance.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    if( !m_throughHoleMin.Validate( 2, 1000, EDA_UNITS::MILS ) )   // #107 to 1 inch
        return false;

    if( !m_holeToHoleMin.Validate( 0, 10, EDA_UNITS::INCHES ) )
        return false;

    // These are all stored in project file, not board, so no need for OnModify()

    m_BrdSettings->m_BlindBuriedViaAllowed = m_OptAllowBlindBuriedVias->GetValue();
    m_BrdSettings->m_MicroViasAllowed = m_OptAllowMicroVias->GetValue();

    m_BrdSettings->m_MaxError = Clamp<int>( IU_PER_MM * MINIMUM_ERROR_SIZE_MM,
                                            m_maxError.GetValue(),
                                            IU_PER_MM * MAXIMUM_ERROR_SIZE_MM );

    m_BrdSettings->m_ZoneFillVersion = m_rbOutlinePolygonFastest->GetValue() ? 6 : 5;
    m_BrdSettings->m_ZoneKeepExternalFillets = m_allowExternalFilletsOpt->GetValue();

    m_BrdSettings->m_MinClearance = m_minClearance.GetValue();
    m_BrdSettings->m_TrackMinWidth = m_trackMinWidth.GetValue();
    m_BrdSettings->m_ViasMinAnnulus = m_viaMinAnnulus.GetValue();
    m_BrdSettings->m_ViasMinSize = m_viaMinSize.GetValue();
    m_BrdSettings->m_HoleClearance = m_holeClearance.GetValue();
    m_BrdSettings->SetCopperEdgeClearance( m_edgeClearance.GetValue() );

    m_BrdSettings->m_MinThroughDrill = m_throughHoleMin.GetValue();
    m_BrdSettings->SetMinHoleSeparation( m_holeToHoleMin.GetValue() );

    m_BrdSettings->m_MicroViasMinSize = m_uviaMinSize.GetValue();
    m_BrdSettings->m_MicroViasMinDrill = m_uviaMinDrill.GetValue();

    m_BrdSettings->m_SilkClearance = m_silkClearance.GetValue();

    return true;
}


bool PANEL_SETUP_CONSTRAINTS::Show( bool aShow )
{
    bool retVal = wxPanel::Show( aShow );

    if( aShow )
    {
        // These *should* work in the constructor, and indeed they do if this panel is the
        // first displayed.  However, on OSX 3.0.5 (at least), if another panel is displayed
        // first then the icons will be blank unless they're set here.
        m_bitmapZoneFillOpt->SetBitmap( KiBitmap( BITMAPS::show_zone ) );
        m_filletBitmap->SetBitmap( KiBitmap( BITMAPS::zone_fillet ) );
        m_bitmapClearance->SetBitmap( KiBitmap( BITMAPS::ps_diff_pair_gap ) );
        m_bitmapMinTrackWidth->SetBitmap( KiBitmap( BITMAPS::width_track ) );
        m_bitmapMinViaAnnulus->SetBitmap( KiBitmap( BITMAPS::via_annulus ) );
        m_bitmapMinViaDiameter->SetBitmap( KiBitmap( BITMAPS::via_diameter ) );
        m_bitmapMinViaDrill->SetBitmap( KiBitmap( BITMAPS::via_hole_diameter ) );
        m_bitmapMinuViaDiameter->SetBitmap( KiBitmap( BITMAPS::via_diameter ) );
        m_bitmapMinuViaDrill->SetBitmap( KiBitmap( BITMAPS::via_hole_diameter ) );
        m_bitmapHoleClearance->SetBitmap( KiBitmap( BITMAPS::hole_to_copper_clearance ) );
        m_bitmapMinHoleClearance->SetBitmap( KiBitmap( BITMAPS::hole_to_hole_clearance ) );
        m_bitmapEdgeClearance->SetBitmap( KiBitmap( BITMAPS::edge_to_copper_clearance ) );
        m_bitmapBlindBuried->SetBitmap( KiBitmap( BITMAPS::via_buried ) );
        m_bitmap_uVia->SetBitmap( KiBitmap( BITMAPS::via_microvia ) );

        Layout();
    }

    return retVal;
}


void PANEL_SETUP_CONSTRAINTS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}


void PANEL_SETUP_CONSTRAINTS::onChangeOutlineOpt( wxCommandEvent& event )
{
    wxObject* item =event.GetEventObject();

    if( item == m_rbOutlinePolygonBestQ )
        m_rbOutlinePolygonFastest->SetValue( not m_rbOutlinePolygonBestQ->GetValue() );
    else
        m_rbOutlinePolygonBestQ->SetValue( not m_rbOutlinePolygonFastest->GetValue() );
}
