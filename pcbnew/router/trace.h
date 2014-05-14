/*
 * KiRouter - a push-and-(sometimes-)shove PCB router
 *
 * Copyright (C) 2013  CERN
 * Author: Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef __TRACE_H
#define __TRACE_H

#include <string>
#include <iostream>
#include <boost/format.hpp>

static inline void _trace_print( const char* aFuncName, int level, const std::string& aMsg )
{
#ifdef DEBUG
    std::cerr << "trace[" << level << "]: " << aFuncName << ": " << aMsg << std::endl;
#endif
}

#define TRACE( level, fmt, ... ) \
    _trace_print( __FUNCTION__, level, ( boost::format( fmt ) % __VA_ARGS__ ).str() );

#define TRACEn( level, msg ) \
    _trace_print( __FUNCTION__, level, std::string( msg ) );

#endif
