/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2013 CERN
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
#include <wx/window.h>

#include <view/view.h>
#include <view/wx_view_controls.h>

using namespace KiGfx;

WX_VIEW_CONTROLS::WX_VIEW_CONTROLS( VIEW* aView, wxWindow* aParentPanel ) :
    VIEW_CONTROLS( aView ),
    m_autoPanMargin( 0.1 ),
    m_autoPanSpeed( 0.15 ),
    m_autoPanCornerRatio( 0.1 ),
    m_parentPanel( aParentPanel )
{
    m_parentPanel->Connect( wxEVT_MOTION, wxMouseEventHandler(
                                WX_VIEW_CONTROLS::onMotion ), NULL, this );
    m_parentPanel->Connect( wxEVT_MOUSEWHEEL, wxMouseEventHandler(
                                WX_VIEW_CONTROLS::onWheel ), NULL, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_UP, wxMouseEventHandler(
                                WX_VIEW_CONTROLS::onButton ), NULL, this );
    m_parentPanel->Connect( wxEVT_MIDDLE_DOWN, wxMouseEventHandler(
                                WX_VIEW_CONTROLS::onButton ), NULL, this );
#if defined _WIN32 || defined _WIN64
    m_parentPanel->Connect( wxEVT_ENTER_WINDOW, wxMouseEventHandler(
                                WX_VIEW_CONTROLS::onEnter ), NULL, this );
#endif
}


void WX_VIEW_CONTROLS::onMotion( wxMouseEvent& event )
{
    if( event.Dragging() && m_isDragPanning )
    {
        VECTOR2D mousePoint( event.GetX(), event.GetY() );
        VECTOR2D d     = m_dragStartPoint - mousePoint;
        VECTOR2D delta = m_view->ToWorld( d, false );

        m_view->SetCenter( m_lookStartPoint + delta );
        m_parentPanel->Refresh();
    }

    event.Skip();
}


void WX_VIEW_CONTROLS::onWheel( wxMouseEvent& event )
{
    const double wheelPanSpeed = 0.001;

    if( event.ControlDown() || event.ShiftDown() )
    {
        // Scrolling
        VECTOR2D scrollVec = m_view->ToWorld( m_view->GetScreenPixelSize() *
                             ( (double) event.GetWheelRotation() * wheelPanSpeed ), false );
        double   scrollSpeed;

        if( abs( scrollVec.x ) > abs( scrollVec.y ) )
            scrollSpeed = scrollVec.x;
        else
            scrollSpeed = scrollVec.y;

        VECTOR2D t = m_view->GetScreenPixelSize();
        VECTOR2D delta( event.ControlDown() ? -scrollSpeed : 0.0,
                        event.ShiftDown() ? -scrollSpeed : 0.0 );

        m_view->SetCenter( m_view->GetCenter() + delta );
        m_parentPanel->Refresh();
    }
    else
    {
        // Zooming
        wxLongLong  timeStamp   = wxGetLocalTimeMillis();
        double      timeDiff    = timeStamp.ToDouble() - m_timeStamp.ToDouble();

        m_timeStamp = timeStamp;
        double      zoomScale;

        // Set scaling speed depending on scroll wheel event interval
        if( timeDiff < 500 && timeDiff > 0 )
        {
            zoomScale = ( event.GetWheelRotation() > 0.0 ) ? 2.05 - timeDiff / 500 :
                        1.0 / ( 2.05 - timeDiff / 500 );
        }
        else
        {
            zoomScale = ( event.GetWheelRotation() > 0.0 ) ? 1.05 : 0.95;
        }

        VECTOR2D anchor = m_view->ToWorld( VECTOR2D( event.GetX(), event.GetY() ) );
        m_view->SetScale( m_view->GetScale() * zoomScale, anchor );
        m_parentPanel->Refresh();
    }

    event.Skip();
}


void WX_VIEW_CONTROLS::onButton( wxMouseEvent& event )
{
    if( event.MiddleDown() )
    {
        m_isDragPanning     = true;
        m_dragStartPoint    = VECTOR2D( event.GetX(), event.GetY() );
        m_lookStartPoint    = m_view->GetCenter();
    }
    else if( event.MiddleUp() )
    {
        m_isDragPanning = false;
    }

    event.Skip();
}


void WX_VIEW_CONTROLS::onEnter( wxMouseEvent& event )
{
    m_parentPanel->SetFocus();
}
