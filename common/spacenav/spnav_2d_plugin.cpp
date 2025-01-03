/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see CHANGELOG.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include "spnav_2d_plugin.h"

#include <gal/graphics_abstraction_layer.h>
#include <class_draw_panel_gal.h>
#include <view/view.h>
#include <view/wx_view_controls.h>

#include <wx/log.h>
#include <pgm_base.h>
#include <settings/common_settings.h>


SPNAV_2D_PLUGIN::SPNAV_2D_PLUGIN( EDA_DRAW_PANEL_GAL* aCanvas )
    : m_timer( this ), m_canvas( aCanvas ), m_focused( true )
{
    m_driver = std::make_unique<LIBSPNAV_DRIVER>();
    m_view = aCanvas->GetView();
    m_scale = 1.0;

    if( m_driver->Connect() )
    {
        m_driver->SetHandler( this );
        Bind( wxEVT_TIMER, &SPNAV_2D_PLUGIN::onPollTimer, this );
        m_timer.Start( 10 );
    }
}

SPNAV_2D_PLUGIN::~SPNAV_2D_PLUGIN()
{
    m_timer.Stop();

    if( m_driver )
        m_driver->Disconnect();
}

void SPNAV_2D_PLUGIN::SetFocus( bool aFocus )
{
    m_focused = aFocus;
}

void SPNAV_2D_PLUGIN::SetCanvas( EDA_DRAW_PANEL_GAL* aCanvas )
{
    m_canvas = aCanvas;
    m_view = aCanvas->GetView();
}

void SPNAV_2D_PLUGIN::onPollTimer( wxTimerEvent& )
{
    if( m_driver && m_focused )
        m_driver->Poll();
}

void SPNAV_2D_PLUGIN::OnPan( double x, double y, double z )
{
    const COMMON_SETTINGS* cfg = Pgm().GetCommonSettings();
    double panFactor = cfg->m_SpaceMouse.pan_speed / 5.0;

    wxLogTrace( "spacenav", "OnPan: x=%f, y=%f, z=%f", x, y, z );

    if( !m_view )
        return;

    if( std::fabs( x ) > std::numeric_limits<double>::epsilon() ||
        std::fabs( y ) > std::numeric_limits<double>::epsilon() ||
        std::fabs( z ) > std::numeric_limits<double>::epsilon() )
    {
        double tx = x * panFactor;
        double tz = z * panFactor;

        if( cfg->m_SpaceMouse.reverse_pan_x )
            tx = -tx;

        if( cfg->m_SpaceMouse.reverse_pan_y )
            tz = -tz;

        VECTOR2D viewPos = m_view->GetCenter();
        viewPos += m_view->ToWorld( VECTOR2D( tx, -tz ), false ) / 40.0;
        m_view->SetCenter( viewPos );

        double zoom = y * panFactor;

        if( cfg->m_SpaceMouse.reverse_zoom )
            zoom = -zoom;

        double current_pixels = m_canvas->GetClientSize().GetWidth();
        double current_scale = m_view->GetScale();
        double desired_pixels = current_pixels + zoom / 10.0;

        if( desired_pixels < 1 )
            desired_pixels = 1;

        double new_scale = current_scale * ( current_pixels / desired_pixels);

        m_view->SetScale( new_scale, viewPos );

        wxMouseEvent moveEvent(  KIGFX::WX_VIEW_CONTROLS::EVT_REFRESH_MOUSE );
        VECTOR2D msp = m_canvas->GetViewControls()->GetMousePosition( false );
        moveEvent.SetX( msp.x );
        moveEvent.SetY( msp.y );

        m_canvas->RequestRefresh();
        wxPostEvent( m_canvas, moveEvent );
    }

}

void SPNAV_2D_PLUGIN::OnRotate( double rx, double ry, double rz )
{
    //Ignore the rotation
}

void SPNAV_2D_PLUGIN::OnButton( int button, bool pressed )
{
    // Buttons are ignored for now
    (void) button;
    (void) pressed;
}
