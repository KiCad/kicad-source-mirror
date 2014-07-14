/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2013-2014 CERN
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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
                                        GalType aGalType ) :
    wxWindow( aParentWindow, aWindowId, aPosition, aSize )
{
    m_parent     = aParentWindow;
    m_gal        = NULL;
    m_backend    = GAL_TYPE_NONE;
    m_view       = NULL;
    m_painter    = NULL;
    m_eventDispatcher = NULL;

    SwitchBackend( aGalType );
    SetBackgroundStyle( wxBG_STYLE_CUSTOM );

    m_painter = new KIGFX::PCB_PAINTER( m_gal );

    m_view = new KIGFX::VIEW( true );
    m_view->SetPainter( m_painter );
    m_view->SetGAL( m_gal );

    m_viewControls = new KIGFX::WX_VIEW_CONTROLS( m_view, this );

    Connect( wxEVT_SIZE, wxSizeEventHandler( EDA_DRAW_PANEL_GAL::onSize ), NULL, this );
    Connect( wxEVT_ENTER_WINDOW, wxEventHandler( EDA_DRAW_PANEL_GAL::onEnter ), NULL, this );

    const wxEventType events[] =
    {
        wxEVT_LEFT_UP, wxEVT_LEFT_DOWN, wxEVT_LEFT_DCLICK,
        wxEVT_RIGHT_UP, wxEVT_RIGHT_DOWN, wxEVT_RIGHT_DCLICK,
        wxEVT_MIDDLE_UP, wxEVT_MIDDLE_DOWN, wxEVT_MIDDLE_DCLICK,
        wxEVT_MOTION, wxEVT_MOUSEWHEEL, wxEVT_CHAR, KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE
    };

    BOOST_FOREACH( wxEventType eventType, events )
    {
        Connect( eventType, wxEventHandler( EDA_DRAW_PANEL_GAL::onEvent ),
                 NULL, m_eventDispatcher );
    }

    // Set up timer that prevents too frequent redraw commands
    m_refreshTimer.SetOwner( this );
    m_pendingRefresh = false;
    m_drawing = false;
    Connect( wxEVT_TIMER, wxTimerEventHandler( EDA_DRAW_PANEL_GAL::onRefreshTimer ), NULL, this );
}


EDA_DRAW_PANEL_GAL::~EDA_DRAW_PANEL_GAL()
{
    if( m_painter )
        delete m_painter;

    if( m_viewControls )
        delete m_viewControls;

    if( m_view )
        delete m_view;

    if( m_gal )
        delete m_gal;
}


void EDA_DRAW_PANEL_GAL::onPaint( wxPaintEvent& WXUNUSED( aEvent ) )
{
    m_pendingRefresh = false;
    m_lastRefresh = wxGetLocalTimeMillis();

    if( !m_drawing )
    {
        m_drawing = true;

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

        m_drawing = false;
    }
}


void EDA_DRAW_PANEL_GAL::onSize( wxSizeEvent& aEvent )
{
    m_gal->ResizeScreen( aEvent.GetSize().x, aEvent.GetSize().y );
    m_view->MarkTargetDirty( KIGFX::TARGET_CACHED );
    m_view->MarkTargetDirty( KIGFX::TARGET_NONCACHED );
}


void EDA_DRAW_PANEL_GAL::onRefreshTimer( wxTimerEvent& aEvent )
{
    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_DRAW_PANEL_GAL::Refresh( bool aEraseBackground, const wxRect* aRect )
{
    if( m_pendingRefresh )
        return;

    wxLongLong t = wxGetLocalTimeMillis();
    wxLongLong delta = t - m_lastRefresh;

    if( delta >= MinRefreshPeriod )
    {
        wxPaintEvent redrawEvent;
        wxPostEvent( this, redrawEvent );
        m_pendingRefresh = true;
    }
    else
    {
        // One shot timer
        m_refreshTimer.Start( ( MinRefreshPeriod - delta ).ToLong(), true );
        m_pendingRefresh = true;
    }
}


void EDA_DRAW_PANEL_GAL::SetEventDispatcher( TOOL_DISPATCHER* aEventDispatcher )
{
    m_eventDispatcher = aEventDispatcher;

#if wxCHECK_VERSION( 3, 0, 0 )
    if( m_eventDispatcher )
    {
        m_parent->Connect( wxEVT_TOOL,
                           wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                           NULL, m_eventDispatcher );
    }
    else
    {
        // While loops are used to be sure, that we are removing all event handlers
        while( m_parent->Disconnect( wxEVT_TOOL,
                                     wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                                     NULL, m_eventDispatcher ) );
    }
#else
    if( m_eventDispatcher )
    {
        m_parent->Connect( wxEVT_COMMAND_MENU_SELECTED,
                           wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                           NULL, m_eventDispatcher );

        m_parent->Connect( wxEVT_COMMAND_TOOL_CLICKED,
                           wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                           NULL, m_eventDispatcher );
    }
    else
    {
        // While loops are used to be sure, that we are removing all event handlers
        while( m_parent->Disconnect( wxEVT_COMMAND_MENU_SELECTED,
                                     wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                                     NULL, m_eventDispatcher ) );

        while( m_parent->Disconnect( wxEVT_COMMAND_TOOL_CLICKED,
                                     wxCommandEventHandler( TOOL_DISPATCHER::DispatchWxCommand ),
                                     NULL, m_eventDispatcher ) );
    }
#endif
}


void EDA_DRAW_PANEL_GAL::StartDrawing()
{
    m_pendingRefresh = false;
    Connect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), NULL, this );

    wxPaintEvent redrawEvent;
    wxPostEvent( this, redrawEvent );
}


void EDA_DRAW_PANEL_GAL::StopDrawing()
{
    m_pendingRefresh = true;
    m_refreshTimer.Stop();
    Disconnect( wxEVT_PAINT, wxPaintEventHandler( EDA_DRAW_PANEL_GAL::onPaint ), NULL, this );
}


void EDA_DRAW_PANEL_GAL::SetHighContrastLayer( LAYER_NUM aLayer )
{
    // Set display settings for high contrast mode
    KIGFX::RENDER_SETTINGS* rSettings = m_view->GetPainter()->GetSettings();

    SetTopLayer( aLayer );

    rSettings->ClearActiveLayers();
    rSettings->SetActiveLayer( aLayer );

    m_view->UpdateAllLayersColor();
}


void EDA_DRAW_PANEL_GAL::SetTopLayer( LAYER_NUM aLayer )
{
    m_view->ClearTopLayers();
    m_view->SetTopLayer( aLayer );
    m_view->UpdateAllLayersOrder();
}


void EDA_DRAW_PANEL_GAL::SwitchBackend( GalType aGalType )
{
    // Do not do anything if the currently used GAL is correct
    if( aGalType == m_backend && m_gal != NULL )
        return;

    // Prevent refreshing canvas during backend switch
    StopDrawing();

    delete m_gal;

    switch( aGalType )
    {
    case GAL_TYPE_OPENGL:
        m_gal = new KIGFX::OPENGL_GAL( this, this, this );
        break;

    case GAL_TYPE_CAIRO:
        m_gal = new KIGFX::CAIRO_GAL( this, this, this );
        break;

    case GAL_TYPE_NONE:
        return;
    }

    wxSize size = GetClientSize();
    m_gal->ResizeScreen( size.GetX(), size.GetY() );

    if( m_painter )
        m_painter->SetGAL( m_gal );

    if( m_view )
        m_view->SetGAL( m_gal );

    m_backend = aGalType;
}


void EDA_DRAW_PANEL_GAL::onEvent( wxEvent& aEvent )
{
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
