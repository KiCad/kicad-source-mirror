/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016-2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <confirm.h> // DisplayError

#include <gal/opengl/kiglew.h> // Must be included first

#include <stdexcept>
#include <wx/log.h> // wxLogTrace


/**
 * Flag to enable debug output of the GAL OpenGL error checking.
 *
 * Use "KICAD_GAL_OPENGL_ERROR" to enable GAL OpenGL error tracing.
 *
 * @ingroup trace_env_vars
 */
static const wxChar* const traceGalOpenGlError = wxT( "KICAD_GAL_OPENGL_ERROR" );


int checkGlError( const std::string& aInfo, const char* aFile, int aLine, bool aThrow )
{
    int      result = glGetError();
    wxString errorMsg;

    switch( result )
    {
    case GL_NO_ERROR:
        // all good
        break;

    case GL_INVALID_ENUM:
        errorMsg = wxString::Format( "Error: %s: invalid enum", aInfo );
        break;

    case GL_INVALID_VALUE:
        errorMsg = wxString::Format( "Error: %s: invalid value", aInfo );
        break;

    case GL_INVALID_OPERATION:
        errorMsg = wxString::Format( "Error: %s: invalid operation", aInfo );
        break;

    case GL_INVALID_FRAMEBUFFER_OPERATION:
    {
        GLenum status = glCheckFramebufferStatusEXT( GL_FRAMEBUFFER_EXT );

        if( status != GL_FRAMEBUFFER_COMPLETE_EXT )
        {
            switch( status )
            {
            case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT_EXT:
                errorMsg = "The framebuffer attachment points are incomplete.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT_EXT:
                errorMsg = "No images attached to the framebuffer.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER_EXT:
                errorMsg = "The framebuffer does not have at least one image attached to it.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER_EXT:
                errorMsg = "The framebuffer read buffer is incomplete.";
                break;

            case GL_FRAMEBUFFER_UNSUPPORTED_EXT:
                errorMsg = "The combination of internal formats of the attached images violates "
                           "an implementation dependent set of restrictions.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_MULTISAMPLE_EXT:
                errorMsg = "GL_RENDERBUFFER_SAMPLES is not the same for all attached render "
                           "buffers.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_LAYER_TARGETS_EXT:
                errorMsg = "Framebuffer incomplete layer targets errors.";
                break;

            case GL_FRAMEBUFFER_INCOMPLETE_DIMENSIONS_EXT:
                errorMsg = "Framebuffer attachments have different dimensions";
                break;

            default:
                errorMsg.Printf( "Unknown incomplete framebuffer error id %X", status );
            }
        }
        else
        {
            errorMsg = wxString::Format( "Error: %s: invalid framebuffer operation", aInfo );
        }
    }
    break;

    case GL_OUT_OF_MEMORY:
        errorMsg = wxString::Format( "Error: %s: out of memory", aInfo );
        break;

    case GL_STACK_UNDERFLOW:
        errorMsg = wxString::Format( "Error: %s: stack underflow", aInfo );
        break;

    case GL_STACK_OVERFLOW:
        errorMsg = wxString::Format( "Error: %s: stack overflow", aInfo );
        break;

    default:
        errorMsg = wxString::Format( "Error: %s: unknown error", aInfo );
        break;
    }

    if( result != GL_NO_ERROR )
    {
        if( aThrow )
        {
            wxLogTrace( traceGalOpenGlError, wxT( "Throwing exception for glGetError() '%s' "
                                                  "in file '%s' on line %d." ),
                        errorMsg,
                        aFile,
                        aLine );

            throw std::runtime_error( (const char*) errorMsg.char_str() );
        }
        else
        {
            wxString msg = wxString::Format( wxT( "glGetError() '%s' in file '%s' on line %d." ),
                                             errorMsg,
                                             aFile,
                                             aLine );

            DisplayErrorMessage( nullptr, "OpenGL Error", errorMsg );
        }
    }

    return result;
}


// debugMsgCallback is a callback function for glDebugMessageCallback.
// It must have the right type ( GLAPIENTRY )
static void GLAPIENTRY debugMsgCallback( GLenum aSource, GLenum aType, GLuint aId, GLenum aSeverity,
                                         GLsizei aLength, const GLchar* aMessage,
                                         const void* aUserParam )
{
    switch( aSeverity )
    {
    case GL_DEBUG_SEVERITY_HIGH:
        wxLogTrace( traceGalOpenGlError, wxS( "OpenGL ERROR: %s" ), aMessage );
        break;
    case GL_DEBUG_SEVERITY_MEDIUM:
        wxLogTrace( traceGalOpenGlError, wxS( "OpenGL WARNING: %s" ), aMessage );
        break;
    case GL_DEBUG_SEVERITY_LOW:
        wxLogTrace( traceGalOpenGlError, wxS( "OpenGL INFO: %s" ), aMessage );
        break;
    case GL_DEBUG_SEVERITY_NOTIFICATION:
        return;
    }
}


void enableGlDebug( bool aEnable )
{
    if( aEnable )
    {
        glEnable( GL_DEBUG_OUTPUT );
        glDebugMessageCallback( (GLDEBUGPROC) debugMsgCallback, nullptr );
    }
    else
    {
        glDisable( GL_DEBUG_OUTPUT );
    }
}
