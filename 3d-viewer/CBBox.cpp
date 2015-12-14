/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015 Mario Luzeiro <mrluzeiro@gmail.com>
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
 * @file CBBox.cpp
 * @brief Bounding Box class implementation
 */

#include "CBBox.h"


// openGL includes used for debug the bounding box
#ifdef __WXMAC__
#  ifdef __DARWIN__
#    include <OpenGL/glu.h>
#  else
#    include <glu.h>
#  endif
#else
#  include <GL/glu.h>
#endif

CBBOX::CBBOX()
{
    Reset();
}

CBBOX::CBBOX( const S3D_VERTEX &aPbInit )
{
    m_min = aPbInit;
    m_max = aPbInit;

    m_initialized = true;
}

CBBOX::CBBOX( const S3D_VERTEX &aPbMin, const S3D_VERTEX &aPbMax )
{
    Set( aPbMin, aPbMax );
}

CBBOX::~CBBOX()
{
}

void CBBOX::Set( const S3D_VERTEX &aPbMin, const S3D_VERTEX &aPbMax )
{
    m_min.x =  glm::min( aPbMin.x, aPbMax.x );
    m_min.y =  glm::min( aPbMin.y, aPbMax.y );
    m_min.z =  glm::min( aPbMin.z, aPbMax.z );

    m_max.x =  glm::max( aPbMin.x, aPbMax.x );
    m_max.y =  glm::max( aPbMin.y, aPbMax.y );
    m_max.z =  glm::max( aPbMin.z, aPbMax.z );

    m_initialized = true;
}

bool CBBOX::IsInitialized() const
{
    return m_initialized;
}

void CBBOX::Reset()
{
    m_min = S3D_VERTEX( 0.0f, 0.0f, 0.0f );
    m_max = S3D_VERTEX( 0.0f, 0.0f, 0.0f );

    m_initialized = false;
}

void CBBOX::Union( const S3D_VERTEX &aPoint )
{
    if( !m_initialized )
    {
        m_initialized = true;
        // Initialize the bounding box with the given point
        m_min = aPoint;
        m_max = aPoint;
    }
    else
    {
        // get the minimun value between the added point and the existent bounding box
        m_min.x =  glm::min( m_min.x, aPoint.x );
        m_min.y =  glm::min( m_min.y, aPoint.y );
        m_min.z =  glm::min( m_min.z, aPoint.z );

        // get the maximun value between the added point and the existent bounding box
        m_max.x =  glm::max( m_max.x, aPoint.x );
        m_max.y =  glm::max( m_max.y, aPoint.y );
        m_max.z =  glm::max( m_max.z, aPoint.z );
    }
}


void CBBOX::Union( const CBBOX &aBBox )
{
    if( aBBox.m_initialized == false )
        return;

    if( !m_initialized )
    {
        // Initialize the bounding box with the given bounding box
        m_initialized = true;
        m_min = aBBox.m_min;
        m_max = aBBox.m_max;
    }
    else
    {
        // get the minimun value between the added bounding box and the existent bounding box
        m_min.x =  glm::min( m_min.x, aBBox.m_min.x );
        m_min.y =  glm::min( m_min.y, aBBox.m_min.y );
        m_min.z =  glm::min( m_min.z, aBBox.m_min.z );

        // get the maximun value between the added bounding box and the existent bounding box
        m_max.x =  glm::max( m_max.x, aBBox.m_max.x );
        m_max.y =  glm::max( m_max.y, aBBox.m_max.y );
        m_max.z =  glm::max( m_max.z, aBBox.m_max.z );
    }
}


S3D_VERTEX CBBOX::GetCenter() const
{
    return (m_max + m_min) * 0.5f;
}

S3D_VERTEX CBBOX::Min() const
{
    return m_min;
}

S3D_VERTEX CBBOX::Max() const
{
    return m_max;
}

void CBBOX::Scale( float aScale )
{
    if( m_initialized == false )
        return;

    S3D_VERTEX scaleV  = S3D_VERTEX( aScale, aScale, aScale );
    S3D_VERTEX centerV = GetCenter();

    m_min = (m_min - centerV) * scaleV + centerV;
    m_max = (m_max - centerV) * scaleV + centerV;
}


bool CBBOX::OverlapsBox( const CBBOX &aBBox ) const
{
    if( aBBox.m_initialized == false )
        return false;

    bool x = ( m_max.x >= aBBox.m_min.x ) && ( m_min.x <= aBBox.m_max.x );
    bool y = ( m_max.y >= aBBox.m_min.y ) && ( m_min.y <= aBBox.m_max.y );
    bool z = ( m_max.z >= aBBox.m_min.z ) && ( m_min.z <= aBBox.m_max.z );

    return ( x && y && z );
}


bool CBBOX::Inside( const S3D_VERTEX &aPoint ) const
{
    if( m_initialized == false )
        return false;

    return (( aPoint.x >= m_min.x ) && ( aPoint.x <= m_max.x ) &&
            ( aPoint.y >= m_min.y ) && ( aPoint.y <= m_max.y ) &&
            ( aPoint.z >= m_min.z ) && ( aPoint.z <= m_max.z ));
}


float CBBOX::Volume() const
{
    if( m_initialized == false )
        return 0.0f;

    S3D_VERTEX d = m_max - m_min;
    return d.x * d.y * d.z;
}


void CBBOX::ApplyTransformation( glm::mat4 aTransformMatrix )
{
    if( m_initialized == false )
        return;

    S3D_VERTEX v1 = S3D_VERTEX( aTransformMatrix * glm::vec4( m_min.x, m_min.y, m_min.z, 1.0f ) );
    S3D_VERTEX v2 = S3D_VERTEX( aTransformMatrix * glm::vec4( m_max.x, m_max.y, m_max.z, 1.0f ) );

    Reset();
    Union( v1 );
    Union( v2 );
}


void CBBOX::ApplyTransformationAA( glm::mat4 aTransformMatrix )
{
    if( m_initialized == false )
        return;

    // apply the transformation matrix for each of vertices of the bounding box
    // and make a union with all vertices
    CBBOX tmpBBox = CBBOX(  S3D_VERTEX( aTransformMatrix * glm::vec4( m_min.x, m_min.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_max.x, m_min.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_min.x, m_max.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_min.x, m_min.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_min.x, m_max.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_max.x, m_max.y, m_min.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_max.x, m_min.y, m_max.z, 1.0f ) ) );
    tmpBBox.Union(          S3D_VERTEX( aTransformMatrix * glm::vec4( m_max.x, m_max.y, m_max.z, 1.0f ) ) );

    m_min = tmpBBox.m_min;
    m_max = tmpBBox.m_max;
}


void CBBOX::GLdebug() const
{
    if( m_initialized == false )
        return;

    glBegin( GL_LINE_LOOP );
    glVertex3f( m_min.x, m_min.y, m_min.z );
    glVertex3f( m_max.x, m_min.y, m_min.z );
    glVertex3f( m_max.x, m_max.y, m_min.z );
    glVertex3f( m_min.x, m_max.y, m_min.z );
    glEnd();

    glBegin( GL_LINE_LOOP );
    glVertex3f( m_min.x, m_min.y, m_max.z );
    glVertex3f( m_max.x, m_min.y, m_max.z );
    glVertex3f( m_max.x, m_max.y, m_max.z );
    glVertex3f( m_min.x, m_max.y, m_max.z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( m_min.x, m_min.y, m_min.z );
    glVertex3f( m_min.x, m_min.y, m_max.z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( m_max.x, m_min.y, m_min.z );
    glVertex3f( m_max.x, m_min.y, m_max.z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( m_max.x, m_max.y, m_min.z );
    glVertex3f( m_max.x, m_max.y, m_max.z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( m_min.x, m_max.y, m_min.z );
    glVertex3f( m_min.x, m_max.y, m_max.z );
    glEnd();
}
