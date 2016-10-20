/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013-2016 CERN
 * Copyright (C) 2016 Kicad Developers, see authors.txt for contributors.
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

// Multi-channel signed distance field
#define USE_MSDF

// Shader types
const float SHADER_LINE                 = 1.0;
const float SHADER_FILLED_CIRCLE        = 2.0;
const float SHADER_STROKED_CIRCLE       = 3.0;
const float SHADER_FONT                 = 4.0;

varying vec4 shaderParams;
varying vec2 circleCoords;
uniform sampler2D fontTexture;

// Needed to reconstruct the mipmap level / texel derivative
uniform int fontTextureWidth;

void filledCircle( vec2 aCoord )
{
    if( dot( aCoord, aCoord ) < 1.0 )
        gl_FragColor = gl_Color;
    else
        discard;
}


void strokedCircle( vec2 aCoord, float aRadius, float aWidth )
{
    float outerRadius = aRadius + ( aWidth / 2 );
    float innerRadius = aRadius - ( aWidth / 2 );
    float relWidth = innerRadius / outerRadius;

    if( ( dot( aCoord, aCoord ) < 1.0 ) &&
        ( dot( aCoord, aCoord ) > relWidth * relWidth ) )
        gl_FragColor = gl_Color;
    else
        discard;
}

#ifdef USE_MSDF
float median( vec3 v )
{
    return max( min( v.r, v.g ), min( max( v.r, v.g ), v.b ) );
}
#endif

void main()
{
    if( shaderParams[0] == SHADER_FILLED_CIRCLE )
    {
        filledCircle( circleCoords );
    }
    else if( shaderParams[0] == SHADER_STROKED_CIRCLE )
    {
        strokedCircle( circleCoords, shaderParams[2], shaderParams[3] );
    }
    else if( shaderParams[0] == SHADER_FONT )
    {
        vec2 tex           = shaderParams.yz;

        // Unless we're streching chars it is okay to consider
        // one derivative for filtering
        float derivative   = length( dFdx( tex ) ) * fontTextureWidth / 4;

#ifdef USE_MSDF
        float dist         = median( texture2D( fontTexture, tex ).rgb );
#else
        float dist         = texture2D( fontTexture, tex ).r;
#endif

        // use the derivative for zoom-adaptive filtering
        float alpha = smoothstep( 0.5 - derivative, 0.5 + derivative, dist );

        gl_FragColor = vec4( gl_Color.rgb, alpha );
    }
    else
    {
        // Simple pass-through
        gl_FragColor = gl_Color;
    }
}
