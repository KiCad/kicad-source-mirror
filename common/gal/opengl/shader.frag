/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Fragment shader
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

#version 120

// Shader types
const float SHADER_LINE                 = 1.0;
const float SHADER_FILLED_CIRCLE        = 2.0;
const float SHADER_STROKED_CIRCLE       = 3.0;

varying vec4 shaderParams;

void filledCircle( vec2 aCoord )
{
    if( dot( aCoord, aCoord ) < 1.0 )
        gl_FragColor = gl_Color;
    else
        discard;
}


void strokedCircle( vec2 aCoord, float aWidth )
{
    if( ( dot( aCoord, aCoord ) < 1.0 ) && 
        ( dot( aCoord, aCoord ) > aWidth * aWidth ) )
        gl_FragColor = gl_Color;
    else
        discard;
}


void main()
{
    if( shaderParams[0] == SHADER_FILLED_CIRCLE )
    {
        filledCircle( vec2( shaderParams[1], shaderParams[2] ) );
    }
    else if( shaderParams[0] == SHADER_STROKED_CIRCLE )
    {
        strokedCircle( vec2( shaderParams[1], shaderParams[2] ), shaderParams[3] );
    }
    else
    {
        // Simple pass-through
        gl_FragColor = gl_Color;
    }
}

