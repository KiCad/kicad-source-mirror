/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2021-2022 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <board_commit.h>
#include <bitmaps.h>
#include <tool/tool_manager.h>
#include <tool/actions.h>
#include "teardrop.h"
#include "dialog_teardrop_base.h"
#include <widgets/unit_binder.h>
#include <widgets/wx_progress_reporters.h>


// Curved shapes options. The actual value is the ORed of options
#define CURVED_OPTION_NONE  0       /* No curved teardrop shape */
#define CURVED_OPTION_ROUND 1       /* Curved teardrop shape for vias and round pad shapes */
#define CURVED_OPTION_RECT  2       /* Curved teardrop shape for rect pad shapes */
#define CURVED_OPTION_TRACK 4       /* Curved teardrop shape for track to track shapes */

bool g_rawTeardrops = false;

class TEARDROP_DIALOG: public TEARDROP_DIALOG_BASE
{
public:
    TEARDROP_DIALOG( PCB_EDIT_FRAME* aParent ):
        TEARDROP_DIALOG_BASE( aParent ),
        m_brdSettings( nullptr ),
        m_frame( aParent ),
        m_teardropMaxLenSettingRound( aParent,m_stMaxLenRound, m_tcTdMaxLenRound, m_stMaxLenRoundUnits ),
        m_teardropMaxHeightSettingRound( aParent, m_stTdMaxSizeRound, m_tcMaxHeightRound, m_stMaxHeightRoundUnits ),
        m_teardropMaxLenSettingRect( aParent,m_stMaxLenRect, m_tcTdMaxLenRect, m_stMaxLenRectUnits ),
        m_teardropMaxHeightSettingRect( aParent, m_stTdMaxSizeRect, m_tcMaxHeightRect, m_stMaxHeightRectUnits ),
        m_teardropMaxLenSettingTrack( aParent,m_stMaxLenTrack, m_tcTdMaxLenTrack, m_stMaxLenTrackUnits ),
        m_teardropMaxHeightSettingTrack( aParent, m_stTdMaxSizeTrack, m_tcMaxHeightTrack, m_stMaxHeightTrackUnits )
    {
        // Setup actual bitmaps that cannot be set inside wxFormBuilder:
        m_bitmapTdCircularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_sizes ) );
        m_bitmapTdRectangularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_rect_sizes ) );
        m_bitmapTdTrackInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_track_sizes ) );

        m_rawTeardropsHint->SetFont( KIUI::GetInfoFont( this ) );

        m_brdSettings = &m_frame->GetBoard()->GetBoard()->GetDesignSettings();
        TEARDROP_PARAMETERS_LIST* prmsList = m_brdSettings->GetTeadropParamsList();

        m_cbPadVia->SetValue( prmsList->m_TargetViasPads );
        m_cbSmdSimilarPads->SetValue( prmsList->m_TargetPadsWithNoHole );
        m_cbRoundShapesOnly->SetValue( prmsList->m_UseRoundShapesOnly );
        m_cbTrack2Track->SetValue( prmsList->m_TargetTrack2Track );
        m_cbPadsInZones->SetValue( prmsList->m_TdOnPadsInZones );

        m_cbOptUseNextTrack->SetValue( prmsList->m_AllowUseTwoTracks );
        m_spPointCount->SetValue( prmsList->m_CurveSegCount );

        TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
        m_teardropMaxLenSettingRound.SetValue( prms->m_TdMaxLen );
        m_teardropMaxHeightSettingRound.SetValue( prms->m_TdMaxHeight );
        m_spTeardropLenPercentRound->SetValue( prms->m_LengthRatio*100 );
        m_spTeardropSizePercentRound->SetValue( prms->m_HeightRatio*100 );
        m_rbShapeRound->SetSelection( prms->IsCurved() );
        m_spTeardropHDPercentRound->SetValue( prms->m_WidthtoSizeFilterRatio*100 );

        prms = prmsList->GetParameters( TARGET_RECT );
        m_teardropMaxLenSettingRect.SetValue( prms->m_TdMaxLen );
        m_teardropMaxHeightSettingRect.SetValue( prms->m_TdMaxHeight );
        m_spTeardropLenPercentRect->SetValue( prms->m_LengthRatio*100 );
        m_spTeardropSizePercentRect->SetValue( prms->m_HeightRatio*100 );
        m_rbShapeRect->SetSelection( prms->IsCurved() );
        m_spTeardropHDPercentRect->SetValue( prms->m_WidthtoSizeFilterRatio*100 );

        prms = prmsList->GetParameters( TARGET_TRACK );
        m_teardropMaxLenSettingTrack.SetValue(prms->m_TdMaxLen );
        m_teardropMaxHeightSettingTrack.SetValue( prms->m_TdMaxHeight );
        m_spTeardropLenPercentTrack->SetValue( prms->m_LengthRatio*100 );
        m_spTeardropSizePercentTrack->SetValue( prms->m_HeightRatio*100 );
        m_rbShapeTrack->SetSelection( prms->IsCurved() );
        m_spTeardropHDPercentTrack->SetValue( prms->m_WidthtoSizeFilterRatio*100 );

        m_generateRawTeardrops->SetValue( g_rawTeardrops );

        // recalculate sizers, now the bitmap is initialized
        finishDialogSettings();
    }

    ~TEARDROP_DIALOG()
    {
        g_rawTeardrops = m_generateRawTeardrops->GetValue();

        TransferToParamList();
    }

    /**
     * Copy the settings from dialog to the current board settings
     */
    void TransferToParamList()
    {
        int shape_seg_count = GetCurvePointCount();
        TEARDROP_PARAMETERS_LIST* prmsList = m_brdSettings->GetTeadropParamsList();

        prmsList->m_TargetViasPads = m_cbPadVia->GetValue();
        prmsList->m_TargetPadsWithNoHole = m_cbSmdSimilarPads->GetValue();
        prmsList->m_UseRoundShapesOnly = m_cbRoundShapesOnly->GetValue();
        prmsList->m_TargetTrack2Track = m_cbTrack2Track->GetValue();
        prmsList->m_TdOnPadsInZones = m_cbPadsInZones->GetValue();

        prmsList->m_AllowUseTwoTracks = m_cbOptUseNextTrack->GetValue();
        prmsList->m_CurveSegCount = m_spPointCount->GetValue();

        TEARDROP_PARAMETERS* prms = prmsList->GetParameters( TARGET_ROUND );
        prms->m_LengthRatio = GetTeardropLenPercentRound();
        prms->m_HeightRatio = GetTeardropSizePercentRound();
        prms->m_TdMaxLen = m_teardropMaxLenSettingRound.GetValue();
        prms->m_TdMaxHeight = m_teardropMaxHeightSettingRound.GetValue();
        prms->m_CurveSegCount = (CurvedShapeOption() & CURVED_OPTION_ROUND) ? shape_seg_count : 0;
        prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercentRound->GetValue() / 100.0;

        prms = prmsList->GetParameters( TARGET_RECT );
        prms->m_LengthRatio = GetTeardropLenPercentRect();
        prms->m_HeightRatio = GetTeardropSizePercentRect();
        prms->m_TdMaxLen = m_teardropMaxLenSettingRect.GetValue();
        prms->m_TdMaxHeight = m_teardropMaxHeightSettingRect.GetValue();
        prms->m_CurveSegCount = (CurvedShapeOption() & CURVED_OPTION_RECT) ? shape_seg_count : 0;
        prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercentRect->GetValue() / 100.0;

        prms = prmsList->GetParameters( TARGET_TRACK );
        prms->m_LengthRatio = GetTeardropLenPercentTrack();
        prms->m_HeightRatio = GetTeardropSizePercentTrack();
        prms->m_TdMaxLen = m_teardropMaxLenSettingTrack.GetValue();
        prms->m_TdMaxHeight = m_teardropMaxHeightSettingTrack.GetValue();
        prms->m_CurveSegCount = (CurvedShapeOption() & CURVED_OPTION_TRACK) ? shape_seg_count : 0;
        prms->m_WidthtoSizeFilterRatio = m_spTeardropHDPercentTrack->GetValue() / 100.0;
    }

    int CurvedShapeOption()
    {
        int opt = 0;

        if( m_rbShapeRound->GetSelection() )
            opt |= CURVED_OPTION_ROUND;

        if( m_rbShapeRect->GetSelection() )
            opt |= CURVED_OPTION_RECT;

        if( m_rbShapeTrack->GetSelection() )
            opt |= CURVED_OPTION_TRACK;

        return opt;
    }

    // Options for curved shapes
    int GetCurvePointCount() { return m_spPointCount->GetValue(); }

    // Getters for size parameters
    double GetTeardropLenPercentRound() { return m_spTeardropLenPercentRound->GetValue()/100.0; }
    double GetTeardropSizePercentRound() { return m_spTeardropSizePercentRound->GetValue()/100.0; }
    int GetTeardropMaxLenRound() { return m_teardropMaxLenSettingRound.GetValue(); }
    int GetTeardropMaxHeightRound() { return m_teardropMaxHeightSettingRound.GetValue(); }

    double GetTeardropLenPercentRect() { return m_spTeardropLenPercentRect->GetValue()/100.0; }
    double GetTeardropSizePercentRect() { return m_spTeardropSizePercentRect->GetValue()/100.0; }
    int GetTeardropMaxLenRect() { return m_teardropMaxLenSettingRect.GetValue(); }
    int GetTeardropMaxHeightRect() { return m_teardropMaxHeightSettingRect.GetValue(); }

    double GetTeardropLenPercentTrack() { return m_spTeardropLenPercentTrack->GetValue()/100.0; }
    double GetTeardropSizePercentTrack() { return m_spTeardropSizePercentTrack->GetValue()/100.0; }
    int GetTeardropMaxLenTrack() { return m_teardropMaxLenSettingTrack.GetValue(); }
    int GetTeardropMaxHeightTrack() { return m_teardropMaxHeightSettingTrack.GetValue(); }

    // Optins to filter pads
    bool TeardropOnPadVia() { return m_cbPadVia->GetValue(); }
    bool IncludeNotPTH() { return m_cbSmdSimilarPads->GetValue(); }
    bool RoundShapesOnly() { return m_cbRoundShapesOnly->GetValue(); }

    bool CanUseTwoTracks() { return m_cbOptUseNextTrack->GetValue(); }

    bool TeardropOnTracks() { return m_cbTrack2Track->GetValue(); }

    bool GenerateRawTeardrops() { return m_generateRawTeardrops->GetValue(); }

private:
    BOARD_DESIGN_SETTINGS* m_brdSettings;
    PCB_EDIT_FRAME* m_frame;
    UNIT_BINDER     m_teardropMaxLenSettingRound;
    UNIT_BINDER     m_teardropMaxHeightSettingRound;
    UNIT_BINDER     m_teardropMaxLenSettingRect;
    UNIT_BINDER     m_teardropMaxHeightSettingRect;
    UNIT_BINDER     m_teardropMaxLenSettingTrack;
    UNIT_BINDER     m_teardropMaxHeightSettingTrack;
};


void PCB_EDIT_FRAME::OnRunTeardropTool( wxCommandEvent& event )
{
    TEARDROP_DIALOG dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxBusyCursor dummy;

    std::unique_ptr<WX_PROGRESS_REPORTER> reporter;
    reporter = std::make_unique<WX_PROGRESS_REPORTER>( this, _( "Filling Zones" ), 4 );

    BOARD_COMMIT committer( this );

    dlg.TransferToParamList();
    TEARDROP_MANAGER trdm( GetBoard(), this, reporter.get() );

    int added_count = trdm.SetTeardrops( &committer, dlg.CanUseTwoTracks(),
                                         !dlg.GenerateRawTeardrops() );

    GetToolManager()->PostEvent( EVENTS::ConnectivityChangedEvent );

    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();
    m_infoBar->ShowMessageFor( wxString::Format( _( "%d teardrops created" ), added_count ),
                               1000, wxICON_EXCLAMATION );
}


void PCB_EDIT_FRAME::OnRemoveTeardropTool( wxCommandEvent& event )
{
    BOARD_COMMIT committer( this );
    TEARDROP_MANAGER trdm( GetBoard(), this );

    int count = trdm.RemoveTeardrops( &committer, true );

    GetCanvas()->RedrawRatsnest();
    GetCanvas()->Refresh();
    GetToolManager()->PostEvent( EVENTS::SelectedItemsModified );

    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();
    m_infoBar->ShowMessageFor( wxString::Format( _( "%d teardrops removed." ), count ),
                               1000, wxICON_EXCLAMATION );
}

