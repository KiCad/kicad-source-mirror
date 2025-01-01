/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2016 SoftPLC Corporation, Dick Hollenbeck <dick@softplc.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

%warnfilter(511) IO_ERROR;

%ignore PARSE_ERROR;
%ignore FUTURE_FORMAT_ERROR;
%include ki_exception.h

%{
#include <ki_exception.h>
%}

%include exception.i        // from SWIG

// Target a specific function with "C++ to python exception handling and
// translation code".  Invoke this macro separately for each C++ function
// that throws.  This is a less bulkier scheme than assuming that ALL C++
// functions throw, because they do not all throw.
%define HANDLE_EXCEPTIONS(function)
%exception function {
    try
    {
        $action
    }
    catch( const IO_ERROR& e )
    {
        UTF8 str = e.Problem();

        SWIG_exception( SWIG_IOError, str.c_str() );
        return NULL;
    }
    catch( const std::exception& e )
    {
        std::string str = e.what();

        SWIG_exception( SWIG_IOError, str.c_str() );
        return NULL;
    }
    catch( ... )
    {
        SWIG_fail;
    }
}
%enddef

