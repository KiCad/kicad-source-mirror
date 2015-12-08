/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2015 KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file  ctrack_ball.cpp
 * @brief
 */

#include "common_ogl/openGL_includes.h"
#include "ctrack_ball.h"
#include "trackball.h"
#include <wx/log.h>


CTRACK_BALL::CTRACK_BALL( float aRangeScale ) : CCAMERA( aRangeScale )
{
    wxLogTrace( m_logTrace, wxT( "CTRACK_BALL::CTRACK_BALL" ) );

    memset( m_quat, 0, sizeof( m_quat ) );
    trackball( m_quat, 0.0, 0.0, 0.0, 0.0 );
}


void CTRACK_BALL::Drag( const wxPoint &aNewMousePosition )
{
    m_parametersChanged = true;

    double spin_quat[4];

    // "Pass the x and y coordinates of the last and current positions of
    //  the mouse, scaled so they are from (-1.0 ... 1.0)."
    float zoom = std::min( m_zoom, 1.0f );
    trackball( spin_quat,
               zoom * (2.0 * m_lastPosition.x - m_windowSize.x) / m_windowSize.x,
               zoom * (m_windowSize.y - 2.0 * m_lastPosition.y) / m_windowSize.y,
               zoom * (2.0 * aNewMousePosition.x - m_windowSize.x) / m_windowSize.x,
               zoom * ( m_windowSize.y - 2.0 * aNewMousePosition.y ) / m_windowSize.y);

    add_quats( spin_quat, m_quat, m_quat );

    GLfloat rotationMatrix[4][4];

    build_rotmatrix( rotationMatrix, m_quat );

    m_rotationMatrix = glm::make_mat4( &rotationMatrix[0][0] );

    updateViewMatrix();

    updateFrustum();
}

void CTRACK_BALL::SetBoardLookAtPos( const SFVEC3F &aBoardPos )
{
    if( m_boardLookAt_pos != aBoardPos )
    {
        m_boardLookAt_pos = -aBoardPos;

        updateViewMatrix();
        updateFrustum();
        m_parametersChanged = true;
    }
}

void CTRACK_BALL::Pan( const wxPoint &aNewMousePosition )
{
    m_parametersChanged = true;

    // Current zoom and an additional factor are taken into account
    // for the amount of panning.

    float zoom = std::min( m_zoom, 1.0f );
    float panFactor = m_range_scale * zoom * (zoom * 4.0f);
    m_camera_pos.x -= panFactor * ( m_lastPosition.x - aNewMousePosition.x ) / m_windowSize.x;
    m_camera_pos.y -= panFactor * (aNewMousePosition.y - m_lastPosition.y ) / m_windowSize.y;

    updateViewMatrix();
    updateFrustum();
}


void CTRACK_BALL::Pan( const SFVEC3F &aDeltaOffsetInc )
{
    m_parametersChanged = true;

    m_camera_pos += aDeltaOffsetInc;

    updateViewMatrix();
    updateFrustum();
}


void CTRACK_BALL::Reset()
{
    m_parametersChanged = true;
}

