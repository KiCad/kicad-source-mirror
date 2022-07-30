/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2016-2022 Kicad Developers, see change_log.txt for contributors.
 *
 * Base class for HiDPI aware wxGLCanvas implementations.
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

#include <gal/hidpi_gl_3D_canvas.h>

const float HIDPI_GL_3D_CANVAS::m_delta_move_step_factor = 0.7f;

HIDPI_GL_3D_CANVAS::HIDPI_GL_3D_CANVAS( CAMERA& aCamera, wxWindow* aParent, wxWindowID,
                                        const int* aAttribList, const wxPoint& aPos,
                                        const wxSize& aSize, long aStyle, const wxString& aName,
                                        const wxPalette& aPalette ) :
        HIDPI_GL_CANVAS( aParent, wxID_ANY, aAttribList, aPos, aSize, aStyle, aName, aPalette ),
        m_mouse_is_moving( false ),
        m_mouse_was_moved( false ),
        m_camera_is_moving( false ),
        m_camera( aCamera )
{
}


void HIDPI_GL_3D_CANVAS::OnMouseMoveCamera( wxMouseEvent& event )
{
    if( m_camera_is_moving )
        return;

    const wxSize&  nativeWinSize = GetNativePixelSize();
    const wxPoint& nativePosition = GetNativePosition( event.GetPosition() );

    m_camera.SetCurWindowSize( nativeWinSize );

    if( event.Dragging() )
    {
        if( event.LeftIsDown() ) // Drag
            m_camera.Drag( nativePosition );
        else if( event.MiddleIsDown() ) // Pan
            m_camera.Pan( nativePosition );

        m_mouse_is_moving = true;
        m_mouse_was_moved = true;
    }

    m_camera.SetCurMousePosition( nativePosition );
}

void HIDPI_GL_3D_CANVAS::OnMouseWheelCamera( wxMouseEvent& event, bool aPan )
{
    bool mouseActivity = false;

    if( m_camera_is_moving )
        return;

    float delta_move = m_delta_move_step_factor * m_camera.GetZoom();

    if( aPan )
        delta_move *= 0.01f * event.GetWheelRotation();
    else if( event.GetWheelRotation() < 0 )
        delta_move = -delta_move;

    // mousewheel_panning enabled:
    //      wheel           -> pan;
    //      wheel + shift   -> horizontal scrolling;
    //      wheel + ctrl    -> zooming;
    // mousewheel_panning disabled:
    //      wheel + shift   -> vertical scrolling;
    //      wheel + ctrl    -> horizontal scrolling;
    //      wheel           -> zooming.

    if( aPan && !event.ControlDown() )
    {
        if( event.GetWheelAxis() == wxMOUSE_WHEEL_HORIZONTAL || event.ShiftDown() )
            m_camera.Pan( SFVEC3F( -delta_move, 0.0f, 0.0f ) );
        else
            m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );

        mouseActivity = true;
    }
    else if( event.ShiftDown() && !aPan )
    {
        m_camera.Pan( SFVEC3F( 0.0f, -delta_move, 0.0f ) );
        mouseActivity = true;
    }
    else if( event.ControlDown() && !aPan )
    {
        m_camera.Pan( SFVEC3F( delta_move, 0.0f, 0.0f ) );
        mouseActivity = true;
    }
    else
    {
        mouseActivity = m_camera.Zoom( event.GetWheelRotation() > 0 ? 1.1f : 1 / 1.1f );
    }

    // If it results on a camera movement
    if( mouseActivity )
    {
        m_mouse_is_moving = true;
        m_mouse_was_moved = true;
    }

    // Update the cursor current mouse position on the camera
    m_camera.SetCurMousePosition( GetNativePosition( event.GetPosition() ) );
}
