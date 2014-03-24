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
#include <class_drawpanel_gal.h>
#include <class_pcb_screen.h>
#include <gal/graphics_abstraction_layer.h>

using namespace KIGFX;
using boost::optional;

PCBNEW_CONTROL::PCBNEW_CONTROL() :
    TOOL_INTERACTIVE( "pcbnew.Settings" )
{
}


bool PCBNEW_CONTROL::Init()
{
    setTransitions();

    return true;
}


int PCBNEW_CONTROL::ZoomIn( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomOut( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomCenter( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ZoomFitScreen( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::TrackDisplayMode( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::PadDisplayMode( TOOL_EVENT& aEvent )
{
    wxCommandEvent dummy;
    getEditFrame<PCB_EDIT_FRAME>()->OnTogglePadDrawMode( dummy );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ViaDisplayMode( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::HighContrastMode( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
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
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_FRONT );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner1( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_2 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner2( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_3 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner3( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_4 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner4( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_5 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner5( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_6 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerInner6( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SwitchLayer( NULL, LAYER_N_7 );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerBottom( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SetActiveLayer( LAYER_N_BACK, true );
    getEditFrame<PCB_EDIT_FRAME>()->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerNext( TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    LAYER_NUM layer = editFrame->GetActiveLayer();
    layer = ( layer + 1 ) % ( LAST_COPPER_LAYER + 1 );
    assert( IsCopperLayer( layer ) );

    editFrame->SwitchLayer( NULL, layer );
    editFrame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerPrev( TOOL_EVENT& aEvent )
{
    PCB_EDIT_FRAME* editFrame = getEditFrame<PCB_EDIT_FRAME>();
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( --layer < 0 )
        layer = LAST_COPPER_LAYER;

    assert( IsCopperLayer( layer ) );
    editFrame->SwitchLayer( NULL, layer );
    editFrame->GetGalCanvas()->SetFocus();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaInc( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaDec( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


// Grid control
int PCBNEW_CONTROL::GridFast1( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SetFastGrid1();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridFast2( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SetFastGrid2();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridNext( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SetNextGrid();
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::GridPrev( TOOL_EVENT& aEvent )
{
    getEditFrame<PCB_EDIT_FRAME>()->SetPrevGrid();
    setTransitions();

    return 0;
}


// Track & via size control
int PCBNEW_CONTROL::TrackWidthInc( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>( PCB_T );
    int widthIndex = board->GetTrackWidthIndex() + 1;

    if( widthIndex >= (int) board->m_TrackWidthList.size() )
        widthIndex = board->m_TrackWidthList.size() - 1;

    board->SetTrackWidthIndex( widthIndex );

    wxUpdateUIEvent dummy;
    getEditFrame<PCB_EDIT_FRAME>()->OnUpdateSelectTrackWidth( dummy );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::TrackWidthDec( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>( PCB_T );
    int widthIndex = board->GetTrackWidthIndex() - 1;

    if( widthIndex < 0 )
        widthIndex = 0;

    board->SetTrackWidthIndex( widthIndex );

    wxUpdateUIEvent dummy;
    getEditFrame<PCB_EDIT_FRAME>()->OnUpdateSelectTrackWidth( dummy );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ViaSizeInc( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>( PCB_T );
    int sizeIndex = board->GetViaSizeIndex() + 1;

    if( sizeIndex >= (int) board->m_ViasDimensionsList.size() )
        sizeIndex = board->m_ViasDimensionsList.size() - 1;

    board->SetViaSizeIndex( sizeIndex );

    wxUpdateUIEvent dummy;
    getEditFrame<PCB_EDIT_FRAME>()->OnUpdateSelectViaSize( dummy );
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ViaSizeDec( TOOL_EVENT& aEvent )
{
    BOARD* board = getModel<BOARD>( PCB_T );
    int sizeIndex = board->GetViaSizeIndex() - 1;

    if( sizeIndex < 0 )
        sizeIndex = 0;

    board->SetViaSizeIndex( sizeIndex );

    wxUpdateUIEvent dummy;
    getEditFrame<PCB_EDIT_FRAME>()->OnUpdateSelectViaSize( dummy );
    setTransitions();

    return 0;
}


// Miscellaneous
int PCBNEW_CONTROL::ResetCoords( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::SwitchUnits( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


int PCBNEW_CONTROL::ShowHelp( TOOL_EVENT& aEvent )
{
    std::cout << __PRETTY_FUNCTION__ << std::endl;
    setTransitions();

    return 0;
}


void PCBNEW_CONTROL::setTransitions()
{
    // View controls
    Go( &PCBNEW_CONTROL::ZoomIn,             COMMON_ACTIONS::zoomIn.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoomOut,            COMMON_ACTIONS::zoomOut.MakeEvent() );
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

    // Track & via size control
    Go( &PCBNEW_CONTROL::TrackWidthInc,      COMMON_ACTIONS::trackWidthInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::TrackWidthDec,      COMMON_ACTIONS::trackWidthDec.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaSizeInc,         COMMON_ACTIONS::viaSizeInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaSizeDec,         COMMON_ACTIONS::viaSizeDec.MakeEvent() );

    // Miscellaneous
    Go( &PCBNEW_CONTROL::ResetCoords,        COMMON_ACTIONS::resetCoords.MakeEvent() );
    Go( &PCBNEW_CONTROL::SwitchUnits,        COMMON_ACTIONS::switchUnits.MakeEvent() );
    Go( &PCBNEW_CONTROL::ShowHelp,           COMMON_ACTIONS::showHelp.MakeEvent() );
}
