/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 CERN
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

#include <confirm.h>

#include <GL/glew.h>
#include <cassert>

int checkGlError( const std::string& aInfo )
{
    int result = glGetError();

    switch( result )
    {
        case GL_NO_ERROR:
            // all good
            break;

        case GL_INVALID_ENUM:
            DisplayError( NULL, wxString::Format( "Error: %s: invalid enum", aInfo ) );
            break;

        case GL_INVALID_VALUE:
            DisplayError( NULL, wxString::Format( "Error: %s: invalid value", aInfo ) );
            break;

        case GL_INVALID_OPERATION:
            DisplayError( NULL, wxString::Format( "Error: %s: invalid operation", aInfo ) );
            break;

        case GL_INVALID_FRAMEBUFFER_OPERATION:
            DisplayError( NULL, wxString::Format( "Error: %s: invalid framebuffer operation", aInfo ) );
            break;

        case GL_OUT_OF_MEMORY:
            DisplayError( NULL, wxString::Format( "Error: %s: out of memory", aInfo ) );
            break;

        case GL_STACK_UNDERFLOW:
            DisplayError( NULL, wxString::Format( "Error: %s: stack underflow", aInfo ) );
            break;

        case GL_STACK_OVERFLOW:
            DisplayError( NULL, wxString::Format( "Error: %s: stack overflow", aInfo ) );
            break;

        default:
            DisplayError( NULL, wxString::Format( "Error: %s: unknown error", aInfo ) );
            break;
    }

    //assert( result == GL_NO_ERROR );

    return result;
}

