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
const float SHADER_LINE                 = 1.0f;
const float SHADER_FILLED_CIRCLE        = 2.0f;
const float SHADER_STROKED_CIRCLE       = 3.0f;

// Minimum line width
const float MIN_WIDTH = 1.0f;

attribute vec4 attrShaderParams;
varying vec4 shaderParams;
varying vec2 circleCoords;

void main()
{
    // Pass attributes to the fragment shader
    shaderParams = attrShaderParams;
    
    if( shaderParams[0] == SHADER_LINE )
    {
        float lineWidth = shaderParams[3];
        float worldScale = gl_ModelViewMatrix[0][0];
        float scale;
     
        // Make lines appear to be at least 1 pixel wide
        if( worldScale * lineWidth < MIN_WIDTH )
            scale = MIN_WIDTH / ( worldScale * lineWidth );
        else
            scale = 1.0f;
        
        gl_Position = gl_ModelViewProjectionMatrix * 
            ( gl_Vertex + vec4( shaderParams.yz * scale, 0.0, 0.0 ) );
    }
    else if( ( shaderParams[0] == SHADER_STROKED_CIRCLE ) || 
             ( shaderParams[0] == SHADER_FILLED_CIRCLE  ) )
    {
        // Compute relative circle coordinates basing on indices
        // Circle
        if( shaderParams[1] == 1.0f )
            circleCoords = vec2( -sqrt( 3.0f ), -1.0f );
        else if( shaderParams[1] == 2.0f )
            circleCoords = vec2( sqrt( 3.0f ), -1.0f );
        else if( shaderParams[1] == 3.0f )
            circleCoords = vec2( 0.0f, 2.0f );
            
        // Semicircle
        else if( shaderParams[1] == 4.0f )
            circleCoords = vec2( -3.0f / sqrt( 3.0f ), 0.0f );
        else if( shaderParams[1] == 5.0f )
            circleCoords = vec2( 3.0f / sqrt( 3.0f ), 0.0f );
        else if( shaderParams[1] == 6.0f )
            circleCoords = vec2( 0.0f, 2.0f );

        // Make the line appear to be at least 1 pixel wide
        float lineWidth = shaderParams[3];
        float worldScale = gl_ModelViewMatrix[0][0];
     
        // Make lines appear to be at least 1 pixel width
        if( worldScale * lineWidth < MIN_WIDTH )
            shaderParams[3] = shaderParams[3] / ( worldScale * lineWidth );

        gl_Position = ftransform();
    }
    else
    {
        // Pass through the coordinates like in the fixed pipeline
        gl_Position = ftransform();
    }
    
    gl_FrontColor = gl_Color;
}

