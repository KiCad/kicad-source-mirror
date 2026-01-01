/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * XOR/Difference mode fragment shader for gerbview layer comparison
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

uniform sampler2D srcTex;
uniform sampler2D dstTex;

void main()
{
    vec2 tc = gl_TexCoord[0].st;
    vec4 srcColor = texture2D( srcTex, tc );
    vec4 dstColor = texture2D( dstTex, tc );

    // XOR/Difference mode: absolute difference of color channels
    // Identical overlapping content cancels to black (0,0,0)
    // Different content shows the difference
    vec3 diff = abs( srcColor.rgb - dstColor.rgb );

    // Use maximum alpha from either layer
    float alpha = max( srcColor.a, dstColor.a );

    gl_FragColor = vec4( diff, alpha );
}
