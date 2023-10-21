/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <pcb_edit_frame.h>
#include <pcb_painter.h>
#include <board.h>
#include <board_design_settings.h>
#include "panel_setup_teardrops.h"


PANEL_SETUP_TEARDROPS::PANEL_SETUP_TEARDROPS( wxWindow* aParentWindow, PCB_EDIT_FRAME* aFrame ) :
        PANEL_SETUP_TEARDROPS_BASE( aParentWindow ),
        m_BrdSettings( &aFrame->GetBoard()->GetDesignSettings() ),
        m_teardropMaxLenRound( aFrame, m_stMaxLen, m_tcTdMaxLen, m_stMaxLenUnits ),
        m_teardropMaxHeightRound( aFrame, m_stTdMaxSize, m_tcMaxHeight, m_stMaxHeightUnits ),
        m_teardropMaxLenRect( aFrame, m_stMaxLen1, m_tcTdMaxLen1, m_stMaxLenUnits1 ),
        m_teardropMaxHeightRect( aFrame, m_stTdMaxSize1, m_tcMaxHeight1, m_stMaxHeightUnits1 ),
        m_teardropMaxLenTrack( aFrame, m_stMaxLen2, m_tcTdMaxLen2, m_stMaxLenUnits2 ),
        m_teardropMaxHeightTrack( aFrame, m_stTdMaxSize2, m_tcMaxHeight2, m_stMaxHeightUnits2 )
{
    m_bitmapTeardrop->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_sizes ) );
    m_bitmapTeardrop1->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_rect_sizes ) );
    m_bitmapTeardrop2->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_track_sizes ) );

    wxFont infoFont = KIUI::GetInfoFont( this ).Italic();
    m_minTrackWidthHint->SetFont( infoFont );
    m_minTrackWidthHint1->SetFont( infoFont );
    m_minTrackWidthHint2->SetFont( infoFont );
}


bool PANEL_SETUP_TEARDROPS::TransferDataToWindow()
{
    TEARDROP_PARAMETERS_LIST* prmsList = m_BrdSettings->GetTeadropParamsList();

    TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
    m_teardropMaxLenRound.SetValue( prms->m_TdMaxLen );
    m_teardropMaxHeightRound.SetValue( prms->m_TdMaxWidth );
    m_spTeardropLenPercent->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spTeardropSizePercent->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbPreferZoneConnection->SetValue( !prms->m_TdOnPadsInZones );
    m_cbTeardropsUseNextTrack->SetValue( prms->m_AllowUseTwoTracks );

    if( prms->IsCurved() )
    {
        m_rbCurved->SetValue( true );
        m_curvePointsCtrl->SetValue( prms->m_CurveSegCount );
    }
    else
    {
        m_rbStraightLines->SetValue( true );
        m_curvePointsCtrl->SetValue( 5 );
    }

    prms = prmsList->GetParameters( TARGET_RECT );
    m_teardropMaxLenRect.SetValue( prms->m_TdMaxLen );
    m_teardropMaxHeightRect.SetValue( prms->m_TdMaxWidth );
    m_spTeardropLenPercent1->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spTeardropSizePercent1->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent1->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbPreferZoneConnection1->SetValue( !prms->m_TdOnPadsInZones );
    m_cbTeardropsUseNextTrack1->SetValue( prms->m_AllowUseTwoTracks );

    if( prms->IsCurved() )
    {
        m_rbCurved1->SetValue( true );
        m_curvePointsCtrl1->SetValue( prms->m_CurveSegCount );
    }
    else
    {
        m_rbStraightLines1->SetValue( true );
        m_curvePointsCtrl1->SetValue( 5 );
    }

    prms = prmsList->GetParameters( TARGET_TRACK );
    m_teardropMaxLenTrack.SetValue( prms->m_TdMaxLen );
    m_teardropMaxHeightTrack.SetValue( prms->m_TdMaxWidth );
    m_spTeardropLenPercent2->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spTeardropSizePercent2->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent2->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbTeardropsUseNextTrack2->SetValue( prms->m_AllowUseTwoTracks );

    if( prms->IsCurved() )
    {
        m_rbCurved2->SetValue( true );
        m_curvePointsCtrl2->SetValue( prms->m_CurveSegCount );
    }
    else
    {
        m_rbStraightLines2->SetValue( true );
        m_curvePointsCtrl2->SetValue( 5 );
    }

    return true;
}


bool PANEL_SETUP_TEARDROPS::TransferDataFromWindow()
{
    TEARDROP_PARAMETERS_LIST* prmsList = m_BrdSettings->GetTeadropParamsList();

    TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
    prms->m_BestLengthRatio = m_spTeardropLenPercent->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spTeardropSizePercent->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenRound.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxHeightRound.GetIntValue();
    prms->m_CurveSegCount = m_rbStraightLines->GetValue() ?  0 : m_curvePointsCtrl->GetValue();
    prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercent->GetValue() / 100.0;
    prms->m_TdOnPadsInZones = !m_cbPreferZoneConnection->GetValue();
    prms->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack->GetValue();

    prms = prmsList->GetParameters( TARGET_RECT );
    prms->m_BestLengthRatio = m_spTeardropLenPercent1->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spTeardropSizePercent1->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenRect.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxHeightRect.GetIntValue();
    prms->m_CurveSegCount = m_rbStraightLines1->GetValue() ?  0 : m_curvePointsCtrl1->GetValue();
    prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercent1->GetValue() / 100.0;
    prms->m_TdOnPadsInZones = !m_cbPreferZoneConnection1->GetValue();
    prms->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack1->GetValue();

    prms = prmsList->GetParameters( TARGET_TRACK );
    prms->m_BestLengthRatio = m_spTeardropLenPercent2->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spTeardropSizePercent2->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenTrack.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxHeightTrack.GetIntValue();
    prms->m_CurveSegCount = m_rbStraightLines2->GetValue() ?  0 : m_curvePointsCtrl2->GetValue();
    prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercent2->GetValue() / 100.0;
    prms->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack2->GetValue();

    return true;
}


void PANEL_SETUP_TEARDROPS::ImportSettingsFrom( BOARD* aBoard )
{
    BOARD_DESIGN_SETTINGS* savedSettings = m_BrdSettings;

    m_BrdSettings = &aBoard->GetDesignSettings();
    TransferDataToWindow();

    m_BrdSettings = savedSettings;
}
