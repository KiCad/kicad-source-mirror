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

#ifndef SHADER_H_
#define SHADER_H_

#include <GL/glew.h>

#include <string>
#include <deque>

namespace KIGFX
{
class OPENGL_GAL;

/// Type definition for the shader
enum SHADER_TYPE
{
    SHADER_TYPE_VERTEX	 = GL_VERTEX_SHADER,    ///< Vertex shader
    SHADER_TYPE_FRAGMENT = GL_FRAGMENT_SHADER,  ///< Fragment shader
    SHADER_TYPE_GEOMETRY = GL_GEOMETRY_SHADER   ///< Geometry shader
};

/**
 * @brief Class SHADER provides the access to the OpenGL shaders.
 *
 * The purpose of this class is advanced drawing with OpenGL. One example is using the pixel
 * shader for drawing exact circles or for anti-aliasing. This class supports vertex, geometry
 * and fragment shaders.
 * <br>
 * Make sure that the hardware supports these features. This can be identified with the "GLEW"
 * library.
 */
class SHADER
{
public:

    /**
     * @brief Constructor
     */
    SHADER();

    /**
     * @brief Destructor
     */
    virtual ~SHADER();

    /**
     * @brief Loads one of the built-in shaders and compiles it.
     *
     * @param aShaderNumber is the shader number (indexing from 0).
     * @param aShaderType is the type of the shader.
     * @return True in case of success, false otherwise.
     */
    bool LoadBuiltinShader( unsigned int aShaderNumber, SHADER_TYPE aShaderType );

    /**
     * @brief Loads one of the built-in shaders and compiles it.
     *
     * @param aShaderSourceName is the shader source file name.
     * @param aShaderType is the type of the shader.
     * @return True in case of success, false otherwise.
     */
    bool LoadShaderFromFile( const std::string& aShaderSourceName, SHADER_TYPE aShaderType );

    /**
     * @brief Link the shaders.
     *
     * @return true in case of success, false otherwise.
     */
    bool Link();

    /**
     * @brief Returns true if shaders are linked correctly.
     */
    bool IsLinked() const
    {
        return isShaderLinked;
    }

    /**
     * @brief Use the shader.
     */
    inline void Use()
    {
        glUseProgram( programNumber );
        active = true;
    }

    /**
     * @brief Deactivate the shader and use the default OpenGL program.
     */
    inline void Deactivate()
    {
        glUseProgram( 0 );
        active = false;
    }

    /**
     * @brief Returns the current state of the shader.
     *
     * @return True if any of shaders is enabled.
     */
    inline bool IsActive() const
    {
        return active;
    }

    /**
     * @brief Configure the geometry shader - has to be done before linking!
     *
     * @param maxVertices is the maximum of vertices to be generated.
     * @param geometryInputType is the input type [e.g. GL_LINES, GL_TRIANGLES, GL_QUADS etc.]
     * @param geometryOutputType is the output type [e.g. GL_LINES, GL_TRIANGLES, GL_QUADS etc.]
     */
    void ConfigureGeometryShader( GLuint maxVertices, GLuint geometryInputType,
                                  GLuint geometryOutputType );

    /**
     * @brief Add a parameter to the parameter queue.
     *
     * To communicate with the shader use this function to set up the names for the uniform
     * variables. These are queued in a list and can be assigned with the SetParameter(..)
     * method using the queue position.
     *
     * @param aParameterName is the name of the parameter.
     * @return the added parameter location.
     */
    int AddParameter( const std::string& aParameterName );

    /**
     * @brief Set a parameter of the shader.
     *
     * @param aParameterNumber is the number of the parameter.
     * @param aValue is the value of the parameter.
     */
    void SetParameter( int aParameterNumber, float aValue ) const;
    void SetParameter( int aParameterNumber, int aValue ) const;

    /**
     * @brief Gets an attribute location.
     *
     * @param aAttributeName is the name of the attribute.
     * @return the location.
     */
    int GetAttribute( std::string aAttributeName ) const;

private:

    /**
     * @brief Get the shader program information.
     *
     * @param aProgram is the program number.
     */
    void programInfo( GLuint aProgram );

    /**
     * @brief Get the shader information.
     *
     * @param aShader is the shader number.
     */
    void shaderInfo( GLuint aShader );

    /**
     * @brief Read the shader source file
     *
     * @param aShaderSourceName is the shader source file name.
     * @return the source as string
     */
    std::string readSource( std::string aShaderSourceName );

    /**
     * @brief Add a shader and compile the shader sources.
     *
     * @param aShaderSource is the shader source content.
     * @param aShaderType is the type of the shader.
     * @return True in case of success, false otherwise.
     */
    bool addSource( const std::string& aShaderSource, SHADER_TYPE aShaderType );

    std::deque<GLuint>  shaderNumbers;      ///< Shader number list
    GLuint              programNumber;      ///< Shader program number
    bool                isProgramCreated;   ///< Flag for program creation
    bool                isShaderLinked;     ///< Is the shader linked?
    bool                active;             ///< Is any of shaders used?
    GLuint              maximumVertices;    ///< The maximum of vertices to be generated
    GLuint              geomInputType;      ///< Input type [e.g. GL_LINES, GL_TRIANGLES, GL_QUADS etc.]
    GLuint              geomOutputType;     ///< Output type [e.g. GL_LINES, GL_TRIANGLES, GL_QUADS etc.]
    std::deque<GLint>   parameterLocation;  ///< Location of the parameter
};
} // namespace KIGFX

#endif /* SHADER_H_ */
