/*
 * This program source code file is part of KICAD, a free EDA CAD application.
 *
 * Copyright (C) 2010 SoftPLC Corporation, <dick@softplc.com>
 * Copyright (C) 2010 Kicad Developers, see change_log.txt for contributors.
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

#ifndef KICAD_EXCEPTIONS_H_
#define KICAD_EXCEPTIONS_H_

/*  Just exceptions
*/


#include <wx/string.h>
#include <string>

/**
 * Struct IO_ERROR
 * is a class used to hold an error message and may be used to throw exceptions
 * containing meaningful error messages.
 * @author Dick Hollenbeck
 */
struct IO_ERROR
{
    wxString    errorText;

    IO_ERROR( const wxString& aMsg ) :
        errorText( aMsg )
    {
    }

    IO_ERROR( const std::string& aMsg ) :
        errorText( wxConvertMB2WX( aMsg.c_str() ) )
    {
    }
};


/**
 * Class PARSE_ERROR
 * contains a filename or source description, a line number, a character offset,
 * and an error message.
 * @author Dick Hollenbeck
 */
struct PARSE_ERROR : public IO_ERROR
{
    wxString    source;         ///< filename typically, or other source
    int         lineNumber;
    int         byteIndex;      ///< char offset, starting from 1, into the problem line.

    PARSE_ERROR( const wxString& aMsg, const wxString& aSource,
                    int aLineNumber, int aByteIndex ) :
        IO_ERROR( aMsg ),
        source( aSource ),
        lineNumber( aLineNumber )
    {
    }
};

#endif  // KICAD_EXCEPTIONS_H_
