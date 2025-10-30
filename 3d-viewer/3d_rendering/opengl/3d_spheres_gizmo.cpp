/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright 2025, Damjan Prerad <damjanovmail@gmail.com>
 * Copyright (C) The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <gal/opengl/kiglew.h> // Must be included first
#include <glm/geometric.hpp>

#include "3d_spheres_gizmo.h"


SPHERES_GIZMO::~SPHERES_GIZMO()
{
    if( m_quadric )
    {
        gluDeleteQuadric( m_quadric );
        m_quadric = nullptr;
    }
}


SPHERES_GIZMO::SPHERES_GIZMO( int aGizmoPosX, int aGizmoPosY )
{
    m_gizmoPosX = aGizmoPosX;
    m_gizmoPosY = aGizmoPosY;

    m_quadric = gluNewQuadric();
    gluQuadricNormals( m_quadric, GLU_SMOOTH );
}


void SPHERES_GIZMO::setViewport( int ax, int ay, int aWidth, int aHeight )
{
    m_viewportX = ax;
    m_viewportY = ay;
    m_viewportW = aWidth;
    m_viewportH = aHeight;
}


std::tuple<int, int, int, int> SPHERES_GIZMO::getViewport() const
{
    return std::make_tuple( m_viewportX, m_viewportY, m_viewportW, m_viewportH );
}


void SPHERES_GIZMO::setGizmoPosition( int ax, int ay )
{
    m_gizmoPosX = ax;
    m_gizmoPosY = ay;
}


void SPHERES_GIZMO::setGizmoMaterial()
{
    glEnable( GL_COLOR_MATERIAL );
    glColorMaterial( GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE );

    const SFVEC4F ambient = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F diffuse = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F emissive = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F specular = SFVEC4F( 0.1f, 0.1f, 0.1f, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 96.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT, &ambient.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, &diffuse.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r );
}

void SPHERES_GIZMO::handleMouseInput( int aMouseX, int aMouseY )
{
    int smallViewportW = m_viewportH / 8;
    int smallViewportH = m_viewportH / 8;

    bool inside = ( aMouseX >= m_gizmoPosX && aMouseX <= m_gizmoPosX + smallViewportW && aMouseY >= m_gizmoPosY
                    && aMouseY <= m_gizmoPosY + smallViewportH );

    if( inside )
    {
        m_ndcX = 2.0f * static_cast<float>( aMouseX - m_gizmoPosX ) / smallViewportW - 1.0f;
        m_ndcY = 2.0f * static_cast<float>( aMouseY - m_gizmoPosY ) / smallViewportH - 1.0f;
    }
    else
    {
        m_ndcX = -1.0f;
        m_ndcY = -1.0f;
    }
}


void SPHERES_GIZMO::render3dSpheresGizmo( glm::mat4 aCameraRotationMatrix )
{
    float fov = 60.0f;

    glDisable( GL_CULL_FACE );

    // Set up a square viewport (Y x Y)
    glViewport( m_gizmoPosX, m_gizmoPosY, m_viewportH / 8, m_viewportH / 8 );
    glClear( GL_DEPTH_BUFFER_BIT );

    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    gluPerspective( fov, 1.0f, 0.001f, 2.0f * RANGE_SCALE_3D );

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glm::mat4 TranslationMatrix = glm::translate( glm::mat4( 1.0f ), SFVEC3F( 0.0f, 0.0f, -( m_arrowSize * 2.75f ) ) );
    glm::mat4 ViewMatrix = TranslationMatrix * aCameraRotationMatrix;
    glLoadMatrixf( glm::value_ptr( ViewMatrix ) );

    setGizmoMaterial();

    // Intersection test
    glm::mat4 proj = glm::perspective( glm::radians( fov ), 1.0f, 0.001f, 2.0f * RANGE_SCALE_3D );
    glm::mat4 invVP = glm::inverse( proj * ViewMatrix );

    glm::vec4 rayStartNDC( m_ndcX, m_ndcY, -1.0f, 1.0f );
    glm::vec4 rayEndNDC( m_ndcX, m_ndcY, 1.0f, 1.0f );

    glm::vec4 rayStartWorld = invVP * rayStartNDC;
    rayStartWorld /= rayStartWorld.w;

    glm::vec4 rayEndWorld = invVP * rayEndNDC;
    rayEndWorld /= rayEndWorld.w;

    glm::vec3 rayOrigin = glm::vec3( rayStartWorld );
    glm::vec3 rayDirection = glm::normalize( glm::vec3( rayEndWorld - rayStartWorld ) );

    auto intersects =
            []( const glm::vec3& aRayOrigin, const glm::vec3& aRayDir, const glm::vec3& aSphereCenter, float aRadius )
    {
        glm::vec3 L = aSphereCenter - aRayOrigin;
        float     tca = glm::dot( L, aRayDir );
        float     d2 = glm::dot( L, L ) - tca * tca;
        return d2 <= aRadius * aRadius;
    };

    int clickedIndex = -1;
    m_selectedGizmoSphere = GizmoSphereSelection::None;
    for( size_t i = 0; i < m_spheres.size(); ++i )
    {
        const auto& sphere = m_spheres[i];
        if( intersects( rayOrigin, rayDirection, sphere.m_position, sphere.m_radius ) )
        {
            clickedIndex = static_cast<int>( i );

            m_selectedGizmoSphere = static_cast<GizmoSphereSelection>( i );
            break; // only pick the first intersected sphere
        }
    }

    // Update colors
    for( size_t i = 0; i < m_spheres.size(); ++i )
    {
        if( static_cast<int>( i ) == clickedIndex )
        {
            m_spheres[i].m_color = { 1.0f, 1.0f, 1.0f }; // White
        }
        else
        {
            m_spheres[i].m_color = m_spheres[i].m_originalColor; // Restore default
        }
    }

    // Intersection test done

    auto drawBillboardCircle =
            []( const glm::vec3& aCenter, float aRadius, const glm::vec3& aColor,
                const glm::vec3& aCamRight, const glm::vec3& aCamUp, int aSegments = 64 )
            {
                float thickness = aRadius * 0.4f;
                glColor3f( aColor.r, aColor.g, aColor.b );

                glBegin( GL_TRIANGLE_STRIP );
                for( int i = 0; i <= aSegments; ++i )
                {
                    float     angle = 2.0f * glm::pi<float>() * i / aSegments;
                    glm::vec3 dir = cos( angle ) * aCamRight + sin( angle ) * aCamUp;

                    glm::vec3 outer = aCenter + dir * ( aRadius + thickness * 0.5f );
                    glm::vec3 inner = aCenter + dir * ( aRadius - thickness * 0.5f );

                    glVertex3f( outer.x, outer.y, outer.z );
                    glVertex3f( inner.x, inner.y, inner.z );
                }
                glEnd();
            };

    glm::vec3 camRight( aCameraRotationMatrix[0][0], aCameraRotationMatrix[1][0], aCameraRotationMatrix[2][0] );
    glm::vec3 camUp( aCameraRotationMatrix[0][1], aCameraRotationMatrix[1][1], aCameraRotationMatrix[2][1] );

    glEnable( GL_BLEND );
    glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );

    for( const auto& sphere : m_spheres )
    {
        glColor4f( sphere.m_color.r, sphere.m_color.g, sphere.m_color.b, 0.3f );
        glPushMatrix();
        glTranslatef( sphere.m_position.x, sphere.m_position.y, sphere.m_position.z );
        if( m_quadric )
        {
            gluSphere( m_quadric, sphere.m_radius, 32, 32 );
        }
        glPopMatrix();

        drawBillboardCircle( sphere.m_position, sphere.m_radius, sphere.m_color, camRight, camUp );
    }

    // Draw sphere labels

    glDisable( GL_DEPTH_TEST );
    glDisable( GL_LIGHTING );

    std::array<std::string, 6> labels = { "X", "", "Y", "", "Z", "" };

    // View direction (camera looks along negative Z in view space)
    // So we offset a little toward the camera to avoid z-fighting
    glm::vec3 offset = glm::normalize( -rayDirection ) * 0.02f;

    glColor4f( 0.0f, 0.0f, 0.0f, 1.0f );

    auto drawX =
            []( const glm::vec3& aPos, float aSize, const glm::vec3& aColor, const glm::vec3& aCamRight,
                const glm::vec3& aCamUp )
            {
                glColor3f( aColor.r, aColor.g, aColor.b );
                glLineWidth( 3.0f );

                float h = aSize * 0.5f;

                // Define two diagonal line directions in camera-facing plane
                glm::vec3 dir1 = ( -aCamRight + aCamUp ) * h; // one diagonal
                glm::vec3 dir2 = ( -aCamRight - aCamUp ) * h; // other diagonal

                glBegin( GL_LINES );
                glVertex3f( ( aPos - dir1 ).x, ( aPos - dir1 ).y, ( aPos - dir1 ).z );
                glVertex3f( ( aPos + dir1 ).x, ( aPos + dir1 ).y, ( aPos + dir1 ).z );

                glVertex3f( ( aPos - dir2 ).x, ( aPos - dir2 ).y, ( aPos - dir2 ).z );
                glVertex3f( ( aPos + dir2 ).x, ( aPos + dir2 ).y, ( aPos + dir2 ).z );
                glEnd();
            };

    auto drawY =
            []( const glm::vec3& aPos, float aSize, const glm::vec3& aColor, const glm::vec3& aCamRight,
                const glm::vec3& aCamUp )
            {
                glColor3f( aColor.r, aColor.g, aColor.b );
                glLineWidth( 3.0f );

                float h = aSize * 0.5f;

                // Top-left and top-right in screen plane
                glm::vec3 topLeft = aPos + aCamUp * h - aCamRight * h;
                glm::vec3 topRight = aPos + aCamUp * h + aCamRight * h;
                glm::vec3 bottom = aPos - aCamUp * h;

                glBegin( GL_LINES );
                glVertex3f( topLeft.x, topLeft.y, topLeft.z );
                glVertex3f( aPos.x, aPos.y, aPos.z );

                glVertex3f( topRight.x, topRight.y, topRight.z );
                glVertex3f( aPos.x, aPos.y, aPos.z );

                glVertex3f( aPos.x, aPos.y, aPos.z );
                glVertex3f( bottom.x, bottom.y, bottom.z );
                glEnd();
            };

    auto drawZ =
            []( const glm::vec3& aPos, float aSize, const glm::vec3& aColor, const glm::vec3& aCamRight,
                const glm::vec3& aCamUp )
            {
                glColor3f( aColor.r, aColor.g, aColor.b );
                glLineWidth( 3.0f );

                float h = aSize * 0.5f;

                // Define corners in screen plane relative to camera
                glm::vec3 topLeft = aPos + aCamUp * h - aCamRight * h;
                glm::vec3 topRight = aPos + aCamUp * h + aCamRight * h;
                glm::vec3 bottomLeft = aPos - aCamUp * h - aCamRight * h;
                glm::vec3 bottomRight = aPos - aCamUp * h + aCamRight * h;

                glBegin( GL_LINE_STRIP );
                glVertex3f( topLeft.x, topLeft.y, topLeft.z );
                glVertex3f( topRight.x, topRight.y, topRight.z );
                glVertex3f( bottomLeft.x, bottomLeft.y, bottomLeft.z );
                glVertex3f( bottomRight.x, bottomRight.y, bottomRight.z );
                glEnd();
            };

    for( size_t i = 0; i < m_spheres.size(); ++i )
    {
        if( labels[i].empty() )
            continue;

        glm::vec3          textPos = m_spheres[i].m_position + offset;
        const std::string& label = labels[i];

        if( label == "X" )
        {
            drawX( textPos, 0.30f, glm::vec3( 0.0f ), camRight, camUp );
        }
        else if( label == "Y" )
        {
            drawY( textPos, 0.30f, glm::vec3( 0.0f ), camRight, camUp );
        }
        else if( label == "Z" )
        {
            drawZ( textPos, 0.30f, glm::vec3( 0.0f ), camRight, camUp );
        }
    }

    glEnable( GL_LIGHTING );
    glEnable( GL_DEPTH_TEST );

    // Draw lines only to the positive axis spheres
    glLineWidth( 2.0f );
    glBegin( GL_LINES );

    glColor3f( 0.9f, 0.0f, 0.0f ); // X+
    glVertex3f( 0.0f, 0.0f, 0.0f );
    glVertex3f( m_arrowSize, 0.0f, 0.0f );

    glColor3f( 0.0f, 0.9f, 0.0f ); // Y+
    glVertex3f( 0.0f, 0.0f, 0.0f );
    glVertex3f( 0.0f, m_arrowSize, 0.0f );

    glColor3f( 0.0f, 0.0f, 0.9f ); // Z+
    glVertex3f( 0.0f, 0.0f, 0.0f );
    glVertex3f( 0.0f, 0.0f, m_arrowSize );

    glEnd();

    glEnable( GL_CULL_FACE );
}

SPHERES_GIZMO::GizmoSphereSelection SPHERES_GIZMO::getSelectedGizmoSphere() const
{
    return m_selectedGizmoSphere;
}

void SPHERES_GIZMO::resetSelectedGizmoSphere()
{
    m_selectedGizmoSphere = GizmoSphereSelection::None;
    m_ndcX = -1.0f;
    m_ndcY = -1.0f;
}