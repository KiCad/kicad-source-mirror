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

#include <bitmaps.h>
#include <tool/actions.h>
#include <tool/tool_manager.h>
#include <eda_draw_frame.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/view_controls.h>
#include <gal/graphics_abstraction_layer.h>
#include <base_screen.h>
#include <tool/common_tools.h>
#include <id.h>
#include <project.h>
#include <kiface_i.h>
#include <dialog_configure_paths.h>

void COMMON_TOOLS::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
}


int COMMON_TOOLS::SelectionTool( const TOOL_EVENT& aEvent )
{
    // Since selection tools are run permanently underneath the toolStack, this is really
    // just a cancel of whatever other tools might be running.

    m_toolMgr->ProcessEvent( TOOL_EVENT( TC_COMMAND, TA_CANCEL_TOOL ) );
    return 0;
}


// Cursor control
int COMMON_TOOLS::CursorControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    bool fastMove = type & ACTIONS::CURSOR_FAST_MOVE;
    type &= ~ACTIONS::CURSOR_FAST_MOVE;
    bool mirroredX = getView()->IsMirroredX();

    VECTOR2D cursor = getViewControls()->GetRawCursorPosition( false );
    VECTOR2I gridSize = VECTOR2D( m_frame->GetScreen()->GetGridSize() );

    if( fastMove )
        gridSize = gridSize * 10;

    switch( type )
    {
    case ACTIONS::CURSOR_UP:
        cursor -= VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_DOWN:
        cursor += VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_LEFT:
        cursor -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    case ACTIONS::CURSOR_RIGHT:
        cursor += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    case ACTIONS::CURSOR_CLICK:              // fall through
    case ACTIONS::CURSOR_DBL_CLICK:
    case ACTIONS::CURSOR_RIGHT_CLICK:
    {
        TOOL_ACTIONS action = TA_MOUSE_CLICK;
        TOOL_MOUSE_BUTTONS button = BUT_LEFT;
        int modifiers = 0;

        modifiers |= wxGetKeyState( WXK_SHIFT ) ? MD_SHIFT : 0;
        modifiers |= wxGetKeyState( WXK_CONTROL ) ? MD_CTRL : 0;
        modifiers |= wxGetKeyState( WXK_ALT ) ? MD_ALT : 0;

        if( type == ACTIONS::CURSOR_DBL_CLICK )
            action = TA_MOUSE_DBLCLICK;

        if( type == ACTIONS::CURSOR_RIGHT_CLICK )
            button = BUT_RIGHT;

        TOOL_EVENT evt( TC_MOUSE, action, button | modifiers );
        evt.SetMousePosition( getViewControls()->GetCursorPosition() );
        m_toolMgr->ProcessEvent( evt );

        return 0;
    }
    default:
        wxFAIL_MSG( "CursorControl(): unexpected request" );
    }

    getViewControls()->SetCursorPosition( cursor, true, true );
    m_toolMgr->RunAction( ACTIONS::refreshPreview );

    return 0;
}


int COMMON_TOOLS::PanControl( const TOOL_EVENT& aEvent )
{
    long type = aEvent.Parameter<intptr_t>();
    KIGFX::VIEW* view = getView();
    VECTOR2D center = view->GetCenter();
    VECTOR2I gridSize = VECTOR2D( m_frame->GetScreen()->GetGridSize() ) * 10;
    bool mirroredX = view->IsMirroredX();

    switch( type )
    {
    case ACTIONS::CURSOR_UP:
        center -= VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_DOWN:
        center += VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_LEFT:
        center -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    case ACTIONS::CURSOR_RIGHT:
        center += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    default:
        wxFAIL;
        break;
    }

    view->SetCenter( center );

    return 0;
}


int COMMON_TOOLS::ZoomRedraw( const TOOL_EVENT& aEvent )
{
    m_frame->HardRedraw();
    return 0;
}


int COMMON_TOOLS::ZoomInOut( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomIn );
    return doZoomInOut( direction, true );
}


int COMMON_TOOLS::ZoomInOutCenter( const TOOL_EVENT& aEvent )
{
    bool direction = aEvent.IsAction( &ACTIONS::zoomInCenter );
    return doZoomInOut( direction, false );
}


int COMMON_TOOLS::doZoomInOut( bool aDirection, bool aCenterOnCursor )
{
    double zoom = m_frame->GetCanvas()->GetLegacyZoom();

    // Step must be AT LEAST 1.3
    if( aDirection )
        zoom /= 1.3;
    else
        zoom *= 1.3;

    // Now look for the next closest menu step
    std::vector<double>& zoomList = m_frame->GetScreen()->m_ZoomList;
    int idx;

    if( aDirection )
    {
        for( idx = zoomList.size() - 1; idx >= 0; --idx )
        {
            if( zoomList[idx] <= zoom )
                break;
        }

        if( idx < 0 )
            idx = 0;        // if we ran off the end then peg to the end
    }
    else
    {
        for( idx = 0; idx < (int)zoomList.size(); ++idx )
        {
            if( zoomList[idx] >= zoom )
                break;
        }

        if( idx >= (int)zoomList.size() )
            idx = zoomList.size() - 1;        // if we ran off the end then peg to the end
    }

    return doZoomToPreset( idx + 1, aCenterOnCursor );
}


int COMMON_TOOLS::ZoomCenter( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW_CONTROLS* ctls = getViewControls();

    ctls->CenterOnCursor();

    return 0;
}


int COMMON_TOOLS::ZoomFitScreen( const TOOL_EVENT& aEvent )
{
    KIGFX::VIEW*        view = getView();
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();
    EDA_DRAW_FRAME*     frame = getEditFrame<EDA_DRAW_FRAME>();

    BOX2I    bBox = frame->GetDocumentExtents();
    BOX2I    defaultBox = canvas->GetDefaultViewBBox();
    VECTOR2D scrollbarSize = VECTOR2D( canvas->GetSize() - canvas->GetClientSize() );

    view->SetScale( 1.0 );  // the best scale will be fixed later, from this initial value
                            // but this call ensure all view parameters are up to date
                            // especially at init time
    VECTOR2D screenSize = view->ToWorld( canvas->GetClientSize(), false );

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = defaultBox;

    VECTOR2D vsize = bBox.GetSize();
    double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                fabs( vsize.y / screenSize.y ) );

    // Reserve a 10% margin around component bounding box.
    double margin_scale_factor = 1.1;

    // Leave 20% for library editors & viewers
    if( frame->IsType( FRAME_PCB_MODULE_VIEWER ) || frame->IsType( FRAME_PCB_MODULE_VIEWER_MODAL )
            || frame->IsType( FRAME_SCH_VIEWER ) || frame->IsType( FRAME_SCH_VIEWER_MODAL )
            || frame->IsType( FRAME_SCH_LIB_EDITOR ) || frame->IsType( FRAME_PCB_MODULE_EDITOR ) )
    {
        margin_scale_factor = 1.2;
    }

    view->SetScale( scale / margin_scale_factor );
    view->SetCenter( bBox.Centre() );

    // Take scrollbars into account
    VECTOR2D worldScrollbarSize = view->ToWorld( scrollbarSize, false );
    view->SetCenter( view->GetCenter() + worldScrollbarSize / 2.0 );

    return 0;
}


int COMMON_TOOLS::CenterContents( const TOOL_EVENT& aEvent )
{
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();
    BOX2I bBox = getModel<EDA_ITEM>()->ViewBBox();

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = canvas->GetDefaultViewBBox();

    getView()->SetCenter( bBox.Centre() );

    // Take scrollbars into account
    VECTOR2D scrollbarSize = VECTOR2D( canvas->GetSize() - canvas->GetClientSize() );
    VECTOR2D worldScrollbarSize = getView()->ToWorld( scrollbarSize, false );
    getView()->SetCenter( getView()->GetCenter() + worldScrollbarSize / 2.0 );

    return 0;
}


int COMMON_TOOLS::ZoomPreset( const TOOL_EVENT& aEvent )
{
    unsigned int idx = aEvent.Parameter<intptr_t>();
    return doZoomToPreset( idx, false );
}


// Note: idx == 0 is Auto; idx == 1 is first entry in zoomList
int COMMON_TOOLS::doZoomToPreset( int idx, bool aCenterOnCursor )
{
    std::vector<double>& zoomList = m_frame->GetScreen()->m_ZoomList;
    KIGFX::VIEW* view = m_frame->GetCanvas()->GetView();

    if( idx == 0 )      // Zoom Auto
    {
        TOOL_EVENT dummy;
        return ZoomFitScreen( dummy );
    }
    else
    {
        idx--;
    }

    double scale = m_frame->GetZoomLevelCoeff() / zoomList[idx];

    if( aCenterOnCursor )
    {
        view->SetScale( scale, getViewControls()->GetCursorPosition() );

        if( getViewControls()->IsCursorWarpingEnabled() )
            getViewControls()->CenterOnCursor();
    }
    else
    {
        view->SetScale( scale );
    }

    return 0;
}


/**
 * Advance a BASE_SCREEN's grid forwards or backwards by the given offset and
 * return the cmd ID of that grid (doesn't change the grid).
 *
 * This works even if the base screen's grid do not have consecutive command IDs.
 *
 * @param aScreen the base screen to use
 * @param aOffset how many grids to advance by (negative to go backwards)
 * @return the cmd ID of the requested grid, or empty if it can't be found
 */
static OPT<int> getNextPreviousGrid( const BASE_SCREEN& aScreen, int aOffset )
{
    const GRIDS&     grids = aScreen.GetGrids();
    const GRID_TYPE& currGrid = aScreen.GetGrid();

    auto iter = std::find_if( grids.begin(), grids.end(),
            [&]( const GRID_TYPE& aCandidate ) { return aCandidate.m_CmdId == currGrid.m_CmdId; } );

    wxCHECK_MSG( iter != grids.end(), {}, "Grid not found in screen's grid list" );

    int index = std::distance( grids.begin(), iter ) + aOffset;

    // If we go off the end, return invalid, but we could also wrap around if wanted.
    if( index < 0 || static_cast<size_t>( index ) >= grids.size() )
        return {};

    return grids[index].m_CmdId;
}


// Grid control
int COMMON_TOOLS::GridNext( const TOOL_EVENT& aEvent )
{
    const OPT<int> next_grid_id = getNextPreviousGrid( *m_frame->GetScreen(), 1 );

    if( next_grid_id )
        return GridPreset( *next_grid_id - ID_POPUP_GRID_LEVEL_1000 );

    return 1;
}


int COMMON_TOOLS::GridPrev( const TOOL_EVENT& aEvent )
{
    const OPT<int> next_grid_id = getNextPreviousGrid( *m_frame->GetScreen(), -1 );

    if( next_grid_id )
        return GridPreset( *next_grid_id - ID_POPUP_GRID_LEVEL_1000 );

    return 1;
}


int COMMON_TOOLS::GridPreset( const TOOL_EVENT& aEvent )
{
    return GridPreset( aEvent.Parameter<intptr_t>() );
}


int COMMON_TOOLS::GridPreset( int idx )
{
    BASE_SCREEN* screen = m_frame->GetScreen();

    if( !screen->GridExists( idx + ID_POPUP_GRID_LEVEL_1000 ) )
        idx = 0;

    screen->SetGrid( idx + ID_POPUP_GRID_LEVEL_1000 );

    // Be sure m_LastGridSizeId is up to date.
    m_frame->SetLastGridSizeId( idx );

    // Update the combobox (if any)
    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectGrid( dummy );

    // Update GAL canvas from screen
    getView()->GetGAL()->SetGridSize( VECTOR2D( screen->GetGridSize() ) );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    // Put cursor on new grid
    VECTOR2D gridCursor = getViewControls()->GetCursorPosition( true );
    getViewControls()->SetCrossHairCursorPosition( gridCursor, false );
    return 0;
}


int COMMON_TOOLS::ToggleGrid( const TOOL_EVENT& aEvent )
{
    m_frame->SetGridVisibility( !m_frame->IsGridVisible() );

    m_frame->GetCanvas()->GetGAL()->SetGridVisibility( m_frame->IsGridVisible() );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    m_frame->GetCanvas()->Refresh();

    return 0;
}


int COMMON_TOOLS::GridProperties( const TOOL_EVENT& aEvent )
{
    wxCommandEvent cmd( wxEVT_COMMAND_MENU_SELECTED );

    cmd.SetId( ID_GRID_SETTINGS );
    m_frame->ProcessEvent( cmd );

    return 0;
}


int COMMON_TOOLS::MetricUnits( const TOOL_EVENT& aEvent )
{
    m_frame->ChangeUserUnits( MILLIMETRES );
    return 0;
}


int COMMON_TOOLS::ImperialUnits( const TOOL_EVENT& aEvent )
{
    m_frame->ChangeUserUnits( INCHES );
    return 0;
}


int COMMON_TOOLS::ToggleUnits( const TOOL_EVENT& aEvent )
{
    m_frame->ChangeUserUnits( m_frame->GetUserUnits() == INCHES ? MILLIMETRES : INCHES );
    return 0;
}


int COMMON_TOOLS::TogglePolarCoords( const TOOL_EVENT& aEvent )
{
    m_frame->SetStatusText( wxEmptyString );
    m_frame->SetShowPolarCoords( !m_frame->GetShowPolarCoords() );
    m_frame->UpdateStatusBar();

    return 0;
}


int COMMON_TOOLS::ResetLocalCoords( const TOOL_EVENT& aEvent )
{
    auto vcSettings = m_toolMgr->GetCurrentToolVC();

    // Use either the active tool forced cursor position or the general settings
    if( vcSettings.m_forceCursorPosition )
        m_frame->GetScreen()->m_LocalOrigin = vcSettings.m_forcedPosition;
    else
        m_frame->GetScreen()->m_LocalOrigin = getViewControls()->GetCursorPosition();

    m_frame->UpdateStatusBar();

    return 0;
}


int COMMON_TOOLS::ToggleCursor( const TOOL_EVENT& aEvent )
{
    auto& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.m_forceDisplayCursor = !galOpts.m_forceDisplayCursor;
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::ToggleCursorStyle( const TOOL_EVENT& aEvent )
{
    KIGFX::GAL_DISPLAY_OPTIONS& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.m_fullscreenCursor = !galOpts.m_fullscreenCursor;
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::SwitchCanvas( const TOOL_EVENT& aEvent )
{
    if( aEvent.IsAction( &ACTIONS::acceleratedGraphics ) )
        m_frame->SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE_OPENGL );
    else if( aEvent.IsAction( &ACTIONS::standardGraphics ) )
        m_frame->SwitchCanvas( EDA_DRAW_PANEL_GAL::GAL_TYPE_CAIRO );
    else
        wxFAIL_MSG( "Unknown canvas type" );

    return 0;
}


void COMMON_TOOLS::setTransitions()
{
    Go( &COMMON_TOOLS::SelectionTool,      ACTIONS::selectionTool.MakeEvent() );

    // Cursor control
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorUp.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorDown.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorLeft.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorRight.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorUpFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorDownFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorLeftFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorRightFast.MakeEvent() );

    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorClick.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::cursorDblClick.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,      ACTIONS::showContextMenu.MakeEvent() );

    // Pan control
    Go( &COMMON_TOOLS::PanControl,         ACTIONS::panUp.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,         ACTIONS::panDown.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,         ACTIONS::panLeft.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,         ACTIONS::panRight.MakeEvent() );

    // Zoom control
    Go( &COMMON_TOOLS::ZoomRedraw,         ACTIONS::zoomRedraw.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomIn.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,          ACTIONS::zoomOut.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomInCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,    ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomCenter,         ACTIONS::zoomCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomFitScreen,      ACTIONS::zoomFitScreen.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomPreset,         ACTIONS::zoomPreset.MakeEvent() );
    Go( &COMMON_TOOLS::CenterContents,     ACTIONS::centerContents.MakeEvent() );

    // Grid control
    Go( &COMMON_TOOLS::GridNext,           ACTIONS::gridNext.MakeEvent() );
    Go( &COMMON_TOOLS::GridPrev,           ACTIONS::gridPrev.MakeEvent() );
    Go( &COMMON_TOOLS::GridPreset,         ACTIONS::gridPreset.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleGrid,         ACTIONS::toggleGrid.MakeEvent() );
    Go( &COMMON_TOOLS::GridProperties,     ACTIONS::gridProperties.MakeEvent() );

    // Units and coordinates
    Go( &COMMON_TOOLS::ImperialUnits,      ACTIONS::imperialUnits.MakeEvent() );
    Go( &COMMON_TOOLS::MetricUnits,        ACTIONS::metricUnits.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleUnits,        ACTIONS::toggleUnits.MakeEvent() );
    Go( &COMMON_TOOLS::TogglePolarCoords,  ACTIONS::togglePolarCoords.MakeEvent() );
    Go( &COMMON_TOOLS::ResetLocalCoords,   ACTIONS::resetLocalCoords.MakeEvent() );

    // Misc
    Go( &COMMON_TOOLS::ToggleCursor,       ACTIONS::toggleCursor.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleCursorStyle,  ACTIONS::toggleCursorStyle.MakeEvent() );
    Go( &COMMON_TOOLS::SwitchCanvas,       ACTIONS::acceleratedGraphics.MakeEvent() );
    Go( &COMMON_TOOLS::SwitchCanvas,       ACTIONS::standardGraphics.MakeEvent() );
}


