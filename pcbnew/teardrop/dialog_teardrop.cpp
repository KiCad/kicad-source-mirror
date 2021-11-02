/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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

#include "teardrop.h"
#include "dialog_teardrop_base.h"
#include <widgets/unit_binder.h>


class TEARDROP_DIALOG: public TEARDROP_DIALOG_BASE
{
public:
    TEARDROP_DIALOG( PCB_EDIT_FRAME* aParent ):
        TEARDROP_DIALOG_BASE( aParent ),
        m_frame( aParent ),
        m_teardropMaxLenSetting( aParent,m_stMaxLen, m_tcTdMaxLen, m_stLenUnit ),
        m_teardropMaxHeightSetting( aParent, m_stTdMaxSize, m_tcMaxSize, m_stSizeUnit )
    {
        // Setup actual bitmaps that cannot be set inside wxFormBuilder:
        m_bitmapTdCircularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_sizes ) );
        m_bitmapTdRectangularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_rect_sizes ) );
        m_bitmapTdTrackInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_track_sizes ) );

        m_spTeardropLenPercent->SetValue( m_teardropLenPrm );
        m_spTeardropSizePercent->SetValue( m_teardropSizePrm );
        m_cbSmdSimilarPads->SetValue( m_includeNotPTH );
        m_cbRoundShapesOnly->SetValue( m_roundShapesOnly );
        m_rbShape->SetSelection( m_curveOption );
        m_cbOptUseNextTrack->SetValue( m_canUseTwoTracks );
        m_spPointCount->SetValue( m_curveSegCount );
        m_cbTrack2Track->SetValue( m_track2Track );
        m_cbPadVia->SetValue( m_includeViasAndPTH );

        m_teardropMaxLenSetting.SetValue( m_teardropMaxLenPrm );
        m_teardropMaxHeightSetting.SetValue( m_teardropMaxSizePrm );

        // recalculate sizers, now the bitmap is initialized
        finishDialogSettings();
    }

    ~TEARDROP_DIALOG()
    {
        m_teardropLenPrm = m_spTeardropLenPercent->GetValue();
        m_teardropSizePrm = m_spTeardropSizePercent->GetValue();
        m_teardropMaxLenPrm = m_teardropMaxLenSetting.GetValue();
        m_teardropMaxSizePrm = m_teardropMaxHeightSetting.GetValue();
        m_roundShapesOnly = m_cbRoundShapesOnly->GetValue();
        m_includeNotPTH = m_cbSmdSimilarPads->GetValue();
        m_curveOption = m_rbShape->GetSelection();
        m_canUseTwoTracks = m_cbOptUseNextTrack->GetValue();
        m_curveSegCount = m_spPointCount->GetValue();
        m_track2Track = m_cbTrack2Track->GetValue();
        m_includeViasAndPTH = m_cbPadVia->GetValue();
    }

    CURVED_OPTION CurvedShapeOption()
    {
        return (CURVED_OPTION) m_rbShape->GetSelection();
    }

    // Options for curved shapes
    int GetCurvePointCount() { return m_spPointCount->GetValue(); }
    double GetTeardropLenPercent() { return m_spTeardropLenPercent->GetValue()/100.0; }
    double GetTeardropSizePercent() { return m_spTeardropSizePercent->GetValue()/100.0; }
    int GetTeardropMaxLen() { return m_teardropMaxLenSetting.GetValue(); }
    int GetTeardropMaxHeight() { return m_teardropMaxHeightSetting.GetValue(); }

    // Optins to filter pads
    bool TeardropOnPadVia() { return m_cbPadVia->GetValue(); }
    bool IncludeNotPTH() { return m_cbSmdSimilarPads->GetValue(); }
    bool RoundShapesOnly() { return m_cbRoundShapesOnly->GetValue(); }

    bool CanUseTwoTracks() { return m_cbOptUseNextTrack->GetValue(); }

    bool TeardropOnTracks() { return m_cbTrack2Track->GetValue(); }

private:
    PCB_EDIT_FRAME* m_frame;
    UNIT_BINDER     m_teardropMaxLenSetting;
    UNIT_BINDER     m_teardropMaxHeightSetting;

    // Used to store settings during a session:
    static double   m_teardropLenPrm;
    static double   m_teardropSizePrm;
    static int      m_teardropMaxLenPrm;
    static int      m_teardropMaxSizePrm;
    static bool     m_includeNotPTH;
    static bool     m_roundShapesOnly;
    static int      m_curveOption;
    static bool     m_canUseTwoTracks;
    static int      m_curveSegCount;
    static bool     m_track2Track;
    static bool     m_includeViasAndPTH;

};

// Store settings during a session:
double TEARDROP_DIALOG::m_teardropLenPrm = 50;
double TEARDROP_DIALOG::m_teardropSizePrm = 100;
int  TEARDROP_DIALOG::m_teardropMaxLenPrm = Millimeter2iu( 1.0 );
int  TEARDROP_DIALOG::m_teardropMaxSizePrm = Millimeter2iu( 2.0 );
bool TEARDROP_DIALOG::m_includeNotPTH = true;
bool TEARDROP_DIALOG::m_roundShapesOnly = false;
int  TEARDROP_DIALOG::m_curveOption = 0;
bool TEARDROP_DIALOG::m_canUseTwoTracks = true;
int  TEARDROP_DIALOG::m_curveSegCount = 5;
bool TEARDROP_DIALOG::m_track2Track = true;
bool TEARDROP_DIALOG::m_includeViasAndPTH = true;


void PCB_EDIT_FRAME::OnRunTeardropTool( wxCommandEvent& event )
{
    TEARDROP_DIALOG dlg( this );

    if( dlg.ShowModal() != wxID_OK )
        return;

    wxBusyCursor dummy;

    BOARD_COMMIT committer( this );

    TEARDROP_MANAGER trdm( GetBoard(), this );

    int shape_seg_count;

    if( dlg.CurvedShapeOption() != CURVED_OPTION::OPTION_NONE )
        shape_seg_count = dlg.GetCurvePointCount();
    else
        shape_seg_count = 1;

    trdm.SetTeardropMaxSize( dlg.GetTeardropMaxLen(), dlg.GetTeardropMaxHeight() );
    trdm.SetTeardropSizeRatio( dlg.GetTeardropLenPercent(), dlg.GetTeardropSizePercent() );
    trdm.SetTeardropCurvedPrms( dlg.CurvedShapeOption(), shape_seg_count );

    const bool discardTeardropInSameZone = true;
    trdm.SetTargets( dlg.TeardropOnPadVia(), dlg.RoundShapesOnly(),
                     dlg.IncludeNotPTH(), dlg.TeardropOnTracks() );

    int added_count = trdm.SetTeardrops( &committer,
                                   discardTeardropInSameZone,
                                   dlg.CanUseTwoTracks() );

    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();
    m_infoBar->ShowMessageFor( wxString::Format( _( "%d Teardrops created" ),
                                                 added_count ),
                               1000, wxICON_EXCLAMATION );
}


void PCB_EDIT_FRAME::OnRemoveTeardropTool( wxCommandEvent& event )
{
    BOARD_COMMIT committer( this );
    TEARDROP_MANAGER trdm( GetBoard(), this );

    int count = trdm.RemoveTeardrops( &committer, true );

    m_infoBar->RemoveAllButtons();
    m_infoBar->AddCloseButton();
    m_infoBar->ShowMessageFor( wxString::Format( _( "%d Teardrops removed." ), count ),
                               1000, wxICON_EXCLAMATION );
}

