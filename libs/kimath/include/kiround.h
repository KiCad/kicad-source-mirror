/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2019 KiCad Developers, see AUTHORS.txt for contributors.
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
#ifndef KIROUND_H_
#define KIROUND_H_

#include <limits>
#include <typeinfo>
#include <wx/log.h>

/**
 * Round a floating point number to an integer using "round halfway cases away from zero".
 *
 * In Debug build an assert fires if will not fit into the return type.
 */
template <typename fp_type, typename ret_type = int>
constexpr ret_type KiROUND( fp_type v )
{
    using max_ret = long long int;
    fp_type ret = v < 0 ? v - 0.5 : v + 0.5;

    if( std::numeric_limits<ret_type>::max() < ret ||
        std::numeric_limits<ret_type>::lowest() > ret )
    {
        wxLogDebug
        ( "Overflow KiROUND converting value %f to %s", double( v ), typeid(ret_type).name() );
        return 0;
    }

    return ret_type( max_ret( ret ) );
}

#endif
