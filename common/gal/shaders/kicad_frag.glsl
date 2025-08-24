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
const float SHADER_FILLED_CIRCLE        = 2.0;
const float SHADER_STROKED_CIRCLE       = 3.0;
const float SHADER_FONT                 = 4.0;
const float SHADER_LINE_A               = 5.0;
const float SHADER_HOLE_WALL            = 11.0;

varying vec4 v_shaderParams;
varying vec2 v_circleCoords;

uniform sampler2D u_fontTexture;
uniform float u_worldPixelSize;

// Needed to reconstruct the mipmap level / texel derivative
uniform int u_fontTextureWidth;

void filledCircle( vec2 aCoord )
{
    if( dot( aCoord, aCoord ) < 1.0 )
        gl_FragColor = gl_Color;
    else
        discard;
}

float pixelSegDistance( vec2 aCoord )
{
    float aspect = v_shaderParams[1];
    float dist;
    vec2 v = vec2( 1.0 - ( aspect - abs( aCoord.s ) ), aCoord.t );

    if( v.x <= 0.0 )
    {
        dist = abs( aCoord.t );
    }
    else
    {
        dist = length( v );
    }

    return dist;
}

int isPixelInSegment( vec2 aCoord )
{
    return pixelSegDistance( aCoord ) <= 1.0 ? 1 : 0;
}



void strokedCircle( vec2 aCoord, float aRadius, float aWidth )
{
    float outerRadius = max( aRadius, 0.0 );
    float innerRadius = max( aRadius - aWidth, 0.0 );

    if( ( dot( aCoord, aCoord ) < 1.0 ) &&
        ( dot( aCoord, aCoord ) * ( outerRadius * outerRadius ) > innerRadius * innerRadius ) )
        gl_FragColor = gl_Color;
    else
        discard;
}


void drawLine( vec2 aCoord )
{
    if( isPixelInSegment( aCoord ) != 0)
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
    // VS to FS pipeline does math that means we can't rely on the mode
    // parameter being bit-exact without rounding it first.
    float mode = floor( v_shaderParams[0] + 0.5 );

    if( mode == SHADER_LINE_A )
    {
        drawLine( gl_TexCoord[0].st );
    }
    else if( mode == SHADER_FILLED_CIRCLE )
    {
        filledCircle( v_circleCoords );
    }
    else if( mode == SHADER_STROKED_CIRCLE )
    {
        strokedCircle( v_circleCoords, v_shaderParams[2], v_shaderParams[3] );
    }
    else if( mode == SHADER_HOLE_WALL )
    {
        strokedCircle( v_circleCoords, v_shaderParams[2], v_shaderParams[3] );
    }
    else if( mode == SHADER_FONT )
    {
        vec2 tex           = v_shaderParams.yz;

        // Unless we're stretching chars it is okay to consider
        // one derivative for filtering
        float derivative   = length( dFdx( tex ) ) * u_fontTextureWidth / 4;

#ifdef USE_MSDF
        float dist         = median( texture2D( u_fontTexture, tex ).rgb );
#else
        float dist         = texture2D( u_fontTexture, tex ).r;
#endif

        // use the derivative for zoom-adaptive filtering
        float alpha = smoothstep( 0.5 - derivative, 0.5 + derivative, dist ) * gl_Color.a;

        gl_FragColor = vec4( gl_Color.rgb, alpha );
    }
    else
    {
        // Simple pass-through
        gl_FragColor = gl_Color;
    }
}