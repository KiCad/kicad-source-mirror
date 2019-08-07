/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
 * Copyright (C) 2013-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

#include <eda_draw_frame.h>
#include <kiface_i.h>
#include <confirm.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <painter.h>
#include <base_screen.h>
#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/cairo/cairo_gal.h>

#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>

#ifdef __WXDEBUG__
#include <profile.h>
#endif /* PROFILE */


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
    wxScrolledCanvas( aParentWindow, aWindowId, aPosition, aSize ),
    m_options( aOptions )
{
    m_parent     = aParentWindow;
    m_edaFrame   = dynamic_cast<EDA_DRAW_FRAME*>( aParentWindow );
    m_gal        = NULL;
    m_backend    = GAL_TYPE_NONE;
    m_view       = NULL;
    m_painter    = NULL;
    m_eventDispatcher = NULL;
    m_lostFocus  = false;
    m_stealsFocus = true;

    m_currentCursor = wxStockCursor( wxCURSOR_ARROW );

    SetLayoutDirection( wxLayout_LeftToRight );

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_DRAW_PANEL_GAL::onSize ), NULL, this );
    Connect( wxEVT_ENTER_WINDOW, wxEventHandler( EDA_DRAW_PANEL_GAL::onEnter ), NULL, this );
    Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( EDA_DRAW_PANEL_GAL::onLostFocus ), NULL, this );
    Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( EDA_DRAW_PANEL_GAL::onSetCursor ), NULL, this );

    const wxEventType events[] =
    {
        // Binding both EVT_CHAR and EVT_CHAR_HOOK ensures that all key events,
        // especially special key like arrow keys, are handled by the GAL event dispatcher,
        // and not sent to GUI without filtering, because they have a default action (scroll)
        // that must not be called.
        wxEVT_LEFT_UP, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_DCLICK,
        wxEVT_MOTION, wxEVT_MOUSEWHEEL, wxEVT_CHAR, wxEVT_CHAR_HOOK,
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
        wxEVT_MAGNIFY,
#endif
        KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE
    };

    for( wxEventType eventType : events )
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::OnEvent ), NULL, m_eventDispatcher );

    m_pendingRefresh = false;
    m_drawing = false;
    m_drawingEnabled = false;

    // Set up timer that prevents too frequent redraw commands
    m_refreshTimer.SetOwner( this );
    Connect( m_refreshTimer.GetId(), wxEVT_TIMER,
            wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onRefreshTimer ), NULL, this );

    // Set up timer to execute OnShow() method when the window appears on the screen
    m_onShowTimer.SetOwner( this );
    Connect( m_onShowTimer.GetId(), wxEVT_TIMER,
            wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onShowTimer ), NULL, this );
    m_onShowTimer.Start( 10 );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    StopDrawing();

    wxASSERT( !m_drawing );

    delete m_viewControls;
    delete m_view;
    delete m_gal;
}


void EDA_DRAW_PANEL_GAL::SetFocus()
{
// Windows has a strange manner on bringing up and activating windows
// containing a GAL canvas just after moving the mouse cursor into its area.
// Feel free to uncomment or extend the following #ifdef if you experience
// similar problems on your platform.
#ifdef __WINDOWS__
    if( !GetParent()->IsDescendant( wxWindow::FindFocus() ) )
        return;
#endif

    wxScrolledCanvas::SetFocus();
    m_lostFocus = false;
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    m_viewControls->UpdateScrollbars();

    // Update current zoom settings if the canvas is managed by a EDA frame
    // (i.e. not by a preview panel in a dialog)
    if( GetParentEDAFrame() && GetParentEDAFrame()->GetScreen() )
    {
        GetParentEDAFrame()->GetScreen()->SetZoom( GetLegacyZoom() );
        GetParentEDAFrame()->GetScreen()->m_ScrollCenter = GetView()->GetCenter();
    }

    if( !m_gal->IsVisible() )
        return;

    m_pendingRefresh = false;

    if( m_drawing )
        return;

#ifdef __WXDEBUG__
    PROF_COUNTER totalRealTime;
#endif /* PROFILE */

    wxASSERT( m_painter );

    m_drawing = true;
    KIGFX::RENDER_SETTINGS* settings = static_cast<KIGFX::RENDER_SETTINGS*>( m_painter->GetSettings() );

    try
    {
        m_view->UpdateItems();

        KIGFX::GAL_DRAWING_CONTEXT ctx( m_gal );

        m_gal->SetClearColor( settings->GetBackgroundColor() );
        m_gal->SetGridColor( settings->GetGridColor() );
        m_gal->SetCursorColor( settings->GetCursorColor() );

        // TODO: find why ClearScreen() must be called here in opengl mode
        // and only if m_view->IsDirty() in Cairo mode to avoid distaly artifacts
        // when moving the mouse cursor
        if( m_backend == GAL_TYPE_OPENGL )
            m_gal->ClearScreen();

        if( m_view->IsDirty() )
        {
            if( m_backend != GAL_TYPE_OPENGL &&     // Already called in opengl
                m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                m_gal->ClearScreen();

            m_view->ClearTargets();

            // Grid has to be redrawn only when the NONCACHED target is redrawn
            if( m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                m_gal->DrawGrid();

            m_view->Redraw();
        }

        m_gal->DrawCursor( m_viewControls->GetCursorPosition() );
    }
    catch( std::runtime_error& err )
    {
        constexpr auto GAL_FALLBACK = GAL_TYPE_CAIRO;

        SwitchBackend( GAL_FALLBACK );

        DisplayInfoMessage( m_parent,
                            _( "Could not use OpenGL, falling back to software rendering" ),
                            wxString( err.what() ) );
    }

#ifdef __WXDEBUG__
    totalRealTime.Stop();
    wxLogTrace( "GAL_PROFILE", "EDA_DRAW_PANEL_GAL::onPaint(): %.1f ms", totalRealTime.msecs() );
#endif /* PROFILE */

    m_lastRefresh = wxGetLocalTimeMillis();
    m_drawing = false;
}


void EDA_DRAW_PANEL_GAL::onSize( wxSizeEvent& aEvent )
{
    KIGFX::GAL_CONTEXT_LOCKER locker( m_gal );
    wxSize clientSize = GetClientSize();
    m_gal->ResizeScreen( clientSize.x, clientSize.y );

    if( m_view )
    {
        m_view->MarkTargetDirty( KIGFX::TARGET_CACHED );
        m_view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
    }
}


void EDA_DRAW_PANEL_GAL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    if( m_pendingRefresh )
        return;

    m_pendingRefresh = true;

#ifdef __WXMAC__
    // Timers on OS X may have a high latency (seen up to 500ms and more) which
    // makes repaints jerky. No negative impact seen without throttling, so just
    // do an unconditional refresh for OS X.
    ForceRefresh();
#else
    wxLongLong t = wxGetLocalTimeMillis();
    wxLongLong delta = t - m_lastRefresh;

    if( delta >= MinRefreshPeriod )
    {
        ForceRefresh();
    }
    else
    {
        // One shot timer
        m_refreshTimer.Start( ( MinRefreshPeriod - delta ).ToLong(), true );
    }
#endif
}


void EDA_DRAW_PANEL_GAL::ForceRefresh()
{
    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;
    const wxEventType eventTypes[] = { wxEVT_TOOL };

    if( m_eventDispatcher )
    {
        for( wxEventType type : eventTypes )
        {
            m_parent->Connect( type, wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                               NULL, m_eventDispatcher );
        }
    }
    else
    {
        for( wxEventType type : eventTypes )
        {
            // While loop is used to be sure that all event handlers are removed.
            while( m_parent->Disconnect( type,
                                         wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                                         NULL, m_eventDispatcher ) );
        }
    }
}


void EDA_DRAW_PANEL_GAL::StartDrawing()
{
    // Start querying GAL if it is ready
    m_refreshTimer.StartOnce( 100 );
}


void EDA_DRAW_PANEL_GAL::StopDrawing()
{
    m_drawingEnabled = false;
    Disconnect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), NULL, this );
    m_pendingRefresh = false;
    m_refreshTimer.Stop();
}


void EDA_DRAW_PANEL_GAL::SetHighContrastLayer( int aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearActiveLayers();
    rSettings->SetActiveLayer( aLayer );

    m_view->UpdateAllLayersColor();
}


void EDA_DRAW_PANEL_GAL::SetTopLayer( int aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );
    m_view->UpdateAllLayersOrder();
}


double EDA_DRAW_PANEL_GAL::GetLegacyZoom() const
{
    return m_edaFrame->GetZoomLevelCoeff() / m_gal->GetZoomFactor();
}


bool EDA_DRAW_PANEL_GAL::SwitchBackend( GAL_TYPE aGalType )
{
    // Do not do anything if the currently used GAL is correct
    if( aGalType == m_backend && m_gal != NULL )
        return true;

    bool result = true; // assume everything will be fine

    // Prevent refreshing canvas during backend switch
    StopDrawing();

    KIGFX::GAL* new_gal = NULL;

    try
    {
        switch( aGalType )
        {
        case GAL_TYPE_OPENGL:
            try
            {
                new_gal = new KIGFX::OPENGL_GAL( m_options, this, this, this );
                break;
            }
            catch( std::runtime_error& err )
            {
                aGalType = GAL_TYPE_CAIRO;
                DisplayInfoMessage( m_parent,
                        _( "Could not use OpenGL, falling back to software rendering" ),
                        wxString( err.what() ) );
            }

            new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
            break;

        case GAL_TYPE_CAIRO:
            new_gal = new KIGFX::CAIRO_GAL( m_options, this, this, this );
            break;

        default:
            wxASSERT( false );
            // warn about unhandled GAL canvas type, but continue with the fallback option

        case GAL_TYPE_NONE:
            // KIGFX::GAL is a stub - it actually does cannot display anything,
            // but prevents code relying on GAL canvas existence from crashing
            new_gal = new KIGFX::GAL( m_options );
            break;
        }
    }
    catch( std::runtime_error& err )
    {
        // Create a dummy GAL
        new_gal = new KIGFX::GAL( m_options );
        aGalType = GAL_TYPE_NONE;
        DisplayError( m_parent, wxString( err.what() ) );
        result = false;
    }

    // trigger update of the gal options in case they differ
    // from the defaults
    m_options.NotifyChanged();

    wxWindow* galWindow = dynamic_cast<wxWindow*>( new_gal );

    if( galWindow )
        galWindow->Connect( wxEVT_SET_CURSOR, wxSetCursorEventHandler( EDA_DRAW_PANEL_GAL::onSetCursor ), NULL, this );

    delete m_gal;
    m_gal = new_gal;

    wxSize size = GetClientSize();
    m_gal->ResizeScreen( size.GetX(), size.GetY() );

    if( m_painter )
        m_painter->SetGAL( m_gal );

    if( m_view )
    {
        m_view->SetGAL( m_gal );
        // Note: OpenGL requires reverse draw order when draw priority is enabled
        m_view->ReverseDrawOrder( aGalType == GAL_TYPE_OPENGL );
    }

    m_backend = aGalType;

    return result;
}


void EDA_DRAW_PANEL_GAL::OnEvent( wxEvent& aEvent )
{
    if( m_lostFocus && m_stealsFocus )
        SetFocus();

    if( !m_eventDispatcher )
        aEvent.Skip();
    else
        m_eventDispatcher->DispatchWxEvent( aEvent );

    Refresh();
}


void EDA_DRAW_PANEL_GAL::onEnter( wxEvent& aEvent )
{
    // Getting focus is necessary in order to receive key events properly
    if( m_stealsFocus )
        SetFocus();

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onLostFocus( wxFocusEvent& aEvent )
{
    m_lostFocus = true;

    aEvent.Skip();
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer( wxTimerEvent& aEvent )
{
    if( !m_drawingEnabled )
    {
        if( m_gal && m_gal->IsInitialized() )
        {
            m_drawing = false;
            m_pendingRefresh = true;
            Connect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), NULL, this );
            m_drawingEnabled = true;
        }
        else
        {
            // Try again soon
            m_refreshTimer.StartOnce( 100 );
            return;
        }
    }

    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_DRAW_PANEL_GAL::onShowTimer( wxTimerEvent& aEvent )
{
    if( m_gal && m_gal->IsVisible() )
    {
        m_onShowTimer.Stop();
        OnShow();
    }
}


void EDA_DRAW_PANEL_GAL::SetCurrentCursor( wxStockCursor aStockCursorID )
{
    if ( aStockCursorID <= wxCURSOR_NONE || aStockCursorID >= wxCURSOR_MAX )
        aStockCursorID = wxCURSOR_ARROW;

    SetCurrentCursor( wxCursor( aStockCursorID ) );
}


void EDA_DRAW_PANEL_GAL::SetCurrentCursor( const wxCursor& aCursor )
{
    m_currentCursor = aCursor;
    SetCursor( m_currentCursor );
}


void EDA_DRAW_PANEL_GAL::onSetCursor( wxSetCursorEvent& event )
{
    event.SetCursor( m_currentCursor );
}
