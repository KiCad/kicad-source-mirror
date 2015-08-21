/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2015 CERN
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

#include <wx/wx.h>
#include <wx/frame.h>
#include <wx/window.h>
#include <wx/event.h>
#include <wx/colour.h>
#include <wx/filename.h>
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

#include <boost/foreach.hpp>

#ifdef __WXDEBUG__
#include <profile.h>
#endif /* __WXDEBUG__ */

EDA_DRAW_PANEL_GAL::EDA_DRAW_PANEL_GAL( wxWindow* aParentWindow, wxWindowID aWindowId,
                                        const wxPoint& aPosition, const wxSize& aSize,
                                        GAL_TYPE aGalType ) :
    wxScrolledCanvas( aParentWindow, aWindowId, aPosition, aSize )
{
    m_parent     = aParentWindow;
    m_gal        = NULL;
    m_backend    = GAL_TYPE_NONE;
    m_view       = NULL;
    m_painter    = NULL;
    m_eventDispatcher = NULL;
    m_lostFocus  = false;

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );
    ShowScrollbars( wxSHOW_SB_ALWAYS, wxSHOW_SB_ALWAYS );
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
#ifdef USE_OSX_MAGNIFY_EVENT
        wxEVT_MAGNIFY,
#endif
        KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE
    };

    BOOST_FOREACH( wxEventType eventType, events )
    {
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::onEvent ),
                 NULL, m_eventDispatcher );
    }

    // View controls is the first in the event handler chain, so the Tool Framework operates
    // on updated viewport data.
    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    // Set up timer that prevents too frequent redraw commands
    m_refreshTimer.SetOwner( this );
    m_pendingRefresh = false;
    m_drawing = false;
    m_drawingEnabled = false;
    Connect( wxEVT_TIMER, wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onRefreshTimer ), NULL, this );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    delete m_painter;
    delete m_viewControls;
    delete m_view;
    delete m_gal;
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    m_pendingRefresh = false;

    if( m_drawing )
        return;

    m_drawing = true;

    m_viewControls->UpdateScrollbars();
    m_view->UpdateItems();
    m_gal->BeginDrawing();
    m_gal->ClearScreen( m_painter->GetSettings()->GetBackgroundColor() );

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

    wxLongLong t = wxGetLocalTimeMillis();
    wxLongLong delta = t - m_lastRefresh;

    if( delta >= MinRefreshPeriod )
    {
        ForceRefresh();
        m_pendingRefresh = true;
    }
    else
    {
        // One shot timer
        m_refreshTimer.Start( ( MinRefreshPeriod - delta ).ToLong(), true );
        m_pendingRefresh = true;
    }
}


void EDA_DRAW_PANEL_GAL::ForceRefresh()
{
    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;

#if wxCHECK_VERSION( 3, 0, 0 )
    const wxEventType eventTypes[] = { wxEVT_TOOL };
#else
    const wxEventType eventTypes[] = { wxEVT_COMMAND_MENU_SELECTED, wxEVT_COMMAND_TOOL_CLICKED };
#endif

    if( m_eventDispatcher )
    {
        BOOST_FOREACH( wxEventType type, eventTypes )
        {
            m_parent->Connect( type, wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                               NULL, m_eventDispatcher );
        }
    }
    else
    {
        BOOST_FOREACH( wxEventType type, eventTypes )
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
    m_drawing = true;
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
            new_gal = new KIGFX::OPENGL_GAL( this, this, this );
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

    return result;
}


void EDA_DRAW_PANEL_GAL::onEvent( wxEvent& aEvent )
{
    if( m_lostFocus )
    {
        SetFocus();
        m_lostFocus = false;
    }

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
}


void EDA_DRAW_PANEL_GAL::onLostFocus( wxFocusEvent& aEvent )
{
    m_lostFocus = true;
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer( wxTimerEvent& aEvent )
{
    if( !m_drawingEnabled )
    {
        if( m_gal->IsInitialized() )
        {
            m_drawing = false;
            m_pendingRefresh = true;
            Connect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), NULL, this );
            m_drawingEnabled = true;
        }
        else
        {
            // Try again soon
            m_refreshTimer.Start( 100, true );
            return;
        }
    }

    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}
