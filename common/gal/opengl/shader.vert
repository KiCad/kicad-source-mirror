/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2013 CERN
 * @author Maciej Suminski <maciej.suminski@cern.ch>
 *
 * Vertex shader
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

attribute vec4 attrShaderParams;
varying vec4 shaderParams;

void main()
{
    // Pass attributes to the fragment shader
    shaderParams = attrShaderParams;
    
    if( shaderParams[0] == SHADER_LINE )
    {
        float lineWidth = shaderParams[3];
        float worldScale = gl_ModelViewMatrix[0][0];
        float scale;
     
        // Make lines appear to be at least 1 pixel width
        if( worldScale * lineWidth < 1.0 )
            scale = 1.0 / ( worldScale * lineWidth );
        else
            scale = 1.0;
        
        gl_Position = gl_ModelViewProjectionMatrix * 
            ( gl_Vertex + vec4( shaderParams.yz * scale, 0.0, 0.0 ) );
    }
    else
    {
        // Pass through the coordinates like in the fixed pipeline
        gl_Position = ftransform();
    }
    
    gl_FrontColor = gl_Color;
}

