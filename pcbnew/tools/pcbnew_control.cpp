/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include "pcbnew_control.h"
#include "common_actions.h"

#include <pcbnew_id.h>
#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_track.h>
#include <class_draw_panel_gal.h>
#include <class_pcb_screen.h>
#include <pcbcommon.h>
#include <confirm.h>

#include <gal/graphics_abstraction_layer.h>
#include <view/view_controls.h>
#include <pcb_painter.h>

PCBNEW_CONTROL::PCBNEW_CONTROL() :
    TOOL_INTERACTIVE( "pcbnew.Control" )
{
}


void PCBNEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();
}


bool PCBNEW_CONTROL::Init()
{
    setTransitions();

    return true;
}


int PCBNEW_CONTROL::ZoomInOut( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    double zoomScale = 1.0;

    if( aEvent.IsAction( &COMMON_ACTIONS::zoomIn ) )
        zoomScale = 1.3;
    else if( aEvent.IsAction( &COMMON_ACTIONS::zoomOut ) )
        zoomScale = 0.7;

    view->SetScale( view->GetScale() * zoomScale, getViewControls()->GetCursorPosition() );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomInOutCenter( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    double zoomScale = 1.0;

    if( aEvent.IsAction( &COMMON_ACTIONS::zoomInCenter ) )
        zoomScale = 1.3;
    else if( aEvent.IsAction( &COMMON_ACTIONS::zoomOutCenter ) )
        zoomScale = 0.7;

    view->SetScale( view->GetScale() * zoomScale );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomCenter( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    view->SetCenter( getViewControls()->GetCursorPosition() );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomFitScreen( TOOL_EVENT& aEvent )
{
    KIGFX::VIEW* view = m_frame->GetGalCanvas()->GetView();
    KIGFX::GAL* gal = m_frame->GetGalCanvas()->GetGAL();
    BOX2I boardBBox = getModel<BOARD>()->ViewBBox();
    VECTOR2I screenSize = gal->GetScreenPixelSize();

    double iuPerX = screenSize.x ? boardBBox.GetWidth() / screenSize.x : 1.0;
    double iuPerY = screenSize.y ? boardBBox.GetHeight() / screenSize.y : 1.0;

    double bestZoom = std::max( iuPerX, iuPerY );
    double zoomFactor = gal->GetWorldScale() / gal->GetZoomFactor();
    double zoom = 1.0 / ( zoomFactor * bestZoom );

    view->SetScale( zoom );
    view->SetCenter( boardBBox.Centre() );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::TrackDisplayMode( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );

    // Apply new display options to the GAL canvas
    DisplayOpt.DisplayPcbTrackFill = !DisplayOpt.DisplayPcbTrackFill;
    m_frame->m_DisplayPcbTrackFill = DisplayOpt.DisplayPcbTrackFill;
    settings->LoadDisplayOptions( DisplayOpt );

    BOARD* board = getModel<BOARD>();
    for( TRACK* track = board->m_Track; track; track = track->Next() )
        track->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );

    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::PadDisplayMode( TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    m_frame->OnTogglePadDrawMode( dummy );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ViaDisplayMode( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );

    // Apply new display options to the GAL canvas
    DisplayOpt.DisplayViaFill = !DisplayOpt.DisplayViaFill;
    m_frame->m_DisplayViaFill = DisplayOpt.DisplayViaFill;
    settings->LoadDisplayOptions( DisplayOpt );

    BOARD* board = getModel<BOARD>();
    for( TRACK* track = board->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_VIA_T )
            track->ViewUpdate( KIGFX::VIEW_ITEM::GEOMETRY );
    }

    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::HighContrastMode( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );

    DisplayOpt.ContrastModeDisplay = !DisplayOpt.ContrastModeDisplay;
    settings->LoadDisplayOptions( DisplayOpt );
    m_frame->GetGalCanvas()->SetHighContrastLayer( m_frame->GetActiveLayer() );

    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::HighContrastInc( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::HighContrastDec( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


// Layer control
int PCBNEW_CONTROL::LayerTop( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, F_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner1( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In1_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner2( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In2_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner3( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In3_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner4( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In4_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner5( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In5_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner6( TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, In6_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerBottom( TOOL_EVENT& aEvent )
{
    m_frame->SetActiveLayer( B_Cu );
    m_frame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerNext( TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer < F_Cu || layer >= B_Cu )
    {
        setTransitions();
        return 0;
    }

    if( getModel<BOARD>()->GetCopperLayerCount() < 2 ) // Single layer
        layer = B_Cu;
    else if( layer >= getModel<BOARD>()->GetCopperLayerCount() - 2 )
        layer = F_Cu;
    else
        ++layer;

    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );
    editFrame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerPrev( TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer <= F_Cu || layer > B_Cu )
    {
        setTransitions();
        return 0;
    }

    if( getModel<BOARD>()->GetCopperLayerCount() < 2 ) // Single layer
        layer = B_Cu;
    else if( layer == B_Cu )
        layer = std::max( int( F_Cu ), getModel<BOARD>()->GetCopperLayerCount() - 2 );
    else
        --layer;

    assert( IsCopperLayer( layer ) );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );
    editFrame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaInc( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetLayerColor( currentLayer );

    if( currentColor.a <= 0.95 )
    {
        currentColor.a += 0.05;
        settings->SetLayerColor( currentLayer, currentColor );
        m_frame->GetGalCanvas()->GetView()->UpdateLayerColor( currentLayer );
    }

    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaDec( TOOL_EVENT& aEvent )
{
    KIGFX::PCB_PAINTER* painter =
            static_cast<KIGFX::PCB_PAINTER*>( m_frame->GetGalCanvas()->GetView()->GetPainter() );
    KIGFX::PCB_RENDER_SETTINGS* settings =
            static_cast<KIGFX::PCB_RENDER_SETTINGS*> ( painter->GetSettings() );

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetLayerColor( currentLayer );

    if( currentColor.a >= 0.05 )
    {
        currentColor.a -= 0.05;
        settings->SetLayerColor( currentLayer, currentColor );
        m_frame->GetGalCanvas()->GetView()->UpdateLayerColor( currentLayer );
    }

    setTransitions();

    return 0;
}


// Grid control
int PCBNEW_CONTROL::GridFast1( TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid1();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridFast2( TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid2();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridNext( TOOL_EVENT& aEvent )
{
    m_frame->SetNextGrid();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridPrev( TOOL_EVENT& aEvent )
{
    m_frame->SetPrevGrid();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridSetOrigin( TOOL_EVENT& aEvent )
{
    Activate();
    m_frame->SetToolID( ID_PCB_PLACE_GRID_COORD_BUTT, wxCURSOR_PENCIL,
                                               _( "Adjust grid origin" ) );

    KIGFX::VIEW_CONTROLS* controls = getViewControls();
    controls->ShowCursor( true );
    controls->SetSnapping( true );
    controls->SetAutoPan( true );

    while( OPT_TOOL_EVENT evt = Wait() )
    {
        if( evt->IsClick( BUT_LEFT ) )
        {
            getView()->GetGAL()->SetGridOrigin( controls->GetCursorPosition() );
            getView()->MarkDirty();
        }

        else if( evt->IsCancel() || evt->IsActivate() )
            break;
    }

    controls->SetAutoPan( false );
    controls->SetSnapping( false );
    controls->ShowCursor( false );
    setTransitions();
    m_frame->SetToolID( ID_NO_TOOL_SELECTED, wxCURSOR_DEFAULT, wxEmptyString );

    return 0;
}


// Miscellaneous
int PCBNEW_CONTROL::ResetCoords( TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    m_frame->GetScreen()->m_O_Curseur = wxPoint( cursorPos.x, cursorPos.y );
    m_frame->UpdateStatusBar();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::SwitchUnits( TOOL_EVENT& aEvent )
{
    // TODO should not it be refactored to pcb_frame member function?
    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );

    if( g_UserUnit == INCHES )
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_MM );
    else
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_INCH );

    m_frame->ProcessEvent( evt );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ShowHelp( TOOL_EVENT& aEvent )
{
    // TODO
    DisplayInfoMessage( m_frame, _( "Not implemented yet." ) );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ToBeDone( TOOL_EVENT& aEvent )
{
    DisplayInfoMessage( m_frame, _( "Not implemented yet." ) );
    setTransitions();

    return 0;
}


void PCBNEW_CONTROL::setTransitions()
{
    // View controls
    Go( &PCBNEW_CONTROL::ZoomInOut,          COMMON_ACTIONS::zoomIn.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomInOut,          COMMON_ACTIONS::zoomOut.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomInOutCenter,    COMMON_ACTIONS::zoomInCenter.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomInOutCenter,    COMMON_ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomCenter,         COMMON_ACTIONS::zoomCenter.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomFitScreen,      COMMON_ACTIONS::zoomFitScreen.MakeEvent() );

    // Display modes
    Go( &PCBNEW_CONTROL::TrackDisplayMode,   COMMON_ACTIONS::trackDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::PadDisplayMode,     COMMON_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaDisplayMode,     COMMON_ACTIONS::viaDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastMode,   COMMON_ACTIONS::highContrastMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastInc,    COMMON_ACTIONS::highContrastInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastDec,    COMMON_ACTIONS::highContrastDec.MakeEvent() );

    // Layer control
    Go( &PCBNEW_CONTROL::LayerTop,           COMMON_ACTIONS::layerTop.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner1,        COMMON_ACTIONS::layerInner1.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner2,        COMMON_ACTIONS::layerInner2.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner3,        COMMON_ACTIONS::layerInner3.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner4,        COMMON_ACTIONS::layerInner4.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner5,        COMMON_ACTIONS::layerInner5.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerInner6,        COMMON_ACTIONS::layerInner6.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerBottom,        COMMON_ACTIONS::layerBottom.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerNext,          COMMON_ACTIONS::layerNext.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerPrev,          COMMON_ACTIONS::layerPrev.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaInc,      COMMON_ACTIONS::layerAlphaInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaDec ,     COMMON_ACTIONS::layerAlphaDec.MakeEvent() );

    // Grid control
    Go( &PCBNEW_CONTROL::GridFast1,          COMMON_ACTIONS::gridFast1.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridFast2,          COMMON_ACTIONS::gridFast2.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridNext,           COMMON_ACTIONS::gridNext.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridPrev,           COMMON_ACTIONS::gridPrev.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridSetOrigin,      COMMON_ACTIONS::gridSetOrigin.MakeEvent() );

    // Miscellaneous
    Go( &PCBNEW_CONTROL::ResetCoords,        COMMON_ACTIONS::resetCoords.MakeEvent() );
    Go( &PCBNEW_CONTROL::SwitchUnits,        COMMON_ACTIONS::switchUnits.MakeEvent() );
    Go( &PCBNEW_CONTROL::ShowHelp,           COMMON_ACTIONS::showHelp.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToBeDone,           COMMON_ACTIONS::toBeDone.MakeEvent() );
}
