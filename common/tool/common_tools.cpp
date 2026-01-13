/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2016 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <base_screen.h>
#include <base_units.h>
#include <gal/painter.h>
#include <bitmaps.h>
#include <class_draw_panel_gal.h>
#include <dialogs/dialog_configure_paths.h>
#include <dialogs/dialog_unit_entry.h>
#include <eda_draw_frame.h>
#include <gal/graphics_abstraction_layer.h>
#include <id.h>
#include <math/vector2wx.h>
#include <core/kicad_algo.h>
#include <kiface_base.h>
#include <settings/app_settings.h>
#include <tool/actions.h>
#include <tool/common_tools.h>
#include <tool/tool_manager.h>
#include <tool/selection_tool.h>
#include <tool/grid_helper.h>
#include <view/view.h>
#include <view/view_controls.h>
#include "macros.h"


COMMON_TOOLS::COMMON_TOOLS() :
    TOOL_INTERACTIVE( "common.Control" ),
    m_frame( nullptr ),
    m_imperialUnit( EDA_UNITS::INCH ),
    m_metricUnit( EDA_UNITS::MM )
{
}


void COMMON_TOOLS::Reset( RESET_REASON aReason )
{
    m_frame = getEditFrame<EDA_DRAW_FRAME>();
    m_grids.clear();

    if( aReason == RESET_REASON::SHUTDOWN )
        return;

    GRID_SETTINGS& settings = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid;

    // Protect against misconfigured settings with no grids
    if( settings.grids.empty() )
        settings.grids = m_frame->config()->DefaultGridSizeList();

    EDA_IU_SCALE   scale = m_frame->GetIuScale();

    for( GRID& gridDef : settings.grids )
    {
        double gridSizeX = EDA_UNIT_UTILS::UI::DoubleValueFromString( scale, EDA_UNITS::MM, gridDef.x );
        double gridSizeY = EDA_UNIT_UTILS::UI::DoubleValueFromString( scale, EDA_UNITS::MM, gridDef.y );

        m_grids.emplace_back( KiROUND<double, int>( gridSizeX ), KiROUND<double, int>( gridSizeY ) );
    }

    OnGridChanged( false );
}


void COMMON_TOOLS::SetLastUnits( EDA_UNITS aUnit )
{
    if( EDA_UNIT_UTILS::IsImperialUnit( aUnit ) )
        m_imperialUnit = aUnit;
    else if( EDA_UNIT_UTILS::IsMetricUnit( aUnit ) )
        m_metricUnit = aUnit;
    else
        wxASSERT_MSG( false, wxS( "Invalid unit" ) );
}


int COMMON_TOOLS::SelectionTool( const TOOL_EVENT& aEvent )
{
    // Since selection tools are run permanently underneath the toolStack, this is really
    // just a cancel of whatever other tools might be running.

    m_toolMgr->ProcessEvent( TOOL_EVENT( TC_COMMAND, TA_CANCEL_TOOL ) );
    return 0;
}


int COMMON_TOOLS::CursorControl( const TOOL_EVENT& aEvent )
{
    ACTIONS::CURSOR_EVENT_TYPE   type = aEvent.Parameter<ACTIONS::CURSOR_EVENT_TYPE>();
    std::unique_ptr<GRID_HELPER> grid = m_frame->MakeGridHelper();
    VECTOR2D                     gridSize;

    if( grid )
        gridSize = grid->GetGridSize( grid->GetSelectionGrid( m_frame->GetCurrentSelection() ) );
    else
        gridSize = getView()->GetGAL()->GetGridSize();

    bool     mirroredX = getView()->IsMirroredX();
    VECTOR2D cursor = getViewControls()->GetRawCursorPosition( false );

    switch( type )
    {
    case ACTIONS::CURSOR_UP_FAST:
        gridSize *= 10;
        KI_FALLTHROUGH;
    case ACTIONS::CURSOR_UP:
        cursor -= VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_DOWN_FAST:
        gridSize *= 10;
        KI_FALLTHROUGH;
    case ACTIONS::CURSOR_DOWN:
        cursor += VECTOR2D( 0, gridSize.y );
        break;

    case ACTIONS::CURSOR_LEFT_FAST:
        gridSize *= 10;
        KI_FALLTHROUGH;
    case ACTIONS::CURSOR_LEFT:
        cursor -= VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    case ACTIONS::CURSOR_RIGHT_FAST:
        gridSize *= 10;
        KI_FALLTHROUGH;
    case ACTIONS::CURSOR_RIGHT:
        cursor += VECTOR2D( mirroredX ? -gridSize.x : gridSize.x, 0 );
        break;

    case ACTIONS::CURSOR_CLICK:              // fall through
    case ACTIONS::CURSOR_DBL_CLICK:
    case ACTIONS::CURSOR_RIGHT_CLICK:
    {
        TOOL_ACTIONS       action = TA_MOUSE_CLICK;
        TOOL_MOUSE_BUTTONS button = BUT_LEFT;
        int                modifiers = 0;

        modifiers |= wxGetKeyState( WXK_SHIFT ) ? MD_SHIFT : 0;
        modifiers |= wxGetKeyState( WXK_CONTROL ) ? MD_CTRL : 0;
        modifiers |= wxGetKeyState( WXK_ALT ) ? MD_ALT : 0;

        if( type == ACTIONS::CURSOR_DBL_CLICK )
            action = TA_MOUSE_DBLCLICK;
        else if( type == ACTIONS::CURSOR_RIGHT_CLICK )
            button = BUT_RIGHT;

        TOOL_EVENT evt( TC_MOUSE, action, static_cast<int>( button | modifiers ) );
        evt.SetParameter( type );
        evt.SetMousePosition( getViewControls()->GetMousePosition() );
        m_toolMgr->ProcessEvent( evt );

        return 0;
    }
    default:
        wxFAIL_MSG( wxS( "CursorControl(): unexpected request" ) );
    }

    getViewControls()->SetCursorPosition( cursor, true, true, type );
    m_toolMgr->PostAction( ACTIONS::refreshPreview );

    return 0;
}


int COMMON_TOOLS::PanControl( const TOOL_EVENT& aEvent )
{
    ACTIONS::CURSOR_EVENT_TYPE type = aEvent.Parameter<ACTIONS::CURSOR_EVENT_TYPE>();
    KIGFX::VIEW* view = getView();
    VECTOR2D center = view->GetCenter();
    VECTOR2D gridSize = getView()->GetGAL()->GetGridSize() * 10;
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
    double zoom = getView()->GetGAL()->GetZoomFactor();

    // Step must be AT LEAST 1.3
    if( aDirection )
        zoom *= 1.3;
    else
        zoom /= 1.3;

    // Now look for the next closest menu step
    std::vector<double>& zoomList = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->zoom_factors;
    int idx;

    if( aDirection )
    {
        for( idx = 0; idx < int( zoomList.size() ); ++idx )
        {
            if( zoomList[idx] >= zoom )
                break;
        }

        if( idx >= int( zoomList.size() ) )
            idx = (int) zoomList.size() - 1;        // if we ran off the end then peg to the end
    }
    else
    {
        for( idx = int( zoomList.size() ) - 1; idx >= 0; --idx )
        {
            if( zoomList[idx] <= zoom )
                break;
        }

        if( idx < 0 )
            idx = 0;        // if we ran off the end then peg to the end
    }

    // Note: idx == 0 is Auto; idx == 1 is first entry in zoomList
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
    return doZoomFit( ZOOM_FIT_ALL );
}


int COMMON_TOOLS::ZoomFitObjects( const TOOL_EVENT& aEvent )
{
    return doZoomFit( ZOOM_FIT_OBJECTS );
}


int COMMON_TOOLS::ZoomFitSelection( const TOOL_EVENT& aEvent )
{
    return doZoomFit( ZOOM_FIT_SELECTION );
}


int COMMON_TOOLS::doZoomFit( ZOOM_FIT_TYPE_T aFitType )
{
    KIGFX::VIEW*        view   = getView();
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();
    EDA_DRAW_FRAME*     frame  = getEditFrame<EDA_DRAW_FRAME>();

    BOX2I    bBox = frame->GetDocumentExtents();
    BOX2I    defaultBox = canvas->GetDefaultViewBBox();

    view->SetScale( 1.0 );  // The best scale will be determined later, but this initial
                            // value ensures all view parameters are up to date (especially
                            // at init time)
    VECTOR2D screenSize = view->ToWorld( ToVECTOR2I( canvas->GetClientSize() ), false );

    // Currently "Zoom to Objects" is only supported in Eeschema & Pcbnew.  Support for other
    // programs in the suite can be added as needed.

    if( aFitType == ZOOM_FIT_ALL )
    {
        if( frame->IsType( FRAME_PCB_EDITOR ) )
            bBox = m_frame->GetDocumentExtents( false );
    }

    if( aFitType == ZOOM_FIT_OBJECTS )
    {
        if( frame->IsType( FRAME_SCH ) )
            bBox = m_frame->GetDocumentExtents( false );
        else
            aFitType = ZOOM_FIT_ALL; // Just do a "Zoom to Fit" for unsupported editors
    }

    if( aFitType == ZOOM_FIT_SELECTION )
    {
        SELECTION& selection = m_frame->GetCurrentSelection();

        if( selection.Empty() )
            return 0;

        bBox = selection.GetBoundingBox();
    }

    // If the screen is empty then use the default view bbox

    if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
        bBox = defaultBox;

    VECTOR2D vsize = bBox.GetSize();
    double scale = view->GetScale() / std::max( fabs( vsize.x / screenSize.x ),
                                                fabs( vsize.y / screenSize.y ) );

    // if the scale isn't finite (most likely due to an empty canvas)
    // simply just make sure we are centered and quit out of trying to zoom to fit
    if( !std::isfinite( scale ) )
    {
        view->SetCenter( VECTOR2D( 0, 0 ) );
        canvas->Refresh();
        return 0;
    }

    // Reserve enough margin to limit the amount of the view that might be obscured behind the
    // infobar.
    double margin_scale_factor = 1.04;

    if( canvas->GetClientSize().y < 768 )
        margin_scale_factor = 1.10;

    if( aFitType == ZOOM_FIT_ALL )
    {
        // Leave a bigger margin for library editors & viewers

        if( frame->IsType( FRAME_FOOTPRINT_VIEWER )
                || frame->IsType( FRAME_SCH_VIEWER ) )
        {
            margin_scale_factor = 1.30;
        }
        else if( frame->IsType( FRAME_SCH_SYMBOL_EDITOR )
                || frame->IsType( FRAME_FOOTPRINT_EDITOR ) )
        {
            margin_scale_factor = 1.48;
        }
    }

    view->SetScale( scale / margin_scale_factor );
    view->SetCenter( bBox.Centre() );
    canvas->Refresh();

    return 0;
}


int COMMON_TOOLS::CenterSelection( const TOOL_EVENT& aEvent )
{
    return doCenter( CENTER_TYPE::CENTER_SELECTION );
}


int COMMON_TOOLS::CenterContents( const TOOL_EVENT& aEvent )
{
    return doCenter( CENTER_TYPE::CENTER_CONTENTS );
}


int COMMON_TOOLS::doCenter( CENTER_TYPE aCenterType )
{
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();

    BOX2I bBox;

    if( aCenterType == CENTER_TYPE::CENTER_SELECTION )
    {
        SELECTION& selection = m_frame->GetCurrentSelection();

        // No selection: do nothing
        if( selection.Empty() )
            return 0;

        bBox = selection.GetBoundingBox().Centre();
    }
    else
    {
        bBox = getModel<EDA_ITEM>()->ViewBBox();

        if( bBox.GetWidth() == 0 || bBox.GetHeight() == 0 )
            bBox = canvas->GetDefaultViewBBox();
    }

    getView()->SetCenter( bBox.Centre() );

    // Take scrollbars into account
    VECTOR2D scrollbarSize = VECTOR2D( ToVECTOR2D( canvas->GetSize() - canvas->GetClientSize() ) );
    VECTOR2D worldScrollbarSize = getView()->ToWorld( scrollbarSize, false );
    getView()->SetCenter( getView()->GetCenter() + worldScrollbarSize / 2.0 );
    canvas->Refresh();

    return 0;
}


int COMMON_TOOLS::ZoomPreset( const TOOL_EVENT& aEvent )
{
    int idx = aEvent.Parameter<int>();
    return doZoomToPreset( idx, false );
}


// Note: idx == 0 is Auto; idx == 1 is first entry in zoomList
int COMMON_TOOLS::doZoomToPreset( int idx, bool aCenterOnCursor )
{
    std::vector<double>& zoomList = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->zoom_factors;

    if( idx == 0 )      // Zoom Auto
    {
        TOOL_EVENT dummy;
        return ZoomFitScreen( dummy );
    }
    else
    {
        idx--;
    }

    double scale = zoomList[idx];

    if( aCenterOnCursor )
    {
        getView()->SetScale( scale, getViewControls()->GetCursorPosition() );

        if( getViewControls()->IsCursorWarpingEnabled() )
            getViewControls()->CenterOnCursor();
    }
    else
    {
        getView()->SetScale( scale );
    }

    m_frame->GetCanvas()->Refresh();

    return 0;
}


int COMMON_TOOLS::GridNext( const TOOL_EVENT& aEvent )
{
    int& currentGrid = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.last_size_idx;

    currentGrid++;

    if( currentGrid >= int( m_grids.size() ) )
        currentGrid = 0;

    return OnGridChanged( true );
}


int COMMON_TOOLS::GridPrev( const TOOL_EVENT& aEvent )
{
    int& currentGrid = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.last_size_idx;

    currentGrid--;

    if( currentGrid < 0 )
        currentGrid = (int) m_grids.size() - 1;

    return OnGridChanged( true );
}


int COMMON_TOOLS::GridPreset( const TOOL_EVENT& aEvent )
{
    return GridPreset( aEvent.Parameter<int>(), false );
}


int COMMON_TOOLS::GridPreset( int idx, bool aFromHotkey )
{
    int& currentGrid = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.last_size_idx;

    currentGrid = std::clamp( idx, 0, (int) m_grids.size() - 1 );

    return OnGridChanged( aFromHotkey );
}


int COMMON_TOOLS::OnGridChanged( bool aFromHotkey )
{
    int& currentGrid = m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.last_size_idx;

    currentGrid = std::max( 0, std::min( currentGrid, static_cast<int>( m_grids.size() ) - 1 ) );

    // Update the combobox (if any)
    wxUpdateUIEvent dummy;
    m_frame->OnUpdateSelectGrid( dummy );

    // Update GAL canvas from screen
    getView()->GetGAL()->SetGridSize( m_grids[ currentGrid ] );
    getView()->GetGAL()->SetGridVisibility( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.show );
    getView()->MarkTargetDirty( KIGFX::TARGET_NONCACHED );

    // Put cursor on new grid
    VECTOR2D gridCursor = getViewControls()->GetCursorPosition( true );
    getViewControls()->SetCrossHairCursorPosition( gridCursor, false );

    // Show feedback
    if( aFromHotkey )
        m_toolMgr->PostEvent( EVENTS::GridChangedByKeyEvent );

    return 0;
}


int COMMON_TOOLS::GridFast1( const TOOL_EVENT& aEvent )
{
    return GridPreset( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.fast_grid_1, true );
}


int COMMON_TOOLS::GridFast2( const TOOL_EVENT& aEvent )
{
    return GridPreset( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.fast_grid_2, true );
}


int COMMON_TOOLS::GridFastCycle( const TOOL_EVENT& aEvent )
{
    if( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.last_size_idx
        == m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.fast_grid_1 )
    {
        return GridPreset( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.fast_grid_2, true );
    }

    return GridPreset( m_frame->GetWindowSettings( m_toolMgr->GetSettings() )->grid.fast_grid_1, true );
}


int COMMON_TOOLS::ToggleGrid( const TOOL_EVENT& aEvent )
{
    m_frame->SetGridVisibility( !m_frame->IsGridVisible() );
    return 0;
}


int COMMON_TOOLS::ToggleGridOverrides( const TOOL_EVENT& aEvent )
{
    m_frame->SetGridOverrides( !m_frame->IsGridOverridden() );
    return 0;
}


int COMMON_TOOLS::GridProperties( const TOOL_EVENT& aEvent )
{
    auto showGridPrefs =
            [this]( const wxString& aParentName )
            {
                m_frame->CallAfter(
                        [this, aParentName]()
                        {
                            m_frame->ShowPreferences( _( "Grids" ), aParentName );
                        } );
            };

    switch( m_frame->GetFrameType() )
    {
    case FRAME_SCH:               showGridPrefs( _( "Schematic Editor" ) );     break;
    case FRAME_SCH_SYMBOL_EDITOR: showGridPrefs( _( "Symbol Editor" ) );        break;
    case FRAME_PCB_EDITOR:        showGridPrefs( _( "PCB Editor" ) );           break;
    case FRAME_FOOTPRINT_EDITOR:  showGridPrefs( _( "Footprint Editor" ) );     break;
    case FRAME_FOOTPRINT_VIEWER:  showGridPrefs( _( "Footprint Browser" ) );    break;
    case FRAME_PL_EDITOR:         showGridPrefs( _( "Drawing Sheet Editor" ) ); break;
    case FRAME_GERBER:            showGridPrefs( _( "Gerber Viewer" ) );        break;
    default:                      wxFAIL_MSG( "Unknown frame: " + GetName() );  break;
    }

    return 0;
}


int COMMON_TOOLS::GridOrigin( const TOOL_EVENT& aEvent )
{
    VECTOR2I           origin = m_frame->GetGridOrigin();
    WX_PT_ENTRY_DIALOG dlg( m_frame, _( "Grid Origin" ), _( "X:" ), _( "Y:" ), origin, true );

    if( dlg.ShowModal() == wxID_OK )
    {
        m_frame->SetGridOrigin( dlg.GetValue() );

        m_toolMgr->ResetTools( TOOL_BASE::REDRAW );
        m_toolMgr->RunAction( ACTIONS::gridSetOrigin, new VECTOR2D( m_frame->GetGridOrigin() ) );
        m_frame->GetCanvas()->ForceRefresh();
    }

    return 0;
}


int COMMON_TOOLS::SwitchUnits( const TOOL_EVENT& aEvent )
{
    EDA_UNITS newUnit = aEvent.Parameter<EDA_UNITS>();

    if( EDA_UNIT_UTILS::IsMetricUnit( newUnit ) )
        m_metricUnit = newUnit;
    else if( EDA_UNIT_UTILS::IsImperialUnit( newUnit ) )
        m_imperialUnit = newUnit;
    else
        wxASSERT_MSG( false, wxS( "Invalid unit for the frame" ) );

    m_frame->ChangeUserUnits( newUnit );
    return 0;
}


int COMMON_TOOLS::ToggleUnits( const TOOL_EVENT& aEvent )
{
    m_frame->ChangeUserUnits( EDA_UNIT_UTILS::IsImperialUnit( m_frame->GetUserUnits() ) ?
                                      m_metricUnit :
                                      m_imperialUnit );
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
    if( !m_frame->GetScreen() )     // Can happen in footprint chooser frame
        return 0;

    const KIGFX::VC_SETTINGS& vcSettings = m_toolMgr->GetCurrentToolVC();

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
    galOpts.WriteConfig( *m_frame->GetWindowSettings( m_toolMgr->GetSettings() ) );
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::CursorSmallCrosshairs( const TOOL_EVENT& aEvent )
{
    GAL_DISPLAY_OPTIONS_IMPL& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.SetCursorMode( KIGFX::CROSS_HAIR_MODE::SMALL_CROSS );
    galOpts.WriteConfig( *m_frame->GetWindowSettings( m_toolMgr->GetSettings() ) );
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::CursorFullCrosshairs( const TOOL_EVENT& aEvent )
{
    GAL_DISPLAY_OPTIONS_IMPL& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.SetCursorMode( KIGFX::CROSS_HAIR_MODE::FULLSCREEN_CROSS );
    galOpts.WriteConfig( *m_frame->GetWindowSettings( m_toolMgr->GetSettings() ) );
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::Cursor45Crosshairs( const TOOL_EVENT& aEvent )
{
    GAL_DISPLAY_OPTIONS_IMPL& galOpts = m_frame->GetGalDisplayOptions();

    galOpts.SetCursorMode( KIGFX::CROSS_HAIR_MODE::FULLSCREEN_DIAGONAL );
    galOpts.WriteConfig( *m_frame->GetWindowSettings( m_toolMgr->GetSettings() ) );
    galOpts.NotifyChanged();

    return 0;
}


int COMMON_TOOLS::ToggleBoundingBoxes( const TOOL_EVENT& aEvent )
{
    EDA_DRAW_PANEL_GAL* canvas = m_frame->GetCanvas();

    if( canvas )
    {
        KIGFX::RENDER_SETTINGS* rs = canvas->GetView()->GetPainter()->GetSettings();

        rs->SetDrawBoundingBoxes( !rs->GetDrawBoundingBoxes() );

        canvas->GetView()->UpdateAllItems( KIGFX::ALL );
        canvas->ForceRefresh();
    }

    return 0;
}


void COMMON_TOOLS::setTransitions()
{
    Go( &COMMON_TOOLS::SelectionTool,       ACTIONS::selectionTool.MakeEvent() );

    // Cursor control
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorUp.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorDown.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorLeft.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorRight.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorUpFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorDownFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorLeftFast.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorRightFast.MakeEvent() );

    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorClick.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::cursorDblClick.MakeEvent() );
    Go( &COMMON_TOOLS::CursorControl,       ACTIONS::showContextMenu.MakeEvent() );

    // Pan control
    Go( &COMMON_TOOLS::PanControl,          ACTIONS::panUp.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,          ACTIONS::panDown.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,          ACTIONS::panLeft.MakeEvent() );
    Go( &COMMON_TOOLS::PanControl,          ACTIONS::panRight.MakeEvent() );

    // Zoom control
    Go( &COMMON_TOOLS::ZoomRedraw,          ACTIONS::zoomRedraw.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,           ACTIONS::zoomIn.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOut,           ACTIONS::zoomOut.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,     ACTIONS::zoomInCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomInOutCenter,     ACTIONS::zoomOutCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomCenter,          ACTIONS::zoomCenter.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomFitScreen,       ACTIONS::zoomFitScreen.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomFitObjects,      ACTIONS::zoomFitObjects.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomFitSelection,    ACTIONS::zoomFitSelection.MakeEvent() );
    Go( &COMMON_TOOLS::ZoomPreset,          ACTIONS::zoomPreset.MakeEvent() );
    Go( &COMMON_TOOLS::CenterContents,      ACTIONS::centerContents.MakeEvent() );
    Go( &COMMON_TOOLS::CenterSelection,     ACTIONS::centerSelection.MakeEvent() );

    // Grid control
    Go( &COMMON_TOOLS::GridNext,            ACTIONS::gridNext.MakeEvent() );
    Go( &COMMON_TOOLS::GridPrev,            ACTIONS::gridPrev.MakeEvent() );
    Go( &COMMON_TOOLS::GridPreset,          ACTIONS::gridPreset.MakeEvent() );
    Go( &COMMON_TOOLS::GridFast1,           ACTIONS::gridFast1.MakeEvent() );
    Go( &COMMON_TOOLS::GridFast2,           ACTIONS::gridFast2.MakeEvent() );
    Go( &COMMON_TOOLS::GridFastCycle,       ACTIONS::gridFastCycle.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleGrid,          ACTIONS::toggleGrid.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleGridOverrides, ACTIONS::toggleGridOverrides.MakeEvent() );
    Go( &COMMON_TOOLS::GridProperties,      ACTIONS::gridProperties.MakeEvent() );
    Go( &COMMON_TOOLS::GridOrigin,          ACTIONS::gridOrigin.MakeEvent() );

    // Units and coordinates
    Go( &COMMON_TOOLS::SwitchUnits,         ACTIONS::inchesUnits.MakeEvent() );
    Go( &COMMON_TOOLS::SwitchUnits,         ACTIONS::milsUnits.MakeEvent() );
    Go( &COMMON_TOOLS::SwitchUnits,         ACTIONS::millimetersUnits.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleUnits,         ACTIONS::toggleUnits.MakeEvent() );
    Go( &COMMON_TOOLS::TogglePolarCoords,   ACTIONS::togglePolarCoords.MakeEvent() );
    Go( &COMMON_TOOLS::ResetLocalCoords,    ACTIONS::resetLocalCoords.MakeEvent() );

    // Misc
    Go( &COMMON_TOOLS::ToggleCursor,        ACTIONS::toggleCursor.MakeEvent() );
    Go( &COMMON_TOOLS::CursorSmallCrosshairs,   ACTIONS::cursorSmallCrosshairs.MakeEvent() );
    Go( &COMMON_TOOLS::CursorFullCrosshairs,   ACTIONS::cursorFullCrosshairs.MakeEvent() );
    Go( &COMMON_TOOLS::Cursor45Crosshairs,   ACTIONS::cursor45Crosshairs.MakeEvent() );
    Go( &COMMON_TOOLS::ToggleBoundingBoxes, ACTIONS::toggleBoundingBoxes.MakeEvent() );
}
