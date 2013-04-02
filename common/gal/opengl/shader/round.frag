/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
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

varying float aspect;

void main()
{
    vec2 v = abs( gl_TexCoord[0].xy - vec2( 0.5, 0.5 ) ) * 2.0 - vec2( aspect, 0.0 );
    vec2 d = vec2( v.x / ( 1.0 - aspect ), v.y );

    if( v.x <= 0.0 || (dot( d, d ) < 1.0 ) )
        gl_FragColor = gl_Color;
    else
        discard;

    // gl_FragColor = vec4(0.0, 0.0, 0.0, 0.0);
}
