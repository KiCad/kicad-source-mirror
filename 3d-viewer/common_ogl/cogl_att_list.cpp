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
 * @file  cogl_att_list.cpp
 * @brief Implements a attribute list support for openGL
 */

#include "cogl_att_list.h"


/**
 *  Attributes list to be passed to a wxGLCanvas creation.
 *
 *  This array should be 2*n+1
 *  Sadly wxwidgets / glx < 13 allowed
 *  a thing named "boolean attributes" that don't take a value.
 *  (See src/unix/glx11.cpp -> wxGLCanvasX11::ConvertWXAttrsToGL() ).
 *  To avoid problems due to this, just specify those attributes twice.
 *  Only WX_GL_RGBA, WX_GL_DOUBLEBUFFER, WX_GL_STEREO are such boolean
 *  attributes.
 */
const int COGL_ATT_LIST::m_openGL_AttributesList[] = {
    // Boolean attributes (using itself at padding):
    WX_GL_RGBA,             WX_GL_RGBA,
    WX_GL_DOUBLEBUFFER,     WX_GL_DOUBLEBUFFER,

    // Normal attributes with values:
    WX_GL_DEPTH_SIZE,       16,
    WX_GL_STENCIL_SIZE,     1,
    WX_GL_SAMPLE_BUFFERS,   1,  // Enable multisampling support (antialiasing).
    WX_GL_SAMPLES,          0,  // Disable AA for the start.
    0                           // NULL termination
};


int COGL_ATT_LIST::m_openGL_AttributesList_toUse[
    sizeof( COGL_ATT_LIST::m_openGL_AttributesList ) /
    sizeof( COGL_ATT_LIST::m_openGL_AttributesList[0] ) ] = { 0 };


const int *COGL_ATT_LIST::GetAttributesList( bool aUseAntiAliasing )
{
    memcpy( m_openGL_AttributesList_toUse,
            m_openGL_AttributesList,
            sizeof( m_openGL_AttributesList_toUse ) );

    if( aUseAntiAliasing )
    {
        // Check if the canvas supports multisampling.
        if( wxGLCanvas::IsDisplaySupported( m_openGL_AttributesList_toUse ) )
        {
            // Check for possible sample sizes, start form the top.
            int maxSamples = 8; // Any higher doesn't change anything.
            int samplesOffset = 0;

            for( unsigned int ii = 0;
                 ii < DIM( m_openGL_AttributesList_toUse );
                 ii += 2 )
            {
                if( m_openGL_AttributesList_toUse[ii] == WX_GL_SAMPLES )
                {
                    samplesOffset = ii + 1;
                    break;
                }
            }

            m_openGL_AttributesList_toUse[samplesOffset] = maxSamples;

            for( ; maxSamples > 0 &&
                   !wxGLCanvas::IsDisplaySupported( m_openGL_AttributesList_toUse );
                maxSamples = maxSamples >> 1 )
            {
                m_openGL_AttributesList_toUse[samplesOffset] = maxSamples;
            }
        }
        else
            aUseAntiAliasing = false;
    }

    // Disable antialising if it failed or was not requested
    if( !aUseAntiAliasing )
    {
        // Disable multisampling
        for( unsigned int ii = 0;
             ii < DIM( m_openGL_AttributesList_toUse );
             ii += 2 )
        {
            if( m_openGL_AttributesList_toUse[ii] == WX_GL_SAMPLE_BUFFERS )
            {
                m_openGL_AttributesList_toUse[ii + 1] = 0;
                break;
            }
        }
    }

    return m_openGL_AttributesList_toUse;
}
