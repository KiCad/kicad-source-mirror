/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
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

#include "../../common_ogl/openGL_includes.h"
#include "opengl_utils.h"
#include <trigo.h>
#include <wx/debug.h>   // For the wxASSERT

#define RADPERDEG 0.0174533f

void DrawRoundArrow( SFVEC3F aPosition, SFVEC3F aTargetPos, float aSize )
{
    wxASSERT( aSize > 0.0f );

    SFVEC3F vec = aTargetPos - aPosition;
    float length = glm::length( vec );

    GLUquadricObj *quadObj;

    glPushMatrix ();

      glTranslatef( aPosition.x, aPosition.y, aPosition.z );

      if( ( vec.x != 0.0f ) || ( vec.y != 0.0f ) )
      {
          glRotatef( atan2( vec.y, vec.x ) / RADPERDEG, 0.0f, 0.0f, 1.0f );
          glRotatef( atan2( sqrt( vec.x * vec.x + vec.y * vec.y ), vec.z ) / RADPERDEG,
                     0.0f, 1.0f, 0.0f );

      }
      else if( vec.z < 0.0f )
      {
        glRotatef( 180.0f, 1.0f, 0.0f, 0.0f );
      }

      glTranslatef( 0.0f, 0.0f, length - 4.0f * aSize );

      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluCylinder( quadObj, 2.0 * aSize, 0.0, 4.0 * aSize, 12, 1 );
      gluDeleteQuadric( quadObj );

      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluDisk( quadObj, 0.0, 2.0 * aSize, 12, 1 );
      gluDeleteQuadric( quadObj );

      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluDisk( quadObj, 0.0, aSize, 12, 1 );
      gluDeleteQuadric( quadObj );


      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluSphere( quadObj, aSize, 24, 24 );
      gluDeleteQuadric( quadObj );

      glTranslatef( 0.0f , 0.0f ,-length + 4.0f * aSize );

      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluCylinder( quadObj, aSize, aSize, length - 4.0 * aSize, 12, 1 );
      gluDeleteQuadric( quadObj );

      quadObj = gluNewQuadric();
      gluQuadricDrawStyle( quadObj, GLU_FILL );
      gluQuadricNormals( quadObj, GLU_SMOOTH );
      gluDisk( quadObj, 0.0, aSize, 12, 1 );
      gluDeleteQuadric( quadObj );

    glPopMatrix ();
}


void DrawBoundingBox( const BBOX_3D& aBBox )
{
    wxASSERT( aBBox.IsInitialized() );

    glBegin( GL_LINE_LOOP );
    glVertex3f( aBBox.Min().x, aBBox.Min().y, aBBox.Min().z );
    glVertex3f( aBBox.Max().x, aBBox.Min().y, aBBox.Min().z );
    glVertex3f( aBBox.Max().x, aBBox.Max().y, aBBox.Min().z );
    glVertex3f( aBBox.Min().x, aBBox.Max().y, aBBox.Min().z );
    glEnd();

    glBegin( GL_LINE_LOOP );
    glVertex3f( aBBox.Min().x, aBBox.Min().y, aBBox.Max().z );
    glVertex3f( aBBox.Max().x, aBBox.Min().y, aBBox.Max().z );
    glVertex3f( aBBox.Max().x, aBBox.Max().y, aBBox.Max().z );
    glVertex3f( aBBox.Min().x, aBBox.Max().y, aBBox.Max().z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( aBBox.Min().x, aBBox.Min().y, aBBox.Min().z );
    glVertex3f( aBBox.Min().x, aBBox.Min().y, aBBox.Max().z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( aBBox.Max().x, aBBox.Min().y, aBBox.Min().z );
    glVertex3f( aBBox.Max().x, aBBox.Min().y, aBBox.Max().z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( aBBox.Max().x, aBBox.Max().y, aBBox.Min().z );
    glVertex3f( aBBox.Max().x, aBBox.Max().y, aBBox.Max().z );
    glEnd();

    glBegin( GL_LINE_STRIP );
    glVertex3f( aBBox.Min().x, aBBox.Max().y, aBBox.Min().z );
    glVertex3f( aBBox.Min().x, aBBox.Max().y, aBBox.Max().z );
    glEnd();
}


void DrawHalfOpenCylinder( unsigned int aNrSidesPerCircle )
{
    if( aNrSidesPerCircle > 1 )
    {
        const float radius = 0.5f;
        const EDA_ANGLE delta = ANGLE_360 / aNrSidesPerCircle;

        // Generate bottom
        glNormal3f( 0.0f, 0.0f,-1.0f );
        glBegin( GL_TRIANGLE_FAN );
        glVertex3f( 0.0, 0.0, 0.0 );  // This is the V0 of the FAN

        for( EDA_ANGLE ii = ANGLE_0; ii < ANGLE_180; ii += delta )
        {
            SFVEC2D corner = SFVEC2D( 0.0, radius );
            RotatePoint( &corner.x, &corner.y, ii );
            glVertex3f( static_cast<GLfloat>( corner.x ), static_cast<GLfloat>( corner.y ), 0.0 );
        }

        glVertex3d( 0.0, -radius, 0.0 );
        glEnd();

        // Generate top
        glNormal3f( 0.0f, 0.0f, 1.0f );
        glBegin( GL_TRIANGLE_FAN );
        glVertex3f( 0.0, 0.0, 1.0 );  // This is the V0 of the FAN

        for( EDA_ANGLE ii = ANGLE_180; ii > ANGLE_0; ii -= delta )
        {
            SFVEC2D corner = SFVEC2D( 0.0, radius );

            RotatePoint( &corner.x, &corner.y, ii );
            glVertex3f( static_cast<GLfloat>( corner.x ), static_cast<GLfloat>( corner.y ), 1.0 );
        }

        glVertex3f( 0.0, radius, 1.0 );
        glEnd();

        // Generate contours
        glBegin( GL_QUAD_STRIP );

        for( EDA_ANGLE ii = ANGLE_180; ii > ANGLE_0; ii -= delta )
        {
            SFVEC2D corner = SFVEC2D( 0.0, radius );

            RotatePoint( &corner.x, &corner.y, ii );
            glNormal3f( static_cast<GLfloat>( corner.x * 2.0f ),
                        static_cast<GLfloat>( corner.y * 2.0f ), 0.0f );
            glVertex3f( static_cast<GLfloat>( corner.x ), static_cast<GLfloat>( corner.y ), 1.0f );
            glVertex3f( static_cast<GLfloat>( corner.x ), static_cast<GLfloat>( corner.y ), 0.0f );
        }

        glNormal3f( 0.0, 1.0f, 0.0f );
        glVertex3d( 0.0, radius, 1.0 );
        glVertex3d( 0.0, radius, 0.0 );
        glEnd();
    }
}


void DrawSegment( const ROUND_SEGMENT_2D& aSegment, unsigned int aNrSidesPerCircle )
{
    glPushMatrix();

    const SFVEC2F& start = aSegment.GetStart();
    const SFVEC2F& end_minus_start = aSegment.GetEnd_minus_Start();
    const float radius = aSegment.GetRadius();
    const float width  = aSegment.GetWidth();
    const float length = aSegment.GetLength();

    glTranslatef( start.x, start.y, 0.0f );

    if( ( end_minus_start.x != 0.0f ) || ( end_minus_start.y != 0.0f ) )
    {
        glRotatef( atan2( end_minus_start.y, end_minus_start.x ) / RADPERDEG, 0.0f, 0.0f, 1.0f );
    }

    glPushMatrix();
    glTranslatef( length, 0.0, 0.0f );
    glScalef( width, width, 1.0f );
    DrawHalfOpenCylinder( aNrSidesPerCircle );
    glPopMatrix();

    glBegin( GL_QUADS );
    glNormal3f( 0.0,-1.0, 0.0 );
    glVertex3f( length,-radius, 1.0 );
    glVertex3f( 0.0,   -radius, 1.0 );
    glVertex3f( 0.0,   -radius, 0.0 );
    glVertex3f( length,-radius, 0.0 );
    glEnd();

    glBegin( GL_QUADS );
    glNormal3f( 0.0,  1.0, 0.0 );
    glVertex3f( length, radius, 0.0 );
    glVertex3f( 0.0,    radius, 0.0 );
    glVertex3f( 0.0,    radius, 1.0 );
    glVertex3f( length, radius, 1.0 );
    glEnd();

    glBegin( GL_QUADS );
    glNormal3f( 0.0, 0.0, 1.0 );
    glVertex3f( length, radius, 1.0 );
    glVertex3f( 0.0,    radius, 1.0 );
    glVertex3f( 0.0,   -radius, 1.0 );
    glVertex3f( length,-radius, 1.0 );
    glEnd();

    glBegin( GL_QUADS );
    glNormal3f( 0.0, 0.0,-1.0 );
    glVertex3f( length,-radius, 0.0 );
    glVertex3f( 0.0,   -radius, 0.0 );
    glVertex3f( 0.0,    radius, 0.0 );
    glVertex3f( length, radius, 0.0 );
    glEnd();

    glScalef( width, width, 1.0f );
    glRotatef( 180, 0.0, 0.0, 1.0 );
    DrawHalfOpenCylinder( aNrSidesPerCircle );

    glPopMatrix ();
}
