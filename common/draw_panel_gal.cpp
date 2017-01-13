/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2017 CERN
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

#include <draw_frame.h>
#include <kiface_i.h>
#include <confirm.h>

#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/wx_view_controls.h>
#include <pcb_painter.h>

#include <gal/graphics_abstraction_layer.h>
#include <gal/opengl/opengl_gal.h>
#include <gal/cairo/cairo_gal.h>

#include <tool/tool_dispatcher.h>
#include <tool/tool_manager.h>

#include <pcbstruct.h>  // display options definition

#ifdef PROFILE
#include <profile.h>
#endif /* PROFILE */


EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        KIGFX::GAL_DISPLAY_OPTIONS& aOptions, GAL_TYPE aGalType ) :
    wxScrolledCanvas( aParentWindow, aWindowId, aPosition, aSize ), m_options( aOptions )
{
    m_parent     = aParentWindow;
    m_edaFrame   = dynamic_cast<EDA_DRAW_FRAME*>( aParentWindow );
    m_gal        = NULL;
    m_backend    = GAL_TYPE_NONE;
    m_view       = NULL;
    m_painter    = NULL;
    m_eventDispatcher = NULL;
    m_lostFocus  = false;

    SetLayoutDirection( wxLayout_LeftToRight );

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

// Scrollbars broken in GAL on OSX
#ifdef __WXMAC__
    ShowScrollbars( wxSHOW_SB_NEVER, wxSHOW_SB_NEVER );
#else
    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
#endif
    EnableScrolling( false, false );    // otherwise Zoom Auto disables GAL canvas

    m_painter = new KIGFX::PCB_PAINTER( m_gal );

    m_view = new KIGFX::VIEW( true );
    m_view->SetPainter( m_painter );
    m_view->SetGAL( m_gal );

    Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_DRAW_PANEL_GAL::onSize ), NULL, this );
    Connect( wxEVT_ENTER_WINDOW, wxEventHandler( EDA_DRAW_PANEL_GAL::onEnter ), NULL, this );
    Connect( wxEVT_KILL_FOCUS, wxFocusEventHandler( EDA_DRAW_PANEL_GAL::onLostFocus ), NULL, this );

    const wxEventType events[] =
    {
        wxEVT_LEFT_UP, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_DCLICK,
        wxEVT_MOTION, wxEVT_MOUSEWHEEL, wxEVT_CHAR,
#if wxCHECK_VERSION( 3, 1, 0 ) || defined( USE_OSX_MAGNIFY_EVENT )
        wxEVT_MAGNIFY,
#endif
        KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE
    };

    for( wxEventType eventType : events )
    {
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::onEvent ),
                 NULL, m_eventDispatcher );
    }

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

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

    LoadGalSettings();
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    StopDrawing();
    SaveGalSettings();

    assert( !m_drawing );

    delete m_painter;
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
    m_pendingRefresh = false;

    if( m_drawing )
        return;

#ifdef PROFILE
    PROF_COUNTER totalRealTime;
#endif /* PROFILE */

    m_drawing = true;
    KIGFX::PCB_RENDER_SETTINGS* settings = static_cast<KIGFX::PCB_RENDER_SETTINGS*>( m_painter->GetSettings() );

// Scrollbars broken in GAL on OSX
#ifndef __WXMAC__
    m_viewControls->UpdateScrollbars();
#endif

    m_view->UpdateItems();

    try
    {
        m_gal->BeginDrawing();
        m_gal->ClearScreen( settings->GetBackgroundColor() );

        KIGFX::COLOR4D gridColor = settings->GetLayerColor( ITEM_GAL_LAYER( GRID_VISIBLE ) );
        m_gal->SetGridColor( gridColor );

        if( m_view->IsDirty() )
        {
            m_view->ClearTargets();

            // Grid has to be redrawn only when the NONCACHED target is redrawn
            if( m_view->IsTargetDirty( KIGFX::TARGET_NONCACHED ) )
                m_gal->DrawGrid();

            m_view->Redraw();
        }

        m_gal->DrawCursor( m_viewControls->GetCursorPosition() );
        m_gal->EndDrawing();
    }
    catch( std::runtime_error& err )
    {
        assert( GetBackend() != GAL_TYPE_CAIRO );

        // Cairo is supposed to be the safe backend, there is not a single "throw" in its code
        SwitchBackend( GAL_TYPE_CAIRO );

        if( m_edaFrame )
            m_edaFrame->UseGalCanvas( true );

        DisplayError( m_parent, wxString( err.what() ) );
    }

#ifdef PROFILE
    totalRealTime.Stop();
    wxLogDebug( "EDA_DRAW_PANEL_GAL::onPaint(): %.1f ms", totalRealTime.msecs() );
#endif /* PROFILE */

    m_lastRefresh = wxGetLocalTimeMillis();
    m_drawing = false;
}


void EDA_DRAW_PANEL_GAL::onSize( wxSizeEvent& aEvent )
{
    m_gal->ResizeScreen( aEvent.GetSize().x, aEvent.GetSize().y );
    m_view->MarkTargetDirty( KIGFX::TARGET_CACHED );
    m_view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
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


void EDA_DRAW_PANEL_GAL::SetHighContrastLayer( LAYER_ID aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearActiveLayers();
    rSettings->SetActiveLayer( aLayer );

    m_view->UpdateAllLayersColor();
}


void EDA_DRAW_PANEL_GAL::SetTopLayer( LAYER_ID aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );
    m_view->UpdateAllLayersOrder();
}


double EDA_DRAW_PANEL_GAL::GetLegacyZoom() const
{
    double zoomFactor = m_gal->GetWorldScale() / m_gal->GetZoomFactor();
    return ( 1.0 / ( zoomFactor * m_view->GetScale() ) );
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
            new_gal = new KIGFX::OPENGL_GAL( m_options, this, this, this );
            break;

        case GAL_TYPE_CAIRO:
            new_gal = new KIGFX::CAIRO_GAL( this, this, this );
            break;

        default:
            assert( false );
            // warn about unhandled GAL canvas type, but continue with the fallback option

        case GAL_TYPE_NONE:
            // KIGFX::GAL is a stub - it actually does cannot display anything,
            // but prevents code relying on GAL canvas existence from crashing
            new_gal = new KIGFX::GAL();
            break;
        }
    }
    catch( std::runtime_error& err )
    {
        new_gal = new KIGFX::GAL();
        aGalType = GAL_TYPE_NONE;
        DisplayError( m_parent, wxString( err.what() ) );
        result = false;
    }

    SaveGalSettings();

    assert( new_gal );
    delete m_gal;
    m_gal = new_gal;

    wxSize size = GetClientSize();
    m_gal->ResizeScreen( size.GetX(), size.GetY() );

    if( m_painter )
        m_painter->SetGAL( m_gal );

    if( m_view )
        m_view->SetGAL( m_gal );

    m_backend = aGalType;
    LoadGalSettings();

    return result;
}


bool EDA_DRAW_PANEL_GAL::SaveGalSettings()
{
    if( !m_edaFrame || !m_gal )
        return false;

    wxConfigBase* cfg = Kiface().KifaceSettings();
    wxString baseCfgName = m_edaFrame->GetName();

    if( !cfg )
        return false;

    if( !cfg->Write( baseCfgName + GRID_STYLE_CFG, (long) GetGAL()->GetGridStyle() ) )
        return false;

    return true;
}


bool EDA_DRAW_PANEL_GAL::LoadGalSettings()
{
    if( !m_edaFrame || !m_gal )
        return false;

    wxConfigBase* cfg = Kiface().KifaceSettings();
    wxString baseCfgName = m_edaFrame->GetName();

    if( !cfg )
        return false;

    long gridStyle;
    cfg->Read( baseCfgName + GRID_STYLE_CFG, &gridStyle, (long) KIGFX::GRID_STYLE::GRID_STYLE_DOTS );
    GetGAL()->SetGridStyle( (KIGFX::GRID_STYLE) gridStyle );

    return true;
}


void EDA_DRAW_PANEL_GAL::onEvent( wxEvent& aEvent )
{
    if( m_lostFocus )
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


const wxChar EDA_DRAW_PANEL_GAL::GRID_STYLE_CFG[] = wxT( "GridStyle" );
