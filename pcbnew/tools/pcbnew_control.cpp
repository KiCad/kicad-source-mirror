/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
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

#include <cstdint>

#include "pcbnew_control.h"
#include "pcb_actions.h"
#include "selection_tool.h"
#include "picker_tool.h"
#include "grid_helper.h"

#include <class_board.h>
#include <class_module.h>
#include <class_track.h>
#include <class_zone.h>
#include <class_pcb_screen.h>

#include <confirm.h>
#include <hotkeys.h>
#include <properties.h>
#include <io_mgr.h>

#include <pcbnew_id.h>
#include <wxPcbStruct.h>
#include <pcb_draw_panel_gal.h>
#include <ratsnest_data.h>
#include <tool/tool_manager.h>
#include <gal/graphics_abstraction_layer.h>
#include <view/view_controls.h>
#include <pcb_painter.h>
#include <origin_viewitem.h>
#include <board_commit.h>

#include <functional>
using namespace std::placeholders;


// files.cpp
extern bool AskLoadBoardFileName( wxWindow* aParent, int* aCtl, wxString* aFileName,
                                    bool aKicadFilesOnly = false );
extern IO_MGR::PCB_FILE_T plugin_type( const wxString& aFileName, int aCtl );


// Display modes
TOOL_ACTION PCB_ACTIONS::trackDisplayMode( "pcbnew.Control.trackDisplayMode",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_TRACK_DISPLAY_MODE ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::padDisplayMode( "pcbnew.Control.padDisplayMode",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::viaDisplayMode( "pcbnew.Control.viaDisplayMode",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::zoneDisplayEnable( "pcbnew.Control.zoneDisplayEnable",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::zoneDisplayDisable( "pcbnew.Control.zoneDisplayDisable",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::zoneDisplayOutlines( "pcbnew.Control.zoneDisplayOutlines",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::highContrastMode( "pcbnew.Control.highContrastMode",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_HIGHCONTRAST_MODE ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::highContrastInc( "pcbnew.Control.highContrastInc",
        AS_GLOBAL, '>',
        "", "" );

TOOL_ACTION PCB_ACTIONS::highContrastDec( "pcbnew.Control.highContrastDec",
        AS_GLOBAL, '<',
        "", "" );


// Layer control
TOOL_ACTION PCB_ACTIONS::layerTop( "pcbnew.Control.layerTop",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_COMPONENT ),
        "", "", NULL, AF_NONE, (void*) F_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner1( "pcbnew.Control.layerInner1",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER1 ),
        "", "", NULL, AF_NONE, (void*) In1_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner2( "pcbnew.Control.layerInner2",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER2 ),
        "", "", NULL, AF_NONE, (void*) In2_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner3( "pcbnew.Control.layerInner3",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER3 ),
        "", "", NULL, AF_NONE, (void*) In3_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner4( "pcbnew.Control.layerInner4",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER4 ),
        "", "", NULL, AF_NONE, (void*) In4_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner5( "pcbnew.Control.layerInner5",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER5 ),
        "", "", NULL, AF_NONE, (void*) In5_Cu );

TOOL_ACTION PCB_ACTIONS::layerInner6( "pcbnew.Control.layerInner6",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_INNER6 ),
        "", "", NULL, AF_NONE, (void*) In6_Cu );

TOOL_ACTION PCB_ACTIONS::layerBottom( "pcbnew.Control.layerBottom",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_COPPER ),
        "", "", NULL, AF_NONE, (void*) B_Cu );

TOOL_ACTION PCB_ACTIONS::layerNext( "pcbnew.Control.layerNext",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_NEXT ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::layerPrev( "pcbnew.Control.layerPrev",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_LAYER_TO_PREVIOUS ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::layerToggle( "pcbnew.Control.layerToggle",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ADD_THROUGH_VIA ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::layerAlphaInc( "pcbnew.Control.layerAlphaInc",
        AS_GLOBAL, '}',
        "", "" );

TOOL_ACTION PCB_ACTIONS::layerAlphaDec( "pcbnew.Control.layerAlphaDec",
        AS_GLOBAL, '{',
        "", "" );

TOOL_ACTION PCB_ACTIONS::layerChanged( "pcbnew.Control.layerChanged",
        AS_GLOBAL, 0,
        "", "", NULL, AF_NOTIFY );

// Cursor control
TOOL_ACTION PCB_ACTIONS::cursorUp( "pcbnew.Control.cursorUp",
        AS_GLOBAL, WXK_UP, "", "", NULL, AF_NONE, (void*) CURSOR_UP );
TOOL_ACTION PCB_ACTIONS::cursorDown( "pcbnew.Control.cursorDown",
        AS_GLOBAL, WXK_DOWN, "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );
TOOL_ACTION PCB_ACTIONS::cursorLeft( "pcbnew.Control.cursorLeft",
        AS_GLOBAL, WXK_LEFT, "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );
TOOL_ACTION PCB_ACTIONS::cursorRight( "pcbnew.Control.cursorRight",
        AS_GLOBAL, WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );

TOOL_ACTION PCB_ACTIONS::cursorUpFast( "pcbnew.Control.cursorUpFast",
        AS_GLOBAL, MD_CTRL + WXK_UP, "", "", NULL, AF_NONE, (void*) ( CURSOR_UP | CURSOR_FAST_MOVE ) );
TOOL_ACTION PCB_ACTIONS::cursorDownFast( "pcbnew.Control.cursorDownFast",
        AS_GLOBAL, MD_CTRL + WXK_DOWN, "", "" , NULL, AF_NONE, (void*) ( CURSOR_DOWN | CURSOR_FAST_MOVE ) );
TOOL_ACTION PCB_ACTIONS::cursorLeftFast( "pcbnew.Control.cursorLeftFast",
        AS_GLOBAL, MD_CTRL + WXK_LEFT, "", "" , NULL, AF_NONE, (void*) ( CURSOR_LEFT | CURSOR_FAST_MOVE ) );
TOOL_ACTION PCB_ACTIONS::cursorRightFast( "pcbnew.Control.cursorRightFast",
        AS_GLOBAL, MD_CTRL + WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) ( CURSOR_RIGHT | CURSOR_FAST_MOVE ) );

TOOL_ACTION PCB_ACTIONS::cursorClick( "pcbnew.Control.cursorClick",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEFT_CLICK ),
        "", "", NULL, AF_NONE, (void*) CURSOR_CLICK );
TOOL_ACTION PCB_ACTIONS::cursorDblClick( "pcbnew.Control.cursorDblClick",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_LEFT_DCLICK ),
        "", "", NULL, AF_NONE, (void*) CURSOR_DBL_CLICK );

TOOL_ACTION PCB_ACTIONS::panUp( "pcbnew.Control.panUp",
        AS_GLOBAL, MD_SHIFT + WXK_UP, "", "", NULL, AF_NONE, (void*) CURSOR_UP );
TOOL_ACTION PCB_ACTIONS::panDown( "pcbnew.Control.panDown",
        AS_GLOBAL, MD_SHIFT + WXK_DOWN, "", "" , NULL, AF_NONE, (void*) CURSOR_DOWN );
TOOL_ACTION PCB_ACTIONS::panLeft( "pcbnew.Control.panLeft",
        AS_GLOBAL, MD_SHIFT + WXK_LEFT, "", "" , NULL, AF_NONE, (void*) CURSOR_LEFT );
TOOL_ACTION PCB_ACTIONS::panRight( "pcbnew.Control.panRight",
        AS_GLOBAL, MD_SHIFT + WXK_RIGHT, "", "" , NULL, AF_NONE, (void*) CURSOR_RIGHT );

// Miscellaneous
TOOL_ACTION PCB_ACTIONS::selectionTool( "pcbnew.Control.selectionTool",
        AS_GLOBAL, 0,
        "", "", NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::zoomTool( "pcbnew.Control.zoomTool",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZOOM_SELECTION ),
        _( "Zoom to Selection" ), "", NULL, AF_ACTIVATE );

TOOL_ACTION PCB_ACTIONS::resetCoords( "pcbnew.Control.resetCoords",
        AS_GLOBAL, ' ',
        "", "" );

TOOL_ACTION PCB_ACTIONS::switchCursor( "pcbnew.Control.switchCursor",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::switchUnits( "pcbnew.Control.switchUnits",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_SWITCH_UNITS ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::deleteItemCursor( "pcbnew.Control.deleteItemCursor",
        AS_GLOBAL, 0,
        "", "" );

TOOL_ACTION PCB_ACTIONS::showHelp( "pcbnew.Control.showHelp",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_HELP ),
        "", "" );

TOOL_ACTION PCB_ACTIONS::toBeDone( "pcbnew.Control.toBeDone",
        AS_GLOBAL, 0,           // dialog saying it is not implemented yet
        "", "" );               // so users are aware of that


PCBNEW_CONTROL::PCBNEW_CONTROL() :
    TOOL_INTERACTIVE( "pcbnew.Control" ), m_frame( NULL )
{
    m_gridOrigin.reset( new KIGFX::ORIGIN_VIEWITEM() );
}


PCBNEW_CONTROL::~PCBNEW_CONTROL()
{
}


void PCBNEW_CONTROL::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<PCB_BASE_FRAME>();

    if( aReason == MODEL_RELOAD || aReason == GAL_SWITCH )
    {
        m_gridOrigin->SetPosition( getModel<BOARD>()->GetGridOrigin() );
        getView()->Remove( m_gridOrigin.get() );
        getView()->Add( m_gridOrigin.get() );
    }
}


int PCBNEW_CONTROL::TrackDisplayMode( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();

    // Apply new display options to the GAL canvas
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();
    displ_opts->m_DisplayPcbTrackFill = !displ_opts->m_DisplayPcbTrackFill;
    settings->LoadDisplayOptions( displ_opts );

    for( TRACK* track = getModel<BOARD>()->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_TRACE_T )
            getView()->Update( track, KIGFX::GEOMETRY );
    }

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::PadDisplayMode( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();

    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    // Apply new display options to the GAL canvas
    displ_opts->m_DisplayPadFill = !displ_opts->m_DisplayPadFill;
    settings->LoadDisplayOptions( displ_opts );

    for( MODULE* module = getModel<BOARD>()->m_Modules; module; module = module->Next() )
    {
        for( D_PAD* pad = module->Pads(); pad; pad = pad->Next() )
            getView()->Update( pad, KIGFX::GEOMETRY );
    }

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ViaDisplayMode( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    // Apply new display options to the GAL canvas
    displ_opts->m_DisplayViaFill = !displ_opts->m_DisplayViaFill;
    settings->LoadDisplayOptions( displ_opts );

    for( TRACK* track = getModel<BOARD>()->m_Track; track; track = track->Next() )
    {
        if( track->Type() == PCB_TRACE_T || track->Type() == PCB_VIA_T )
            getView()->Update( track, KIGFX::GEOMETRY );
    }

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::ZoneDisplayMode( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    // Apply new display options to the GAL canvas
    if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayEnable ) )
        displ_opts->m_DisplayZonesMode = 0;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayDisable ) )
        displ_opts->m_DisplayZonesMode = 1;
    else if( aEvent.IsAction( &PCB_ACTIONS::zoneDisplayOutlines ) )
        displ_opts->m_DisplayZonesMode = 2;
    else
        assert( false );

    settings->LoadDisplayOptions( displ_opts );

    BOARD* board = getModel<BOARD>();
    for( int i = 0; i < board->GetAreaCount(); ++i )
        getView()->Update( board->GetArea( i ), KIGFX::GEOMETRY );

    m_frame->GetGalCanvas()->Refresh();

    return 0;
}


int PCBNEW_CONTROL::HighContrastMode( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();
    DISPLAY_OPTIONS* displ_opts = (DISPLAY_OPTIONS*)m_frame->GetDisplayOptions();

    displ_opts->m_ContrastModeDisplay = !displ_opts->m_ContrastModeDisplay;
    settings->LoadDisplayOptions( displ_opts );
    m_frame->GetGalCanvas()->SetHighContrastLayer( m_frame->GetActiveLayer() );

    return 0;
}


int PCBNEW_CONTROL::HighContrastInc( const TOOL_EVENT& aEvent )
{
    return 0;
}


int PCBNEW_CONTROL::HighContrastDec( const TOOL_EVENT& aEvent )
{
    return 0;
}


// Layer control
int PCBNEW_CONTROL::LayerSwitch( const TOOL_EVENT& aEvent )
{
    m_frame->SwitchLayer( NULL, (LAYER_ID) aEvent.Parameter<intptr_t>() );

    return 0;
}


int PCBNEW_CONTROL::LayerNext( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    int layerCount = getModel<BOARD>()->GetCopperLayerCount();

    if( layer == layerCount - 2 || layerCount < 2 )
        layer = B_Cu;
    else if( layer == B_Cu )
        layer = F_Cu;
    else
        ++layer;

    assert( IsCopperLayer( layer ) );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerPrev( const TOOL_EVENT& aEvent )
{
    PCB_BASE_FRAME* editFrame = m_frame;
    LAYER_NUM layer = editFrame->GetActiveLayer();

    if( layer < F_Cu || layer > B_Cu )
        return 0;

    int layerCount = getModel<BOARD>()->GetCopperLayerCount();

    if( layer == F_Cu || layerCount < 2 )
        layer = B_Cu;
    else if( layer == B_Cu )
        layer = layerCount - 2;
    else
        --layer;

    assert( IsCopperLayer( layer ) );
    editFrame->SwitchLayer( NULL, ToLAYER_ID( layer ) );

    return 0;
}


int PCBNEW_CONTROL::LayerToggle( const TOOL_EVENT& aEvent )
{
    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    PCB_SCREEN* screen = m_frame->GetScreen();

    if( currentLayer == screen->m_Route_Layer_TOP )
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_BOTTOM );
    else
        m_frame->SwitchLayer( NULL, screen->m_Route_Layer_TOP );

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaInc( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetLayerColor( currentLayer );

    if( currentColor.a <= 0.95 )
    {
        currentColor.a += 0.05;
        settings->SetLayerColor( currentLayer, currentColor );
        m_frame->GetGalCanvas()->GetView()->UpdateLayerColor( currentLayer );
    }

    return 0;
}


int PCBNEW_CONTROL::LayerAlphaDec( const TOOL_EVENT& aEvent )
{
    auto painter = static_cast<KIGFX::PCB_PAINTER*>( getView()->GetPainter() );
    auto settings = painter->GetSettings();

    LAYER_NUM currentLayer = m_frame->GetActiveLayer();
    KIGFX::COLOR4D currentColor = settings->GetLayerColor( currentLayer );

    if( currentColor.a >= 0.05 )
    {
        currentColor.a -= 0.05;
        settings->SetLayerColor( currentLayer, currentColor );
        m_frame->GetGalCanvas()->GetView()->UpdateLayerColor( currentLayer );
    }

    return 0;
}


// Cursor control
int PCBNEW_CONTROL::CursorControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    bool fastMove = type & PCB_ACTIONS::CURSOR_FAST_MOVE;
    type &= ~PCB_ACTIONS::CURSOR_FAST_MOVE;
    bool mirroredX = getView()->IsMirroredX();

    GRID_HELPER gridHelper( m_frame );
    VECTOR2D cursor = getViewControls()->GetCursorPosition();
    VECTOR2I gridSize = gridHelper.GetGrid();
    VECTOR2D newCursor = gridHelper.Align( cursor );

    if( fastMove )
        gridSize = gridSize * 10;

    switch( type )
    {
        case PCB_ACTIONS::CURSOR_UP:
            newCursor -= VECTOR2D( 0, gridSize.y );
            break;

        case PCB_ACTIONS::CURSOR_DOWN:
            newCursor += VECTOR2D( 0, gridSize.y );
            break;

        case PCB_ACTIONS::CURSOR_LEFT:
            newCursor -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case PCB_ACTIONS::CURSOR_RIGHT:
            newCursor += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case PCB_ACTIONS::CURSOR_CLICK:              // fall through
        case PCB_ACTIONS::CURSOR_DBL_CLICK:
        {
            TOOL_ACTIONS action = TA_NONE;
            int modifiers = 0;

            modifiers |= wxGetKeyState( WXK_SHIFT ) ? MD_SHIFT : 0;
            modifiers |= wxGetKeyState( WXK_CONTROL ) ? MD_CTRL : 0;
            modifiers |= wxGetKeyState( WXK_ALT ) ? MD_ALT : 0;

            if( type == PCB_ACTIONS::CURSOR_CLICK )
                action = TA_MOUSE_CLICK;
            else if( type == PCB_ACTIONS::CURSOR_DBL_CLICK )
                action = TA_MOUSE_DBLCLICK;
            else
                assert( false );

            TOOL_EVENT evt( TC_MOUSE, action, BUT_LEFT | modifiers );
            evt.SetMousePosition( getViewControls()->GetCursorPosition() );
            m_toolMgr->ProcessEvent( evt );

            return 0;
        }
        break;
    }

    // Handler cursor movement
    KIGFX::VIEW* view = getView();
    newCursor = view->ToScreen( newCursor );
    newCursor.x = KiROUND( newCursor.x );
    newCursor.y = KiROUND( newCursor.y );

    // Pan the screen if required
    const VECTOR2I& screenSize = view->GetGAL()->GetScreenPixelSize();
    BOX2I screenBox( VECTOR2I( 0, 0 ), screenSize );

    if( !screenBox.Contains( newCursor ) )
    {
        VECTOR2D delta( 0, 0 );

        if( newCursor.x < screenBox.GetLeft() )
        {
            delta.x = newCursor.x - screenBox.GetLeft();
            newCursor.x = screenBox.GetLeft();
        }
        else if( newCursor.x > screenBox.GetRight() )
        {
            delta.x = newCursor.x - screenBox.GetRight();
            // -1 is to keep the cursor within the drawing area,
            // so the cursor coordinates are still updated
            newCursor.x = screenBox.GetRight() - 1;
        }

        if( newCursor.y < screenBox.GetTop() )
        {
            delta.y = newCursor.y - screenBox.GetTop();
            newCursor.y = screenBox.GetTop();
        }
        else if( newCursor.y > screenBox.GetBottom() )
        {
            delta.y = newCursor.y - screenBox.GetBottom();
            // -1 is to keep the cursor within the drawing area,
            // so the cursor coordinates are still updated
            newCursor.y = screenBox.GetBottom() - 1;
        }

        view->SetCenter( view->GetCenter() + view->ToWorld( delta, false ) );
    }

    m_frame->GetGalCanvas()->WarpPointer( newCursor.x, newCursor.y );

    return 0;
}


int PCBNEW_CONTROL::PanControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    KIGFX::VIEW* view = getView();
    GRID_HELPER gridHelper( m_frame );
    VECTOR2D center = view->GetCenter();
    VECTOR2I gridSize = gridHelper.GetGrid() * 10;
    bool mirroredX = view->IsMirroredX();

    switch( type )
    {
        case PCB_ACTIONS::CURSOR_UP:
            center -= VECTOR2D( 0, gridSize.y );
            break;

        case PCB_ACTIONS::CURSOR_DOWN:
            center += VECTOR2D( 0, gridSize.y );
            break;

        case PCB_ACTIONS::CURSOR_LEFT:
            center -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        case PCB_ACTIONS::CURSOR_RIGHT:
            center += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
            break;

        default:
            assert( false );
            break;
    }

    view->SetCenter( center );

    return 0;
}


// Grid control
int PCBNEW_CONTROL::GridFast1( const TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid1();
    updateGrid();

    return 0;
}


int PCBNEW_CONTROL::GridFast2( const TOOL_EVENT& aEvent )
{
    m_frame->SetFastGrid2();
    updateGrid();

    return 0;
}


static bool setOrigin( KIGFX::VIEW* aView, PCB_BASE_FRAME* aFrame,
                       KIGFX::ORIGIN_VIEWITEM* aItem, const VECTOR2D& aPoint )
{
    aFrame->SetGridOrigin( wxPoint( aPoint.x, aPoint.y ) );
    aView->GetGAL()->SetGridOrigin( aPoint );
    aItem->SetPosition( aPoint );
    aView->MarkDirty();

    return true;
}


int PCBNEW_CONTROL::GridSetOrigin( const TOOL_EVENT& aEvent )
{
    VECTOR2D* origin = aEvent.Parameter<VECTOR2D*>();

    if( origin )
    {
        setOrigin( getView(), m_frame, m_gridOrigin.get(), *origin );
        delete origin;
    }
    else
    {
        Activate();

        PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();
        assert( picker );

        // TODO it will not check the toolbar button in module editor, as it uses a different ID..
        m_frame->SetToolID( ID_PCB_PLACE_GRID_COORD_BUTT, wxCURSOR_PENCIL, _( "Adjust grid origin" ) );
        picker->SetClickHandler( std::bind( setOrigin, getView(), m_frame, m_gridOrigin.get(), _1 ) );
        picker->Activate();
        Wait();
    }

    return 0;
}


int PCBNEW_CONTROL::GridResetOrigin( const TOOL_EVENT& aEvent )
{
    getModel<BOARD>()->SetGridOrigin( wxPoint( 0, 0 ) );
    m_gridOrigin->SetPosition( VECTOR2D( 0, 0 ) );

    return 0;
}


// Miscellaneous
int PCBNEW_CONTROL::ResetCoords( const TOOL_EVENT& aEvent )
{
    VECTOR2I cursorPos = getViewControls()->GetCursorPosition();

    m_frame->GetScreen()->m_O_Curseur = wxPoint( cursorPos.x, cursorPos.y );
    m_frame->UpdateStatusBar();

    return 0;
}


int PCBNEW_CONTROL::SwitchCursor( const TOOL_EVENT& aEvent )
{
    const unsigned int BIG_CURSOR = 8000;
    const unsigned int SMALL_CURSOR = 80;

    PCB_BASE_FRAME* frame = getEditFrame<PCB_BASE_FRAME>();
    KIGFX::GAL* gal = frame->GetGalCanvas()->GetGAL();
    gal->SetCursorSize( frame->GetCursorShape() ? BIG_CURSOR : SMALL_CURSOR );

    return 0;
}


int PCBNEW_CONTROL::SwitchUnits( const TOOL_EVENT& aEvent )
{
    // TODO should not it be refactored to pcb_frame member function?
    wxCommandEvent evt( wxEVT_COMMAND_MENU_SELECTED );

    if( g_UserUnit == INCHES )
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_MM );
    else
        evt.SetId( ID_TB_OPTIONS_SELECT_UNIT_INCH );

    m_frame->ProcessEvent( evt );

    return 0;
}


static bool deleteItem( TOOL_MANAGER* aToolMgr, const VECTOR2D& aPosition )
{
    SELECTION_TOOL* selectionTool = aToolMgr->GetTool<SELECTION_TOOL>();
    assert( selectionTool );

    aToolMgr->RunAction( PCB_ACTIONS::selectionClear, true );
    aToolMgr->RunAction( PCB_ACTIONS::selectionCursor, true );
    selectionTool->SanitizeSelection();

    const SELECTION& selection = selectionTool->GetSelection();

    if( selection.Empty() )
        return true;

    bool canBeRemoved = ( selection.Front()->Type() != PCB_MODULE_T );

    if( canBeRemoved || IsOK( aToolMgr->GetEditFrame(), _( "Are you sure you want to delete item?" ) ) )
        aToolMgr->RunAction( PCB_ACTIONS::remove, true );
    else
        aToolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    return true;
}


int PCBNEW_CONTROL::DeleteItemCursor( const TOOL_EVENT& aEvent )
{
    Activate();

    PICKER_TOOL* picker = m_toolMgr->GetTool<PICKER_TOOL>();
    assert( picker );

    // TODO it will not check the toolbar button in the module editor, as it uses a different ID..
    m_frame->SetToolID( ID_PCB_DELETE_ITEM_BUTT, wxCURSOR_PENCIL, _( "Delete item" ) );
    picker->SetSnapping( false );
    picker->SetClickHandler( std::bind( deleteItem, m_toolMgr, _1 ) );
    picker->Activate();
    Wait();

    return 0;
}


int PCBNEW_CONTROL::AppendBoard( const TOOL_EVENT& aEvent )
{
    int open_ctl;
    wxString fileName;

    PCB_EDIT_FRAME* editFrame = dynamic_cast<PCB_EDIT_FRAME*>( m_frame );

    if( !editFrame )
        return 0;

    BOARD* board = getModel<BOARD>();
    BOARD_COMMIT commit( editFrame );

    // Pick a file to append
    if( !AskLoadBoardFileName( editFrame, &open_ctl, &fileName, true ) )
        return 0;

    IO_MGR::PCB_FILE_T pluginType = plugin_type( fileName, open_ctl );
    PLUGIN::RELEASER pi( IO_MGR::PluginFind( pluginType ) );

    // keep track of existing items, in order to know what are the new items
    // (for undo command for instance)

    // Tracks are inserted, not appended, so mark the existing tracks to know what are the new tracks
    // TODO legacy
    for( TRACK* track = board->m_Track; track; track = track->Next() )
        track->SetFlags( FLAG0 );

    // Other items are appended to the item list, so keep trace to the last existing item is enough
    MODULE* module = board->m_Modules.GetLast();
    BOARD_ITEM* drawing = board->m_Drawings.GetLast();
    int zonescount = board->GetAreaCount();

    // Keep also the count of copper layers, to adjust if necessary
    int initialCopperLayerCount = board->GetCopperLayerCount();
    LSET initialEnabledLayers = board->GetEnabledLayers();

    // Load the data
    try
    {
        PROPERTIES  props;
        char        xbuf[30];
        char        ybuf[30];

        // EAGLE_PLUGIN can use this info to center the BOARD, but it does not yet.
        sprintf( xbuf, "%d", editFrame->GetPageSizeIU().x );
        sprintf( ybuf, "%d", editFrame->GetPageSizeIU().y );

        props["page_width"]  = xbuf;
        props["page_height"] = ybuf;

        editFrame->GetDesignSettings().m_NetClasses.Clear();
        pi->Load( fileName, board, &props );
    }
    catch( const IO_ERROR& ioe )
    {
        wxString msg = wxString::Format( _( "Error loading board.\n%s" ), GetChars( ioe.What() ));
        DisplayError( editFrame, msg );

        return 0;
    }

    m_toolMgr->RunAction( PCB_ACTIONS::selectionClear, true );

    // Process the new items
    for( TRACK* track = board->m_Track; track; track = track->Next() )
    {
        if( track->GetFlags() & FLAG0 )
        {
            track->ClearFlags( FLAG0 );
            continue;
        }

        commit.Added( track );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, track );
    }

    module = module ? module->Next() : board->m_Modules;

    for( ; module; module = module->Next() )
    {
        commit.Added( module );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, module );
    }

    drawing = drawing ? drawing->Next() : board->m_Drawings;

    for( ; drawing; drawing = drawing->Next() )
    {
        commit.Added( drawing );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, drawing );
    }

    for( ZONE_CONTAINER* zone = board->GetArea( zonescount ); zone;
         zone = board->GetArea( zonescount ) )
    {
        ++zonescount;
        commit.Added( zone );
        m_toolMgr->RunAction( PCB_ACTIONS::selectItem, true, zone );
    }

    if( commit.Empty() )
        return 0;

    commit.Push( _( "Append a board" ) );

    // Synchronize layers
    // we should not ask PLUGINs to do these items:
    int copperLayerCount = board->GetCopperLayerCount();

    if( copperLayerCount > initialCopperLayerCount )
        board->SetCopperLayerCount( copperLayerCount );

    // Enable all used layers, and make them visible:
    LSET enabledLayers = board->GetEnabledLayers();
    enabledLayers |= initialEnabledLayers;
    board->SetEnabledLayers( enabledLayers );
    board->SetVisibleLayers( enabledLayers );
    editFrame->ReCreateLayerBox();
    editFrame->ReFillLayerWidget();
    static_cast<PCB_DRAW_PANEL_GAL*>( editFrame->GetGalCanvas() )->SyncLayersVisibility( board );

    // Ratsnest
    board->BuildListOfNets();
    board->SynchronizeNetsAndNetClasses();
    board->GetRatsnest()->ProcessBoard();

    // Start dragging the appended board
    SELECTION_TOOL* selectionTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const SELECTION& selection = selectionTool->GetSelection();
    VECTOR2D v( static_cast<BOARD_ITEM*>( selection.Front() )->GetPosition() );
    getViewControls()->WarpCursor( v, true, true );
    m_toolMgr->InvokeTool( "pcbnew.InteractiveEdit" );

    return 0;
}


int PCBNEW_CONTROL::ShowHelp( const TOOL_EVENT& aEvent )
{
    DisplayHotkeyList( m_frame, m_frame->GetHotkeyConfig() );

    return 0;
}


int PCBNEW_CONTROL::ToBeDone( const TOOL_EVENT& aEvent )
{
    DisplayInfoMessage( m_frame, _( "Not available in OpenGL/Cairo canvases." ) );

    return 0;
}


void PCBNEW_CONTROL::SetTransitions()
{
    // Display modes
    Go( &PCBNEW_CONTROL::TrackDisplayMode,   PCB_ACTIONS::trackDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::PadDisplayMode,     PCB_ACTIONS::padDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ViaDisplayMode,     PCB_ACTIONS::viaDisplayMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,    PCB_ACTIONS::zoneDisplayEnable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,    PCB_ACTIONS::zoneDisplayDisable.MakeEvent() );
    Go( &PCBNEW_CONTROL::ZoneDisplayMode,    PCB_ACTIONS::zoneDisplayOutlines.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastMode,   PCB_ACTIONS::highContrastMode.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastInc,    PCB_ACTIONS::highContrastInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::HighContrastDec,    PCB_ACTIONS::highContrastDec.MakeEvent() );

    // Layer control
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerTop.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner1.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner2.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner3.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner4.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner5.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerInner6.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerSwitch,        PCB_ACTIONS::layerBottom.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerNext,          PCB_ACTIONS::layerNext.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerPrev,          PCB_ACTIONS::layerPrev.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerToggle,        PCB_ACTIONS::layerToggle.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaInc,      PCB_ACTIONS::layerAlphaInc.MakeEvent() );
    Go( &PCBNEW_CONTROL::LayerAlphaDec,      PCB_ACTIONS::layerAlphaDec.MakeEvent() );

    // Cursor control
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorUp.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorDown.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorLeft.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorRight.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorUpFast.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorDownFast.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorLeftFast.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorRightFast.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorClick.MakeEvent() );
    Go( &PCBNEW_CONTROL::CursorControl,      PCB_ACTIONS::cursorDblClick.MakeEvent() );

    // Pan control
    Go( &PCBNEW_CONTROL::PanControl,         PCB_ACTIONS::panUp.MakeEvent() );
    Go( &PCBNEW_CONTROL::PanControl,         PCB_ACTIONS::panDown.MakeEvent() );
    Go( &PCBNEW_CONTROL::PanControl,         PCB_ACTIONS::panLeft.MakeEvent() );
    Go( &PCBNEW_CONTROL::PanControl,         PCB_ACTIONS::panRight.MakeEvent() );

    // Grid control
    Go( &PCBNEW_CONTROL::GridFast1,          ACTIONS::gridFast1.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridFast2,          ACTIONS::gridFast2.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridSetOrigin,      ACTIONS::gridSetOrigin.MakeEvent() );
    Go( &PCBNEW_CONTROL::GridResetOrigin,    ACTIONS::gridResetOrigin.MakeEvent() );

    // Miscellaneous
    Go( &PCBNEW_CONTROL::ResetCoords,        PCB_ACTIONS::resetCoords.MakeEvent() );
    Go( &PCBNEW_CONTROL::SwitchCursor,       PCB_ACTIONS::switchCursor.MakeEvent() );
    Go( &PCBNEW_CONTROL::SwitchUnits,        PCB_ACTIONS::switchUnits.MakeEvent() );
    Go( &PCBNEW_CONTROL::DeleteItemCursor,   PCB_ACTIONS::deleteItemCursor.MakeEvent() );
    Go( &PCBNEW_CONTROL::AppendBoard,        PCB_ACTIONS::appendBoard.MakeEvent() );
    Go( &PCBNEW_CONTROL::ShowHelp,           PCB_ACTIONS::showHelp.MakeEvent() );
    Go( &PCBNEW_CONTROL::ToBeDone,           PCB_ACTIONS::toBeDone.MakeEvent() );
}


void PCBNEW_CONTROL::updateGrid()
{
    BASE_SCREEN* screen = m_frame->GetScreen();
    //GRID_TYPE grid = screen->GetGrid( idx );
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}
