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

#include "dialog_via_stitch_properties.h"

#include <boost/random/mersenne_twister.hpp>
#include <boost/random/uniform_real_distribution.hpp>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <dialogs/dialog_track_via_properties.h>
#include <gal/graphics_abstraction_layer.h>
#include <generators/pcb_via_stitch.h>
#include <geometry/shape_poly_set.h>
#include <origin_viewitem.h>
#include <pcb_base_edit_frame.h>
#include <pcb_painter.h>
#include <pcb_track.h>
#include <pgm_base.h>
#include <project/net_settings.h>
#include <settings/color_settings.h>
#include <tools/generator_tool.h>
#include <widgets/net_selector.h>
#include <widgets/unit_binder.h>
#include <view/view.h>


/// How many pitches to show on a side of the preview tile.  Big enough to give a sense
/// of the pattern across multiple rows; small enough that vias stay readable at any
/// realistic pitch.
static constexpr int PREVIEW_TILE_PITCHES = 6;


DIALOG_VIA_STITCH_PROPERTIES::DIALOG_VIA_STITCH_PROPERTIES( PCB_BASE_EDIT_FRAME* aFrame,
                                                            PCB_VIA_STITCH*      aStitch ) :
        DIALOG_VIA_STITCH_PROPERTIES_BASE( aFrame ),
        m_frame( aFrame ),
        m_stitch( aStitch )
{
    wxCHECK( m_frame && m_stitch, /* void */ );

    SetTitle( _( "Via Stitching Properties" ) );

    m_pitchBinder = std::make_unique<UNIT_BINDER>( m_frame, m_pitchLabel, m_pitchCtrl,
                                                   m_textSizeUnits );
    m_posXBinder  = std::make_unique<UNIT_BINDER>( m_frame, m_posXLabel, m_posXCtrl,
                                                   m_posXUnits );
    m_posYBinder  = std::make_unique<UNIT_BINDER>( m_frame, m_posYLabel, m_posYCtrl,
                                                   m_posYUnits );

    m_netSelector->SetNetInfo( &m_frame->GetBoard()->GetNetInfo() );
    m_netSelector->Bind( FILTERED_ITEM_SELECTED,
                         [this]( wxCommandEvent& aEvt )
                         {
                             updateWorkingCopyFromUI();
                             redrawPreview();
                             aEvt.Skip();
                         } );

    // Work on a detached clone so changes don't touch the live stitch until OK.
    m_workingCopy.reset( static_cast<PCB_VIA_STITCH*>( m_stitch->Clone() ) );
    m_workingCopy->SetParent( m_frame->GetBoard() );

    SetupStandardButtons();
}


DIALOG_VIA_STITCH_PROPERTIES::~DIALOG_VIA_STITCH_PROPERTIES()
{
    if( m_canvasInitialized )
    {
        m_panelShowPreview->StopDrawing();

        KIGFX::VIEW* view = m_panelShowPreview->GetView();

        for( std::unique_ptr<PCB_VIA>& via : m_previewVias )
            view->Remove( via.get() );

        for( std::unique_ptr<PCB_TRACK>& track : m_previewTracks )
            view->Remove( track.get() );

        if( m_axisOrigin )
            view->Remove( m_axisOrigin );
    }

    delete m_axisOrigin;
}


bool DIALOG_VIA_STITCH_PROPERTIES::TransferDataToWindow()
{
    if( !DIALOG_VIA_STITCH_PROPERTIES_BASE::TransferDataToWindow() )
        return false;

    m_netSelector->SetSelectedNetcode( m_stitch->GetNetCode() );

    VECTOR2I pos = m_stitch->GetPosition();
    m_posXBinder->SetValue( pos.x );
    m_posYBinder->SetValue( pos.y );

    m_pitchBinder->SetValue( m_stitch->GetPitch() );

    m_modeCombo->SetSelection( static_cast<int>( m_stitch->GetMode() ) );
    m_patternCombo->SetSelection( static_cast<int>( m_stitch->GetLayout() ) );
    m_seedCtrl->SetValue( wxString::Format( wxT( "%u" ), m_stitch->GetSeed() ) );

    prepareCanvas();
    updateWorkingCopyFromUI();
    redrawPreview();

    // Force the preview to recompute its viewport once the dialog is on screen.
    PostSizeEvent();

    return true;
}


bool DIALOG_VIA_STITCH_PROPERTIES::TransferDataFromWindow()
{
    updateWorkingCopyFromUI();

    int newNetCode = m_netSelector->GetSelectedNetcode();
    if( newNetCode >= 0 )
        m_stitch->SetNetCode( newNetCode );

    m_stitch->SetPitch( m_pitchBinder->GetIntValue() );
    m_stitch->SetMode( static_cast<PCB_VIA_STITCH_MODE>( m_modeCombo->GetSelection() ) );
    m_stitch->SetLayout(
            static_cast<PCB_VIA_STITCH_LAYOUT>( m_patternCombo->GetSelection() ) );

    unsigned long seedVal = 0;
    if( m_seedCtrl->GetValue().ToULong( &seedVal ) )
        m_stitch->SetSeed( static_cast<uint32_t>( seedVal ) );

    // Position: translate the outline to land on the requested first vertex.
    VECTOR2I newPos( m_posXBinder->GetIntValue(), m_posYBinder->GetIntValue() );
    VECTOR2I delta = newPos - m_stitch->GetPosition();

    if( delta != VECTOR2I( 0, 0 ) )
        m_stitch->Move( delta );

    // Propagate any via-template edits the user made via the Configure button.
    m_stitch->SetTemplateItem( wxS( "via" ),
                               std::make_unique<PCB_VIA>( *m_workingCopy->ViaTemplate() ) );

    return true;
}


void DIALOG_VIA_STITCH_PROPERTIES::OnValuesChanged( wxCommandEvent& event )
{
    updateWorkingCopyFromUI();
    redrawPreview();
    event.Skip();
}


void DIALOG_VIA_STITCH_PROPERTIES::OnViaConfigureClicked( wxCommandEvent& event )
{
    DIALOG_TRACK_VIA_PROPERTIES dlg( m_frame, m_workingCopy->ViaTemplate() );

    if( dlg.ShowModal() == wxID_OK )
        redrawPreview();
}


void DIALOG_VIA_STITCH_PROPERTIES::OnCancel( wxCommandEvent& event )
{
    // Stop the GAL from drawing into a window that's about to be torn down.
    if( m_canvasInitialized )
        m_panelShowPreview->StopDrawing();

    event.Skip();
}


void DIALOG_VIA_STITCH_PROPERTIES::OnUpdateUI( wxUpdateUIEvent& event )
{
    PCB_VIA_STITCH_MODE mode = static_cast<PCB_VIA_STITCH_MODE>( m_modeCombo->GetSelection() );
    bool                stitching = ( mode == PCB_VIA_STITCH_MODE::STITCH );

    m_patternLabel->Enable( stitching );
    m_patternCombo->Enable( stitching );

    // The seed control is only used for the Poisson layout.
    PCB_VIA_STITCH_LAYOUT layout =
            static_cast<PCB_VIA_STITCH_LAYOUT>( m_patternCombo->GetSelection() );
    bool poisson = stitching && ( layout == PCB_VIA_STITCH_LAYOUT::POISSON );

    m_seedLabel->Enable( poisson );
    m_seedCtrl->Enable( poisson );
}


void DIALOG_VIA_STITCH_PROPERTIES::updateWorkingCopyFromUI()
{
    if( !m_workingCopy )
        return;

    int newNetCode = m_netSelector->GetSelectedNetcode();
    if( newNetCode >= 0 )
        m_workingCopy->SetNetCode( newNetCode );

    m_workingCopy->SetPitch( m_pitchBinder->GetIntValue() );
    m_workingCopy->SetMode( static_cast<PCB_VIA_STITCH_MODE>( m_modeCombo->GetSelection() ) );
    m_workingCopy->SetLayout(
            static_cast<PCB_VIA_STITCH_LAYOUT>( m_patternCombo->GetSelection() ) );

    unsigned long seedVal = 0;
    if( m_seedCtrl->GetValue().ToULong( &seedVal ) )
        m_workingCopy->SetSeed( static_cast<uint32_t>( seedVal ) );
}


void DIALOG_VIA_STITCH_PROPERTIES::prepareCanvas()
{
    if( m_canvasInitialized )
        return;

    KIGFX::VIEW* view = m_panelShowPreview->GetView();

    m_panelShowPreview->UpdateColors();
    m_panelShowPreview->SetStealsFocus( false );
    m_panelShowPreview->ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );

    auto settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( view->GetPainter()->GetSettings() );
    settings->SetHighContrast( false );
    settings->m_ContrastModeDisplay = HIGH_CONTRAST_MODE::NORMAL;

    view->GetGAL()->SetGridVisibility( false );
    view->GetGAL()->SetAxesEnabled( false );

    // A faint crosshair at the tile origin as a reference point
    COLOR4D axisColor =
            m_frame->GetColorSettings()->GetColor( LAYER_GRID );
    m_axisOrigin = new KIGFX::ORIGIN_VIEWITEM( axisColor, KIGFX::ORIGIN_VIEWITEM::CROSS,
                                               100000, VECTOR2D( 0, 0 ) );
    m_axisOrigin->SetDrawAtZero( true );
    view->Add( m_axisOrigin );

    m_panelShowPreview->StartDrawing();
    m_canvasInitialized = true;

    m_panelShowPreview->Bind( wxEVT_SIZE,
                              [this]( wxSizeEvent& aEvt )
                              {
                                  aEvt.Skip();

                                  CallAfter(
                                          [this]()
                                          {
                                              fitPreview();
                                          } );
                              } );
}


void DIALOG_VIA_STITCH_PROPERTIES::redrawPreview()
{
    if( !m_canvasInitialized || !m_workingCopy )
        return;

    KIGFX::VIEW* view = m_panelShowPreview->GetView();

    // Drop everything old
    for( std::unique_ptr<PCB_VIA>& via : m_previewVias )
        view->Remove( via.get() );

    m_previewVias.clear();

    for( std::unique_ptr<PCB_TRACK>& track : m_previewTracks )
        view->Remove( track.get() );

    m_previewTracks.clear();

    if( m_workingCopy->GetPitch() <= 0 )
    {
        m_previewExtent = 0;
        m_panelShowPreview->Refresh();
        return;
    }

    std::vector<VECTOR2I> samples;
    int                   previewExtent;

    if( m_workingCopy->GetMode() == PCB_VIA_STITCH_MODE::GUARD )
        previewExtent = buildGuardPreview( samples );
    else
        previewExtent = buildStitchPreview( samples );

    for( const VECTOR2I& pt : samples )
    {
        std::unique_ptr<PCB_VIA> via(
                static_cast<PCB_VIA*>( m_workingCopy->ViaTemplate()->Clone() ) );
        via->ResetUuidDirect();
        via->SetParent( m_frame->GetBoard() );
        via->SetPosition( pt );
        via->SetIsFree( true );

        view->Add( via.get() );
        m_previewVias.push_back( std::move( via ) );
    }

    m_previewExtent = previewExtent;
    fitPreview();

    m_panelShowPreview->Refresh();
}


int DIALOG_VIA_STITCH_PROPERTIES::buildStitchPreview( std::vector<VECTOR2I>& aSamples )
{
    const int pitch = m_workingCopy->GetPitch();

    PCB_VIA_STITCH_LAYOUT layout = m_workingCopy->GetLayout();

    if( layout == PCB_VIA_STITCH_LAYOUT::POISSON )
    {
        // The seed only affects the per-seed sub-tile origin shift — so to see its
        // effect the preview has to span more than one tile.  Show a 2x2 grid of
        // tiles with the seed-driven shift applied, matching what buildPlacementCells
        // does for the real placement.
        const std::vector<VECTOR2D>& tile     = PCB_VIA_STITCH::bakedPoissonTile();
        const int                    tileSize = pitch * PCB_VIA_STITCH::POISSON_TILE_PITCHES;
        const int                    numTiles = 2;
        const int                    previewExtent = tileSize * numTiles;

        boost::random::mt19937                           seedRng( m_workingCopy->GetSeed() );
        boost::random::uniform_real_distribution<double> uniform( 0.0, 1.0 );
        const double offsetX = uniform( seedRng ) * tileSize;
        const double offsetY = uniform( seedRng ) * tileSize;

        const int firstTileX = (int) std::floor( ( 0.0 - offsetX ) / (double) tileSize );
        const int firstTileY = (int) std::floor( ( 0.0 - offsetY ) / (double) tileSize );
        const int lastTileX  = (int) std::floor( ( (double) previewExtent - offsetX )
                                                 / (double) tileSize );
        const int lastTileY  = (int) std::floor( ( (double) previewExtent - offsetY )
                                                 / (double) tileSize );

        for( int ty = firstTileY; ty <= lastTileY; ++ty )
        {
            for( int tx = firstTileX; tx <= lastTileX; ++tx )
            {
                for( const VECTOR2D& s : tile )
                {
                    VECTOR2I pt( (int) std::round( offsetX + ( tx + s.x ) * tileSize ),
                                 (int) std::round( offsetY + ( ty + s.y ) * tileSize ) );

                    // Clip to the preview window so we don't draw far-off vias that
                    // would force the autozoom out.
                    if( pt.x < 0 || pt.x > previewExtent
                            || pt.y < 0 || pt.y > previewExtent )
                        continue;

                    aSamples.push_back( pt );
                }
            }
        }

        return previewExtent;
    }

    for( int row = 0; row < PREVIEW_TILE_PITCHES; ++row )
    {
        int xShift = ( layout == PCB_VIA_STITCH_LAYOUT::STAGGERED && ( row % 2 ) ) ? pitch / 2
                                                                                   : 0;

        for( int col = 0; col < PREVIEW_TILE_PITCHES; ++col )
            aSamples.emplace_back( col * pitch + xShift, row * pitch );
    }

    return pitch * PREVIEW_TILE_PITCHES;
}


int DIALOG_VIA_STITCH_PROPERTIES::buildGuardPreview( std::vector<VECTOR2I>& aSamples )
{
    const int pitch  = m_workingCopy->GetPitch();
    const int extent = pitch * PREVIEW_TILE_PITCHES;

    int viaSize = m_workingCopy->ViaTemplate()->GetWidth( PADSTACK::ALL_LAYERS );

    if( viaSize <= 0 )
        viaSize = pcbIUScale.mmToIU( 0.6 );

    BOARD*                 board = m_frame->GetBoard();
    BOARD_DESIGN_SETTINGS& bds   = board->GetDesignSettings();

    const int trackWidth = std::max( bds.GetCurrentTrackWidth(), pcbIUScale.mmToIU( 0.05 ) );
    const int clearance  = bds.m_NetSettings->GetDefaultNetclass()->GetClearance();

    const std::vector<VECTOR2I> corners = {
        { extent * 12 / 100, extent * 78 / 100 },
        { extent * 38 / 100, extent * 78 / 100 },
        { extent * 62 / 100, extent * 22 / 100 },
        { extent * 88 / 100, extent * 22 / 100 },
    };

    const int polyApproxError = bds.m_MaxError;
    const int safetyMargin    = polyApproxError + pcbIUScale.mmToIU( 0.002 );
    const int margin          = viaSize / 2 + clearance + safetyMargin;

    KIGFX::VIEW*   view = m_panelShowPreview->GetView();
    SHAPE_POLY_SET envelope;
    SHAPE_POLY_SET guarded;

    for( size_t ii = 1; ii < corners.size(); ++ii )
    {
        std::unique_ptr<PCB_TRACK> track = std::make_unique<PCB_TRACK>( board );
        track->SetLayer( F_Cu );
        track->SetWidth( trackWidth );
        track->SetStart( corners[ii - 1] );
        track->SetEnd( corners[ii] );

        track->TransformShapeToPolygon( envelope, F_Cu, margin, polyApproxError, ERROR_OUTSIDE );
        track->TransformShapeToPolygon( guarded, F_Cu, 0, polyApproxError, ERROR_OUTSIDE );

        view->Add( track.get() );
        m_previewTracks.push_back( std::move( track ) );
    }

    envelope.Simplify();
    guarded.Simplify();

    aSamples = PCB_VIA_STITCH::SampleGuardEnvelope( envelope, guarded, pitch,
                                                    []( const VECTOR2I& )
                                                    {
                                                        return true;
                                                    } );

    return extent;
}


void DIALOG_VIA_STITCH_PROPERTIES::fitPreview()
{
    if( !m_canvasInitialized || !m_workingCopy || m_previewExtent <= 0 )
        return;

    KIGFX::VIEW* view = m_panelShowPreview->GetView();

    // Make sure we don't try fit too early
    if( view->GetGAL()->GetScreenPixelSize().x <= 0
            || view->GetGAL()->GetScreenPixelSize().y <= 0 )
    {
        return;
    }

    // Frame the preview window with a bit of margin.
    int   margin = m_workingCopy->GetPitch() > 0 ? m_workingCopy->GetPitch() : 1;
    BOX2D viewBox( VECTOR2D( 0, 0 ), VECTOR2D( m_previewExtent, m_previewExtent ) );

    viewBox.Inflate( margin );
    view->SetViewport( viewBox );

    m_panelShowPreview->Refresh();
}
