/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * Geometry shader
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
#extension GL_EXT_geometry_shader4: enable
#extension GL_EXT_gpu_shader4: enable

uniform float viewPortX2;
uniform float viewPortY2;
varying float dist;

void main()
{
    // Compute the transformed start and end points
    vec2    startPoint = gl_PositionIn[0].xy;
    vec2    endPoint   = gl_PositionIn[1].xy;
    float   lineWidth  = gl_PositionIn[1].z;

    // Compute vector start -> end
    vec2    startEndVector = endPoint.xy - startPoint.xy;
    float   lineLength     = distance( startPoint, endPoint );
    float   scale = 0.0;

    if( lineLength > 0.0 )
    {
        scale = 0.5 * lineWidth / lineLength;
    }
    else
    {
        scale = 0.0;
    }

    // Compute the edge points of the line
    vec2    perpendicularVector = scale * vec2( -startEndVector.y, startEndVector.x );
    vec2    point1  = startPoint + perpendicularVector;
    vec2    point2  = startPoint - perpendicularVector;
    vec2    point3  = endPoint + perpendicularVector;
    vec2    point4  = endPoint - perpendicularVector;

    vec4    point1T = gl_ModelViewProjectionMatrix * vec4( point1, gl_PositionIn[0].zw );
    vec4    point2T = gl_ModelViewProjectionMatrix * vec4( point2, gl_PositionIn[0].zw );
    vec4    point3T = gl_ModelViewProjectionMatrix * vec4( point3, gl_PositionIn[0].zw );
    vec4    point4T = gl_ModelViewProjectionMatrix * vec4( point4, gl_PositionIn[0].zw );

    // Construct the quad for the middle
    gl_FrontColor = gl_FrontColorIn[0];
    dist = 0;
    gl_Position = point1T;
    EmitVertex();
    dist = 0;
    gl_Position = point2T;
    EmitVertex();
    dist = 0;
    gl_Position = point3T;
    EmitVertex();
    dist = 0;
    gl_Position = point4T;
    EmitVertex();

    EndPrimitive();

    // Compute the perpendicular vector with 1 pixel width
    vec2 v      = point1T.xy - point3T.xy;
    vec4 onePix = 0.5 * vec4( -v.y, v.x, 0, 0 );
    onePix     *= 1.0 / sqrt( dot( onePix, onePix ) );
    onePix.x   *= 1.0 / viewPortX2;
    onePix.y   *= 1.0 / viewPortY2;

    gl_FrontColor = gl_FrontColorIn[0];

    dist = 1;
    gl_Position = point1T + onePix;
    EmitVertex();
    dist = 1;
    gl_Position = point3T + onePix;
    EmitVertex();
    dist = 0;
    gl_Position = point1T;
    EmitVertex();
    dist = 0;
    gl_Position = point3T;
    EmitVertex();

    EndPrimitive();

    dist = 1;
    gl_Position = point2T - onePix;
    EmitVertex();
    dist = 1;
    gl_Position = point4T - onePix;
    EmitVertex();
    dist = 0;
    gl_Position = point2T;
    EmitVertex();
    dist = 0;
    gl_Position = point4T;
    EmitVertex();

    EndPrimitive();
}
