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
 * @file ogl_attr_list.cpp
 * @brief Implements a attribute list support for openGL
 */

#include "ogl_attr_list.h"
#include <wx/glcanvas.h>
#include <wx/debug.h>
#include <core/arraydim.h>


const wxGLAttributes OGL_ATT_LIST::GetAttributesList( ANTIALIASING_MODE aAntiAliasingMode,
                                                      bool              aAlpha )
{
    wxASSERT( aAntiAliasingMode <= ANTIALIASING_MODE::AA_8X );

    auto makeAttribs = [aAlpha]( int aSamplers )
    {
        wxGLAttributes dispAttrs;

        dispAttrs.RGBA()
                .DoubleBuffer()
                .Depth( 16 )
                .Stencil( 8 )
                .Samplers( aSamplers )
                .SampleBuffers( aSamplers >= 0 ? 1 : -1 )
                .MinRGBA( 8, 8, 8, aAlpha ? 8 : -1 )
                .EndList();

        return dispAttrs;
    };

    int maxSamples = -1;

    if( aAntiAliasingMode > ANTIALIASING_MODE::AA_NONE )
    {
        // Check if the canvas supports multisampling.
        if( wxGLCanvas::IsDisplaySupported( makeAttribs( 0 ) ) )
        {
            static const int aaSamples[4] = { 0, 2, 4, 8 };

            // Check for possible sample sizes, start form the requested.
            maxSamples = aaSamples[static_cast<int>( aAntiAliasingMode )];

            while( maxSamples > 0 && !wxGLCanvas::IsDisplaySupported( makeAttribs( maxSamples ) ) )
            {
                maxSamples = maxSamples >> 1;
            }
        }
    }

    return makeAttribs( maxSamples );
}
