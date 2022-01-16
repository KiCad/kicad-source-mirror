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


// Curved shapes options. The actual value is the ORed of options
#define CURVED_OPTION_NONE  0       /* No curved teardrop shape */
#define CURVED_OPTION_ROUND 1       /* Curved teardrop shape for vias and round pad shapes */
#define CURVED_OPTION_RECT  2       /* Curved teardrop shape for rect pad shapes */
#define CURVED_OPTION_TRACK 4       /* Curved teardrop shape for track to track shapes */

class TEARDROP_DIALOG: public TEARDROP_DIALOG_BASE
{
public:
    TEARDROP_DIALOG( PCB_EDIT_FRAME* aParent ):
        TEARDROP_DIALOG_BASE( aParent ),
        m_frame( aParent ),
        m_teardropMaxLenSettingRound( aParent,m_stMaxLenRound, m_tcTdMaxLenRound, nullptr ),
        m_teardropMaxHeightSettingRound( aParent, m_stTdMaxSizeRound, m_tcMaxHeightRound, m_stLenUnitRound ),
        m_teardropMaxLenSettingRect( aParent,m_stMaxLenRect, m_tcTdMaxLenRect, nullptr ),
        m_teardropMaxHeightSettingRect( aParent, m_stTdMaxSizeRect, m_tcMaxHeightRect, m_stLenUnitRect ),
        m_teardropMaxLenSettingTrack( aParent,m_stMaxLenTrack, m_tcTdMaxLenTrack, nullptr ),
        m_teardropMaxHeightSettingTrack( aParent, m_stTdMaxSizeTrack, m_tcMaxHeightTrack, m_stLenUnitTrack )
    {
        // Setup actual bitmaps that cannot be set inside wxFormBuilder:
        m_bitmapTdCircularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_sizes ) );
        m_bitmapTdRectangularInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_rect_sizes ) );
        m_bitmapTdTrackInfo->SetBitmap( KiBitmap( BITMAPS::teardrop_track_sizes ) );

        m_cbSmdSimilarPads->SetValue( m_includeNotPTH );
        m_cbRoundShapesOnly->SetValue( m_roundShapesOnly );
        m_rbShapeRound->SetSelection( m_curveOptionRoundShapes );
        m_rbShapeRect->SetSelection( m_curveOptionRectShapes );
        m_rbShapeTrack->SetSelection( m_curveOptionTrackShapes );
        m_cbOptUseNextTrack->SetValue( m_canUseTwoTracks );
        m_spPointCount->SetValue( m_curveSegCount );
        m_cbTrack2Track->SetValue( m_track2Track );
        m_cbPadVia->SetValue( m_includeViasAndPTH );

        m_teardropMaxLenSettingRound.SetValue( m_teardropMaxLenPrmRound );
        m_teardropMaxHeightSettingRound.SetValue( m_teardropMaxSizePrmRound );
        m_spTeardropLenPercentRound->SetValue( m_teardropLenPrmRound );
        m_spTeardropSizePercentRound->SetValue( m_teardropSizePrmRound );

        m_teardropMaxLenSettingRect.SetValue( m_teardropMaxLenPrmRect );
        m_teardropMaxHeightSettingRect.SetValue( m_teardropMaxSizePrmRect );
        m_spTeardropLenPercentRect->SetValue( m_teardropLenPrmRect );
        m_spTeardropSizePercentRect->SetValue( m_teardropSizePrmRect );

        m_teardropMaxLenSettingTrack.SetValue( m_teardropMaxLenPrmTrack );
        m_teardropMaxHeightSettingTrack.SetValue( m_teardropMaxSizePrmTrack );
        m_spTeardropLenPercentTrack->SetValue( m_teardropLenPrmTrack );
        m_spTeardropSizePercentTrack->SetValue( m_teardropSizePrmTrack );

        // recalculate sizers, now the bitmap is initialized
        finishDialogSettings();
    }

    ~TEARDROP_DIALOG()
    {
        m_teardropLenPrmRound = m_spTeardropLenPercentRound->GetValue();
        m_teardropSizePrmRound = m_spTeardropSizePercentRound->GetValue();
        m_teardropMaxLenPrmRound = m_teardropMaxLenSettingRound.GetValue();
        m_teardropMaxSizePrmRound = m_teardropMaxHeightSettingRound.GetValue();

        m_teardropLenPrmRect = m_spTeardropLenPercentRect->GetValue();
        m_teardropSizePrmRect = m_spTeardropSizePercentRect->GetValue();
        m_teardropMaxLenPrmRect = m_teardropMaxLenSettingRect.GetValue();
        m_teardropMaxSizePrmRect = m_teardropMaxHeightSettingRect.GetValue();

        m_teardropLenPrmTrack = m_spTeardropLenPercentTrack->GetValue();
        m_teardropSizePrmTrack = m_spTeardropSizePercentTrack->GetValue();
        m_teardropMaxLenPrmTrack = m_teardropMaxLenSettingTrack.GetValue();
        m_teardropMaxSizePrmTrack = m_teardropMaxHeightSettingTrack.GetValue();

        m_roundShapesOnly = m_cbRoundShapesOnly->GetValue();
        m_includeNotPTH = m_cbSmdSimilarPads->GetValue();
        m_curveOptionRoundShapes = m_rbShapeRound->GetSelection();
        m_curveOptionRectShapes = m_rbShapeRect->GetSelection();
        m_curveOptionTrackShapes = m_rbShapeTrack->GetSelection();
        m_canUseTwoTracks = m_cbOptUseNextTrack->GetValue();
        m_curveSegCount = m_spPointCount->GetValue();
        m_track2Track = m_cbTrack2Track->GetValue();
        m_includeViasAndPTH = m_cbPadVia->GetValue();
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

private:
    PCB_EDIT_FRAME* m_frame;
    UNIT_BINDER     m_teardropMaxLenSettingRound;
    UNIT_BINDER     m_teardropMaxHeightSettingRound;
    UNIT_BINDER     m_teardropMaxLenSettingRect;
    UNIT_BINDER     m_teardropMaxHeightSettingRect;
    UNIT_BINDER     m_teardropMaxLenSettingTrack;
    UNIT_BINDER     m_teardropMaxHeightSettingTrack;

    // Used to store settings during a session:
    static double   m_teardropLenPrmRound;
    static double   m_teardropSizePrmRound;
    static int      m_teardropMaxLenPrmRound;
    static int      m_teardropMaxSizePrmRound;

    static double   m_teardropLenPrmRect;
    static double   m_teardropSizePrmRect;
    static int      m_teardropMaxLenPrmRect;
    static int      m_teardropMaxSizePrmRect;

    static double   m_teardropLenPrmTrack;
    static double   m_teardropSizePrmTrack;
    static int      m_teardropMaxLenPrmTrack;
    static int      m_teardropMaxSizePrmTrack;

    static bool     m_includeNotPTH;
    static bool     m_roundShapesOnly;
    static int      m_curveOptionRoundShapes;
    static int      m_curveOptionRectShapes;
    static int      m_curveOptionTrackShapes;
    static bool     m_canUseTwoTracks;
    static int      m_curveSegCount;
    static bool     m_track2Track;
    static bool     m_includeViasAndPTH;

};

// Store settings during a session:
double TEARDROP_DIALOG::m_teardropLenPrmRound = 50;
double TEARDROP_DIALOG::m_teardropSizePrmRound = 100;
int  TEARDROP_DIALOG::m_teardropMaxLenPrmRound = Millimeter2iu( 1.0 );
int  TEARDROP_DIALOG::m_teardropMaxSizePrmRound = Millimeter2iu( 2.0 );

double TEARDROP_DIALOG::m_teardropLenPrmRect = 50;
double TEARDROP_DIALOG::m_teardropSizePrmRect = 100;
int  TEARDROP_DIALOG::m_teardropMaxLenPrmRect = Millimeter2iu( 1.0 );
int  TEARDROP_DIALOG::m_teardropMaxSizePrmRect = Millimeter2iu( 2.0 );

double TEARDROP_DIALOG::m_teardropLenPrmTrack = 100;
double TEARDROP_DIALOG::m_teardropSizePrmTrack = 100;
int  TEARDROP_DIALOG::m_teardropMaxLenPrmTrack = Millimeter2iu( 2.0 );
int  TEARDROP_DIALOG::m_teardropMaxSizePrmTrack = Millimeter2iu( 2.0 );

bool TEARDROP_DIALOG::m_includeNotPTH = true;
bool TEARDROP_DIALOG::m_roundShapesOnly = false;
int  TEARDROP_DIALOG::m_curveOptionRoundShapes = 0;
int  TEARDROP_DIALOG::m_curveOptionRectShapes = 0;
int  TEARDROP_DIALOG::m_curveOptionTrackShapes = 0;
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

    int shape_seg_count = dlg.GetCurvePointCount();

    trdm.SetTeardropMaxSize( TARGET_ROUND, dlg.GetTeardropMaxLenRound(),
                             dlg.GetTeardropMaxHeightRound() );
    trdm.SetTeardropSizeRatio( TARGET_ROUND, dlg.GetTeardropLenPercentRound(),
                               dlg.GetTeardropSizePercentRound() );
    trdm.SetTeardropCurvedPrm( TARGET_ROUND, (dlg.CurvedShapeOption() & CURVED_OPTION_ROUND)
                                             ? shape_seg_count : 0 );

    trdm.SetTeardropMaxSize( TARGET_RECT, dlg.GetTeardropMaxLenRect(),
                             dlg.GetTeardropMaxHeightRect() );
    trdm.SetTeardropSizeRatio( TARGET_RECT, dlg.GetTeardropLenPercentRect(),
                               dlg.GetTeardropSizePercentRect() );
    trdm.SetTeardropCurvedPrm( TARGET_RECT, (dlg.CurvedShapeOption() & CURVED_OPTION_RECT)
                                             ? shape_seg_count : 0 );

    trdm.SetTeardropMaxSize( TARGET_TRACK, dlg.GetTeardropMaxLenTrack(),
                             dlg.GetTeardropMaxHeightTrack() );
    trdm.SetTeardropSizeRatio( TARGET_TRACK, dlg.GetTeardropLenPercentTrack(),
                             dlg.GetTeardropSizePercentTrack() );
    trdm.SetTeardropCurvedPrm( TARGET_TRACK, (dlg.CurvedShapeOption() & CURVED_OPTION_TRACK)
                                              ? shape_seg_count : 0 );

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

