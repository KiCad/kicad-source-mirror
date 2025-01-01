/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * @file  eda_3d_canvas_pivot.cpp
 * @brief Implementation of a 3d cursor
 */

#include "../common_ogl/openGL_includes.h"
#include "../common_ogl/ogl_utils.h"
#include "eda_3d_canvas.h"


static void pivot_render_triangles( float t )
{
    wxASSERT( t >= 0.0f );

    SFVEC3F vertexPointer[12];

    const float u = 1.0f / 6.0f;

    vertexPointer[0] = SFVEC3F( ( -3.0f + t ) * u, -2.0f * u, 0.0f );
    vertexPointer[1] = SFVEC3F( ( -3.0f + t ) * u, 2.0f * u, 0.0f );
    vertexPointer[2] = SFVEC3F( ( -1.0f + t ) * u, 0.0f * u, 0.0f );

    vertexPointer[3] = SFVEC3F( -2.0f * u, ( -3.0f + t ) * u, 0.0f );
    vertexPointer[4] = SFVEC3F( 2.0f * u, ( -3.0f + t ) * u, 0.0f );
    vertexPointer[5] = SFVEC3F( 0.0f * u, ( -1.0f + t ) * u, 0.0f );

    vertexPointer[6] = SFVEC3F( ( 3.0f - t ) * u, -2.0f * u, 0.0f );
    vertexPointer[7] = SFVEC3F( ( 3.0f - t ) * u, 2.0f * u, 0.0f );
    vertexPointer[8] = SFVEC3F( ( 1.0f - t ) * u, 0.0f * u, 0.0f );

    vertexPointer[9] = SFVEC3F( 2.0f * u, ( 3.0f - t ) * u, 0.0f );
    vertexPointer[10] = SFVEC3F( -2.0f * u, ( 3.0f - t ) * u, 0.0f );
    vertexPointer[11] = SFVEC3F( 0.0f * u, ( 1.0f - t ) * u, 0.0f );

    glDisableClientState( GL_TEXTURE_COORD_ARRAY );
    glDisableClientState( GL_COLOR_ARRAY );
    glDisableClientState( GL_NORMAL_ARRAY );
    glEnableClientState( GL_VERTEX_ARRAY );
    glVertexPointer( 3, GL_FLOAT, 0, vertexPointer );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glDrawArrays( GL_TRIANGLES, 0, 4 * 3 );

    glDisable( GL_BLEND );

    glDisableClientState( GL_VERTEX_ARRAY );
}


void EDA_3D_CANVAS::render_pivot( float t, float aScale )
{
    wxASSERT( aScale >= 0.0f );
    wxASSERT( t >= 0.0f );

    if( t > 1.0f )
        t = 1.0f;

    const SFVEC3F &lookAtPos = m_camera.GetLookAtPos_T1();

    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );

    // Set projection and modelview matrixes
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_camera.GetProjectionMatrix() ) );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glLoadMatrixf( glm::value_ptr( m_camera.GetViewMatrix() ) );

    glEnable( GL_COLOR_MATERIAL );
    glColor4f( 0.0f, 1.0f, 0.0f, 0.75f - t * 0.75f );

    // Translate to the look at position
    glTranslatef( lookAtPos.x, lookAtPos.y, lookAtPos.z );

    glScalef( aScale, aScale, aScale );
    pivot_render_triangles( t * 0.5f );

    t = t * 0.80f;
    glScalef( 1.0f - t, 1.0f - t, 1.0f - t );
    glColor4f( 0.0f, 1.0f, 0.0f, 0.8f - t );

    glPushMatrix();
    glRotatef( t * 90.0f, 0.0f, 0.0f, 1.0f );
    pivot_render_triangles( t * 0.5f );
    glPopMatrix();

    glPushMatrix();
    glRotatef( -t * 90.0f, 0.0f, 0.0f, 1.0f );
    pivot_render_triangles( t  * 0.5f );
    glPopMatrix();
}


void EDA_3D_CANVAS::render3dmousePivot( float aScale )
{
    wxASSERT( aScale >= 0.0f );

    glDisable( GL_LIGHTING );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_CULL_FACE );

    // Set projection and modelview matrixes
    glMatrixMode( GL_PROJECTION );
    glLoadMatrixf( glm::value_ptr( m_camera.GetProjectionMatrix() ) );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();
    glLoadMatrixf( glm::value_ptr( m_camera.GetViewMatrix() ) );

    glEnable( GL_COLOR_MATERIAL );
    glColor4f( 0.0f, 0.667f, 0.902f, 0.75f );

    // Translate to the look at position
    glTranslatef( m_3dmousePivotPos.x, m_3dmousePivotPos.y, m_3dmousePivotPos.z );

    glPointSize( 16.0f );
    glEnable( GL_POINT_SMOOTH );
    glHint( GL_POINT_SMOOTH_HINT, GL_NICEST );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    glScalef( aScale, aScale, aScale );

    // Draw a point at the look at position.
    glBegin( GL_POINTS );
    glVertex3f( 0, 0, 0 );
    glEnd();

    glDisable( GL_BLEND );
    glDisable( GL_POINT_SMOOTH );
}
