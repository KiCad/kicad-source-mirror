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

#include "spnav_viewer_plugin.h"

#include <3d-viewer/3d_canvas/eda_3d_canvas.h>
#include <3d-viewer/3d_rendering/track_ball.h>
#include <pgm_base.h>
#include <settings/common_settings.h>

SPNAV_VIEWER_PLUGIN::SPNAV_VIEWER_PLUGIN( EDA_3D_CANVAS* aCanvas )
    : m_timer( this ), m_canvas( aCanvas ), m_camera( nullptr ), m_focused( true )
{
    m_camera = dynamic_cast<TRACK_BALL*>( aCanvas->GetCamera() );
    m_driver = std::make_unique<LIBSPNAV_DRIVER>();

    if( m_driver->Connect() )
    {
        m_driver->SetHandler( this );
        Bind( wxEVT_TIMER, &SPNAV_VIEWER_PLUGIN::onPollTimer, this );
        m_timer.Start( 10 );
    }
}

SPNAV_VIEWER_PLUGIN::~SPNAV_VIEWER_PLUGIN()
{
    m_timer.Stop();

    if( m_driver )
        m_driver->Disconnect();
}

void SPNAV_VIEWER_PLUGIN::SetFocus( bool aFocus )
{
    m_focused = aFocus;
}

void SPNAV_VIEWER_PLUGIN::onPollTimer( wxTimerEvent& )
{
    if( m_driver && m_focused )
        m_driver->Poll();
}

void SPNAV_VIEWER_PLUGIN::OnPan( double x, double y, double z )
{
    if( const COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
    {
        float scale = 0.0005f * ( cfg->m_SpaceMouse.pan_speed / 5.0f );

        if( cfg->m_SpaceMouse.reverse_pan_x )
            x = -x;

        if( cfg->m_SpaceMouse.reverse_pan_y )
            y = -y;

        if( cfg->m_SpaceMouse.reverse_zoom )
            z = -z;

        if( m_camera )
        {
            m_camera->Pan( SFVEC3F( x * scale, -y * scale, z * scale ) );
            m_canvas->Request_refresh();
        }
    }
}

void SPNAV_VIEWER_PLUGIN::OnRotate( double rx, double ry, double rz )
{
    if( const COMMON_SETTINGS* cfg = Pgm().GetCommonSettings() )
    {
        float scale = 0.001f * ( cfg->m_SpaceMouse.rotate_speed / 5.0f );

        if( cfg->m_SpaceMouse.reverse_rotate )
            scale = -scale;

        if( m_camera )
        {
            m_camera->RotateX( ry * scale );
            m_camera->RotateY( rx * scale );
            m_camera->RotateZ( rz * scale );
            m_canvas->Request_refresh();
        }
    }
}

void SPNAV_VIEWER_PLUGIN::OnButton( int button, bool pressed )
{
    // Buttons are ignored for now
    (void) button;
    (void) pressed;
}
