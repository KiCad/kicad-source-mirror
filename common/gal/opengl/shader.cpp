/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2012 Torsten Hueter, torstenhtr <at> gmx.de
 * Copyright (C) 2012 Kicad Developers, see change_log.txt for contributors.
 *
 * Graphics Abstraction Layer (GAL) for OpenGL
 *
 * Shader class
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

#include <iostream>
#include <fstream>
#include <stdexcept>

#include <cstring>
#include <cassert>

#include <gal/opengl/shader.h>
#include "shader_src.h"

using namespace KIGFX;

SHADER::SHADER() :
    isProgramCreated( false ),
    isShaderLinked( false ),
    active( false ),
    maximumVertices( 4 ),
    geomInputType( GL_LINES ),
    geomOutputType( GL_LINES )
{
}


SHADER::~SHADER()
{
    if( isProgramCreated )
    {
        // Delete the shaders and the program
        for( std::deque<GLuint>::iterator it = shaderNumbers.begin(); it != shaderNumbers.end();
             ++it )
        {
            glDeleteShader( *it );
        }

        glDeleteProgram( programNumber );
    }
}


bool SHADER::LoadBuiltinShader( unsigned int aShaderNumber, SHADER_TYPE aShaderType )
{
    if( aShaderNumber >= shaders_number )
        return false;

    return addSource( std::string( shaders_src[aShaderNumber] ), aShaderType );
}


bool SHADER::LoadShaderFromFile( const std::string& aShaderSourceName, SHADER_TYPE aShaderType )
{
    // Load shader sources
    const std::string shaderSource = readSource( aShaderSourceName );

    return addSource( shaderSource, aShaderType );
}


void SHADER::ConfigureGeometryShader( GLuint maxVertices, GLuint geometryInputType,
                                      GLuint geometryOutputType )
{
    maximumVertices = maxVertices;
    geomInputType   = geometryInputType;
    geomOutputType  = geometryOutputType;
}


bool SHADER::Link()
{
    // Shader linking
    glLinkProgram( programNumber );
    programInfo( programNumber );

    // Check the Link state
    glGetObjectParameterivARB( programNumber, GL_OBJECT_LINK_STATUS_ARB,
                               (GLint*) &isShaderLinked );

#ifdef DEBUG
    if( !isShaderLinked )
    {
        int maxLength;
        glGetProgramiv( programNumber, GL_INFO_LOG_LENGTH, &maxLength );
        maxLength = maxLength + 1;
        char* linkInfoLog = new char[maxLength];
        glGetProgramInfoLog( programNumber, maxLength, &maxLength, linkInfoLog );
        std::cerr << "Shader linking error:" << std::endl;
        std::cerr << linkInfoLog;
        delete[] linkInfoLog;
    }
#endif /* DEBUG */

    return isShaderLinked;
}


int SHADER::AddParameter( const std::string& aParameterName )
{
    GLint location = glGetUniformLocation( programNumber, aParameterName.c_str() );

    if( location != -1 )
        parameterLocation.push_back( location );

    return location;
}


void SHADER::SetParameter( int parameterNumber, float value ) const
{
    glUniform1f( parameterLocation[parameterNumber], value );
}


void SHADER::SetParameter( int parameterNumber, int value ) const
{
    glUniform1i( parameterLocation[parameterNumber], value );
}


int SHADER::GetAttribute( std::string aAttributeName ) const
{
    return glGetAttribLocation( programNumber, aAttributeName.c_str() );
}


void SHADER::programInfo( GLuint aProgram )
{
    GLint glInfoLogLength = 0;
    GLint writtenChars    = 0;

    // Get the length of the info string
    glGetProgramiv( aProgram, GL_INFO_LOG_LENGTH, &glInfoLogLength );

    // Print the information
    if( glInfoLogLength > 2 )
    {
        GLchar* glInfoLog = new GLchar[glInfoLogLength];
        glGetProgramInfoLog( aProgram, glInfoLogLength, &writtenChars, glInfoLog );

        std::cerr << glInfoLog << std::endl;

        delete[] glInfoLog;
    }
}


void SHADER::shaderInfo( GLuint aShader )
{
    GLint glInfoLogLength = 0;
    GLint writtenChars    = 0;

    // Get the length of the info string
    glGetShaderiv( aShader, GL_INFO_LOG_LENGTH, &glInfoLogLength );

    // Print the information
    if( glInfoLogLength > 2 )
    {
        GLchar* glInfoLog = new GLchar[glInfoLogLength];
        glGetShaderInfoLog( aShader, glInfoLogLength, &writtenChars, glInfoLog );

        std::cerr << glInfoLog << std::endl;

        delete[] glInfoLog;
    }
}


std::string SHADER::readSource( std::string aShaderSourceName )
{
    // Open the shader source for reading
    std::ifstream inputFile( aShaderSourceName.c_str(), std::ifstream::in );
    std::string   shaderSource;

    if( !inputFile )
        throw std::runtime_error( "Can't read the shader source: " + aShaderSourceName );

    std::string shaderSourceLine;

    // Read all lines from the text file
    while( getline( inputFile, shaderSourceLine ) )
    {
        shaderSource += shaderSourceLine;
        shaderSource += "\n";
    }

    return shaderSource;
}


bool SHADER::addSource( const std::string& aShaderSource, SHADER_TYPE aShaderType )
{
    assert( !isShaderLinked );

    // Create the program
    if( !isProgramCreated )
    {
        programNumber    = glCreateProgram();
        isProgramCreated = true;
    }

    // Create a shader
    GLuint shaderNumber = glCreateShader( aShaderType );
    shaderNumbers.push_back( shaderNumber );

    // Get the program info
    programInfo( programNumber );

    // Copy to char array
    char* source = new char[aShaderSource.size() + 1];
    strncpy( source, aShaderSource.c_str(), aShaderSource.size() + 1 );
    const char** source_ = (const char**) ( &source );

    // Attach the source
    glShaderSource( shaderNumber, 1, source_, NULL );
    programInfo( programNumber );

    // Delete the allocated char array
    delete[] source;

    // Compile and attach shader to the program
    glCompileShader( shaderNumber );
    GLint status;
    glGetShaderiv( shaderNumber, GL_COMPILE_STATUS, &status );

    if( status != GL_TRUE )
    {
        shaderInfo( shaderNumber );
        throw std::runtime_error( "Shader compilation error" );
    }

    glAttachShader( programNumber, shaderNumber );
    programInfo( programNumber );

    // Special handling for the geometry shader
    if( aShaderType == SHADER_TYPE_GEOMETRY )
    {
        glProgramParameteriEXT( programNumber, GL_GEOMETRY_VERTICES_OUT_EXT, maximumVertices );
        glProgramParameteriEXT( programNumber, GL_GEOMETRY_INPUT_TYPE_EXT, geomInputType );
        glProgramParameteriEXT( programNumber, GL_GEOMETRY_OUTPUT_TYPE_EXT, geomOutputType );
    }

    return true;
}
