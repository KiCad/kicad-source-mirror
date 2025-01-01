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

/**
 * @file  ogl_utils.cpp
 * @brief implements generic openGL functions that are common to any openGL target
 */

#include <stdexcept>
#include <gal/opengl/kiglew.h>    // Must be included first

#include "openGL_includes.h"
#include "ogl_utils.h"


void OglGetScreenshot( wxImage& aDstImage )
{
    struct viewport_params
    {
        GLint originX;
        GLint originY;
        GLint x;
        GLint y;
    } viewport;

    glGetIntegerv( GL_VIEWPORT, (GLint*) &viewport );

    unsigned char* pixelbuffer = (unsigned char*) malloc( viewport.x * viewport.y * 4 );

    // Call glFinish before screenshot to ensure everything is fully drawn.
    glFinish();

    glPixelStorei( GL_PACK_ALIGNMENT, 1 );
    glReadBuffer( GL_BACK_LEFT );

    glReadPixels( viewport.originX, viewport.originY, viewport.x, viewport.y, GL_RGBA,
                  GL_UNSIGNED_BYTE, pixelbuffer );

    unsigned char* rgbBuffer = (unsigned char*) malloc( viewport.x * viewport.y * 3 );
    unsigned char* alphaBuffer = (unsigned char*) malloc( viewport.x * viewport.y );

    unsigned char* rgbaPtr = pixelbuffer;
    unsigned char* rgbPtr = rgbBuffer;
    unsigned char* alphaPtr = alphaBuffer;

    for( int y = 0; y < viewport.y; y++ )
    {
        for( int x = 0; x < viewport.x; x++ )
        {
            rgbPtr[0] = rgbaPtr[0];
            rgbPtr[1] = rgbaPtr[1];
            rgbPtr[2] = rgbaPtr[2];
            alphaPtr[0] = rgbaPtr[3];

            rgbaPtr += 4;
            rgbPtr += 3;
            alphaPtr += 1;
        }
    }

    // "Sets the image data without performing checks.
    //  The data given must have the size (width*height*3)
    //  The data must have been allocated with malloc()
    //  If static_data is false, after this call the pointer to the data is owned
    //  by the wxImage object, that will be responsible for deleting it."
    aDstImage.SetData( rgbBuffer, viewport.x, viewport.y, false );
    aDstImage.SetAlpha( alphaBuffer, false );

    free( pixelbuffer );

    aDstImage = aDstImage.Mirror( false );
}


GLuint OglLoadTexture( const IMAGE& aImage )
{
    unsigned char* rgbaBuffer = (unsigned char*) malloc( aImage.GetWidth() *
                                                         aImage.GetHeight() * 4 );

    unsigned char* dst = rgbaBuffer;
    const unsigned char* ori = aImage.GetBuffer();

    for( unsigned int i = 0; i < ( aImage.GetWidth() * aImage.GetHeight() ); ++i )
    {
        unsigned char v = *ori;

        ori++;

        dst[0] = 255;
        dst[1] = 255;
        dst[2] = 255;
        dst[3] = v;
        dst+= 4;
    }

    GLuint texture;
    glPixelStorei( GL_UNPACK_ALIGNMENT, 4 );
    glPixelStorei( GL_PACK_ALIGNMENT, 4 );

    glGenTextures( 1, &texture );
    glBindTexture( GL_TEXTURE_2D, texture );

    glTexImage2D( GL_TEXTURE_2D, 0, GL_RGBA, aImage.GetWidth(), aImage.GetHeight(), 0, GL_RGBA,
                  GL_UNSIGNED_BYTE, rgbaBuffer );

    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT );
    glTexParameterf( GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT );

    glTexEnvf( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    glBindTexture( GL_TEXTURE_2D, 0 );

    glFlush();

    free( rgbaBuffer );

    return texture;
}


void OglSetMaterial( const SMATERIAL&  aMaterial, float aOpacity, bool aUseSelectedMaterial,
                     SFVEC3F aSelectionColor )
{
    const SFVEC4F ambient  = SFVEC4F( aMaterial.m_Ambient, 1.0f );

    // !TODO: at this moment, diffuse color is added via
    // glEnableClientState( GL_COLOR_ARRAY ) so this line may has no effect
    // but can be used for optimization
    const SFVEC4F diffuse  = SFVEC4F( aUseSelectedMaterial ? aSelectionColor : aMaterial.m_Diffuse,
                                      ( 1.0f - aMaterial.m_Transparency ) * aOpacity );
    const SFVEC4F specular = SFVEC4F( aMaterial.m_Specular, 1.0f );
    const SFVEC4F emissive = SFVEC4F( aMaterial.m_Emissive, 1.0f );

    const float shininess =
            128.0f * ( (aMaterial.m_Shininess > 1.0f) ? 1.0f : aMaterial.m_Shininess );

    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &diffuse.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, shininess );
}


void OglSetDiffuseMaterial( const SFVEC3F &aMaterialDiffuse, float aOpacity,
                            bool aUseSelectedMaterial, SFVEC3F aSelectionColor  )
{
    const SFVEC4F ambient  = SFVEC4F( 0.2f, 0.2f, 0.2f, 1.0f );
    const SFVEC4F diffuse  = SFVEC4F( aUseSelectedMaterial ? aSelectionColor : aMaterialDiffuse,
                                      aOpacity );
    const SFVEC4F specular = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );
    const SFVEC4F emissive = SFVEC4F( 0.0f, 0.0f, 0.0f, 1.0f );

    glMaterialfv( GL_FRONT_AND_BACK, GL_AMBIENT,  &ambient.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE,  &diffuse.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_SPECULAR, &specular.r );
    glMaterialfv( GL_FRONT_AND_BACK, GL_EMISSION, &emissive.r );
    glMaterialf( GL_FRONT_AND_BACK, GL_SHININESS, 0.0f );
}


void OglDrawBackground( const SFVEC4F& aTopColor, const SFVEC4F& aBotColor )
{
    glMatrixMode( GL_PROJECTION );
    glLoadIdentity();

    glMatrixMode( GL_MODELVIEW );
    glLoadIdentity();

    glDisable( GL_LIGHTING );
    glDisable( GL_COLOR_MATERIAL );
    glDisable( GL_DEPTH_TEST );
    glDisable( GL_TEXTURE_2D );
    glDisable( GL_BLEND );
    glDisable( GL_ALPHA_TEST );

    glBegin( GL_QUADS );
    glColor4f( aTopColor.r, aTopColor.g, aTopColor.b, aTopColor.a );
    glVertex2f( -1.0, 1.0 );    // Top left corner

    glColor4f( aBotColor.r, aBotColor.g, aBotColor.b, aBotColor.a );
    glVertex2f( -1.0,-1.0 );    // bottom left corner
    glVertex2f( 1.0,-1.0 );     // bottom right corner

    glColor4f( aTopColor.r, aTopColor.g, aTopColor.b, aTopColor.a);
    glVertex2f( 1.0, 1.0 );     // top right corner
    glEnd();
}


void OglResetTextureState()
{
    if( !glActiveTexture || !glClientActiveTexture )
        throw std::runtime_error( "The OpenGL context no longer exists: unable to Reset Textures" );

    glActiveTexture( GL_TEXTURE0 );
    glBindTexture( GL_TEXTURE_2D, 0 );
    glClientActiveTexture( GL_TEXTURE0 );
    glDisable( GL_TEXTURE_2D );
    glTexEnvi( GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE );

    const SFVEC4F zero = SFVEC4F( 0.0f );

    glTexEnvfv( GL_TEXTURE_ENV, GL_TEXTURE_ENV_COLOR, static_cast<const float*>( &zero.x ) );
}
