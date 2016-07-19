/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2016 Mario Luzeiro <mrluzeiro@ua.pt>
 * Copyright (C) 1992-2016 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <fctsys.h>

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
const int COGL_ATT_LIST::m_openGL_attributes_list[] = {

    // Boolean attributes (using itself at padding):

    // 0                    1
    WX_GL_RGBA,           WX_GL_RGBA,
    // 2                    3
    WX_GL_DOUBLEBUFFER,     WX_GL_DOUBLEBUFFER,


    // Normal attributes with values:

    // 4                    5
    WX_GL_DEPTH_SIZE,       16,
    // 6                    7
    WX_GL_STENCIL_SIZE,     8,


    // This ones need to be the last in the list (as the tags will set to 0 if AA fails)

    // 8                    9
    WX_GL_SAMPLES,          0,  // Disable AA for the start.
    //10                    11
    WX_GL_SAMPLE_BUFFERS,   1,  // Enable multisampling support (antialiasing).

    0,                      0   // NULL termination
};

#define ATT_WX_GL_SAMPLES_OFFSET 8
#define ATT_WX_GL_SAMPLES_OFFSET_DATA 9
#define ATT_WX_GL_SAMPLE_BUFFERS_OFFSET 10
#define ATT_WX_GL_SAMPLE_BUFFERS_DATA 11

int COGL_ATT_LIST::m_openGL_attributes_list_to_use[
                                        DIM( COGL_ATT_LIST::m_openGL_attributes_list ) ] = { 0 };


const int *COGL_ATT_LIST::GetAttributesList( bool aUseAntiAliasing )
{
    memcpy( m_openGL_attributes_list_to_use,
            m_openGL_attributes_list,
            sizeof( m_openGL_attributes_list_to_use ) );

    if( aUseAntiAliasing )
    {
        // There is a bug on wxGLCanvas that makes IsDisplaySupported fail
        // while testing for antialiasing.
        // http://trac.wxwidgets.org/ticket/16909
        // this next code will only work after this bug is fixed
        //
        // On my experience (Mario) it was only working on Linux but failing on
        // Windows, so there was no AA.


        // Check if the canvas supports multisampling.
        if( wxGLCanvas::IsDisplaySupported( m_openGL_attributes_list_to_use ) )
        {
            // Check for possible sample sizes, start form the top.
            int maxSamples = 8; // Any higher doesn't change anything.

            m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLES_OFFSET_DATA] = maxSamples;

            for( ; (maxSamples > 0) &&
                   ( !wxGLCanvas::IsDisplaySupported( m_openGL_attributes_list_to_use ) );
                maxSamples = maxSamples >> 1 )
            {
                m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLES_OFFSET_DATA] = maxSamples;
            }
        }
        else
        {
            DBG( printf("GetAttributesList: AntiAliasing is not supported.\n") );
            aUseAntiAliasing = false;
        }
    }

    // Disable antialising if it failed or was not requested
    if( !aUseAntiAliasing )
    {
        // Remove multisampling information
        // (hoping that the GPU driver will decide what is best)
        m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLES_OFFSET]        = 0;
        m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLES_OFFSET_DATA]   = 0;
        m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLE_BUFFERS_OFFSET] = 0;
        m_openGL_attributes_list_to_use[ATT_WX_GL_SAMPLE_BUFFERS_DATA]   = 0;
    }

    return m_openGL_attributes_list_to_use;
}
