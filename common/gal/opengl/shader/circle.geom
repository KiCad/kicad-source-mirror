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
 
// This shader requires GLSL 1.2
#version 120
#extension GL_EXT_geometry_shader4: enable
#extension GL_EXT_gpu_shader4: enable

uniform float viewPortX2;
uniform float viewPortY2;

flat varying vec4 center_;
flat varying vec2 radius_;
flat varying vec4 colorA_;

const float PI = 3.141592654;
const float EPSILON = 0.01;
const float smallestValue = 1.175494351e-38;
const int SEGMENTS = 16;

const float PIXEL_EXTEND = 1.5;

void main()
{
    vec4 center = gl_PositionIn[0];
    vec4 radius = gl_PositionIn[1];

    center_ = gl_ModelViewProjectionMatrix * center;

    // Compute the outer and inner radius in screen coordinates
    // This could be further optimized
    radius_.x = ( gl_ModelViewProjectionMatrix * vec4(radius.x, 0, 0, 1) ).x;
    radius_.x = abs( radius_.x - (gl_ModelViewProjectionMatrix * vec4(0, 0, 0, 1) ).x ) * viewPortX2;
    radius_.y = ( gl_ModelViewProjectionMatrix * vec4(radius.y, 0, 0, 1) ).x;
    radius_.y = abs( radius_.y - (gl_ModelViewProjectionMatrix * vec4(0, 0, 0, 1) ).x ) * viewPortX2;

    // Compute the center point in screen coordinates
    center_.x = center_.x * viewPortX2 + viewPortX2;
    center_.y = center_.y * viewPortY2 + viewPortY2;

    // Compute the extend value, first make sure that the outline is inside the triangles and second add
    // a margin for one pixel for smooth edges
    float extendInner = 1.0;
    float extendOuter = 0;
    if( radius_.y > smallestValue )
    {
        extendOuter += PIXEL_EXTEND / radius_.y;
    }
    extendOuter += 1.0 / cos( PI / SEGMENTS );

    colorA_ = gl_FrontColorIn[0];

    // Create a quad strip for the outer circle edge
    for( float alpha = 0, inc = 2 * PI / SEGMENTS, limit = 2 * PI + EPSILON;
         alpha < limit; alpha += inc )
    {
        gl_Position = gl_ModelViewProjectionMatrix *
                vec4( center.x + extendInner * radius.y * cos( alpha ),
                      center.y + extendInner * radius.y * sin( alpha ), center.zw );
        EmitVertex();
        gl_Position = gl_ModelViewProjectionMatrix *
                vec4( center.x + extendOuter * radius.y * cos( alpha ),
                      center.y + extendOuter * radius.y * sin( alpha ), center.zw );
        EmitVertex();
    }
    EndPrimitive();

    if( radius.x > 0 )
    {
        extendInner = cos( PI / SEGMENTS ) - PIXEL_EXTEND / radius_.x;
        if( extendInner < 0.0 )
        {
            extendInner = 0;
        }
        extendOuter = 1.0 / cos( PI / SEGMENTS);

        // Create a quad strip for the inner circle edge
        for( float alpha = 0, inc = 2 * PI / SEGMENTS, limit = 2 * PI + EPSILON;
             alpha < limit; alpha += inc )
        {
            gl_Position = gl_ModelViewProjectionMatrix *
                    vec4( center.x + extendOuter * radius.x * cos( alpha ),
                          center.y + extendOuter * radius.x * sin( alpha ), center.zw );
            EmitVertex();
            gl_Position = gl_ModelViewProjectionMatrix *
                    vec4( center.x + extendInner * radius.x * cos( alpha ),
                          center.y + extendInner * radius.x * sin( alpha ), center.zw );
            EmitVertex();
        }
        EndPrimitive();
    }
}
