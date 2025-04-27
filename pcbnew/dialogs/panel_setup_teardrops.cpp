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
        m_teardropMaxWidthRound( aFrame, m_stMaxWidthLabel, m_tcMaxWidth, m_stMaxWidthUnits ),
        m_teardropMaxLenRect( aFrame, m_stMaxLen1, m_tcTdMaxLen1, m_stMaxLen1Units ),
        m_teardropMaxWidthRect( aFrame, m_stMaxWidth2Label, m_tcMaxWidth1, m_stMaxWidth1Units ),
        m_teardropMaxLenT2T( aFrame, m_stMaxLen2, m_tcTdMaxLen2, m_stMaxLen2Units ),
        m_teardropMaxWidthT2T( aFrame, m_stMaxWidth2Label, m_tcMaxWidth2, m_stMaxWidth2Units )
{
    m_bitmapTeardrop->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_sizes ) );
    m_bitmapTeardrop1->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_rect_sizes ) );
    m_bitmapTeardrop2->SetBitmap( KiBitmapBundle( BITMAPS::teardrop_track_sizes ) );
}


bool PANEL_SETUP_TEARDROPS::TransferDataToWindow()
{
    TEARDROP_PARAMETERS_LIST* prmsList = m_BrdSettings->GetTeadropParamsList();

    TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
    m_teardropMaxLenRound.SetValue( prms->m_TdMaxLen );
    m_teardropMaxWidthRound.SetValue( prms->m_TdMaxWidth );
    m_spLenPercent->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spWidthPercent->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbPreferZoneConnection->SetValue( !prms->m_TdOnPadsInZones );
    m_cbTeardropsUseNextTrack->SetValue( prms->m_AllowUseTwoTracks );
    m_cbCurvedEdges->SetValue( prms->m_CurvedEdges );

    prms = prmsList->GetParameters( TARGET_RECT );
    m_teardropMaxLenRect.SetValue( prms->m_TdMaxLen );
    m_teardropMaxWidthRect.SetValue( prms->m_TdMaxWidth );
    m_spLenPercent1->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spWidthPercent1->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent1->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbPreferZoneConnection1->SetValue( !prms->m_TdOnPadsInZones );
    m_cbTeardropsUseNextTrack1->SetValue( prms->m_AllowUseTwoTracks );
    m_cbCurvedEdges1->SetValue( prms->m_CurvedEdges );

    prms = prmsList->GetParameters( TARGET_TRACK );
    m_teardropMaxLenT2T.SetValue( prms->m_TdMaxLen );
    m_teardropMaxWidthT2T.SetValue( prms->m_TdMaxWidth );
    m_spLenPercent2->SetValue( prms->m_BestLengthRatio *100.0 );
    m_spWidthPercent2->SetValue( prms->m_BestWidthRatio *100.0 );
    m_spTeardropHDPercent2->SetValue( prms->m_WidthtoSizeFilterRatio*100.0 );
    m_cbTeardropsUseNextTrack2->SetValue( prms->m_AllowUseTwoTracks );
    m_cbCurvedEdges2->SetValue( prms->m_CurvedEdges );

    return true;
}


bool PANEL_SETUP_TEARDROPS::TransferDataFromWindow()
{
    TEARDROP_PARAMETERS_LIST* prmsList = m_BrdSettings->GetTeadropParamsList();

    TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
    prms->m_BestLengthRatio = m_spLenPercent->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spWidthPercent->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenRound.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxWidthRound.GetIntValue();
    prms->m_CurvedEdges = m_cbCurvedEdges->GetValue();
    prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercent->GetValue() / 100.0;
    prms->m_TdOnPadsInZones = !m_cbPreferZoneConnection->GetValue();
    prms->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack->GetValue();

    prms = prmsList->GetParameters( TARGET_RECT );
    prms->m_BestLengthRatio = m_spLenPercent1->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spWidthPercent1->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenRect.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxWidthRect.GetIntValue();
    prms->m_CurvedEdges = m_cbCurvedEdges1->GetValue();
    prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercent1->GetValue() / 100.0;
    prms->m_TdOnPadsInZones = !m_cbPreferZoneConnection1->GetValue();
    prms->m_AllowUseTwoTracks = m_cbTeardropsUseNextTrack1->GetValue();

    prms = prmsList->GetParameters( TARGET_TRACK );
    prms->m_BestLengthRatio = m_spLenPercent2->GetValue() / 100.0;
    prms->m_BestWidthRatio = m_spWidthPercent2->GetValue() / 100.0;
    prms->m_TdMaxLen = m_teardropMaxLenT2T.GetIntValue();
    prms->m_TdMaxWidth = m_teardropMaxWidthT2T.GetIntValue();
    prms->m_CurvedEdges = m_cbCurvedEdges2->GetValue();
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
