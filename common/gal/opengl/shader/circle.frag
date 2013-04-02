/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
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

// This shader requires GLSL 1.2
#version 120

// Input variables
flat varying vec4 center_;
flat varying vec2 radius_;
flat varying vec4 colorA_;
flat varying vec4 colorB_;

void main( void )
{
    // Compute the distance from the circle edge
    float   distA   = distance( center_, gl_FragCoord ) - radius_.y;
    float   distB   = radius_.x - distance( center_, gl_FragCoord );

    // Limit the range to [ 0 .. 1 ]
    if( distA < 0 ) distA = 0;
    if( distA > 1 ) distA = 1;
    if( distB < 0 ) distB = 0;
    if( distB > 1 ) distB = 1;

    // Points with a larger distance from the edge are set deeper
    gl_FragDepth = gl_FragCoord.z + distA * 0.001 + distB * 0.001;

    // Compute the color
    vec4 color;
    color.r = colorA_.r;
    color.g = colorA_.g;
    color.b = colorA_.b;
    color.a = colorA_.a * ( 1 - distA ) * ( 1 - distB );

    // Now output the edge fragment color
    gl_FragColor = color;
}
