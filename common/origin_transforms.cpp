/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2019-2020 Reece R. Pollack <reece@his.com>
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

#include <origin_transforms.h>
#include <geometry/eda_angle.h>


int ORIGIN_TRANSFORMS::ToDisplay( int aValue, COORD_TYPES_T aCoordType ) const
{
    return static_cast<int>( ToDisplay( static_cast<long long int>( aValue ), aCoordType ) );
}


long long int ORIGIN_TRANSFORMS::ToDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue;
}


double ORIGIN_TRANSFORMS::ToDisplay( double aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue;
}


double ORIGIN_TRANSFORMS::ToDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue.AsDegrees();
}


int ORIGIN_TRANSFORMS::FromDisplay( int aValue, COORD_TYPES_T aCoordType ) const
{
    return static_cast<int>( FromDisplay( static_cast<long long int>( aValue ), aCoordType ) );
}


long long int ORIGIN_TRANSFORMS::FromDisplay( long long int aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue;
}


double ORIGIN_TRANSFORMS::FromDisplay( double aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue;
}


EDA_ANGLE ORIGIN_TRANSFORMS::FromDisplay( const EDA_ANGLE& aValue, COORD_TYPES_T aCoordType ) const
{
    return aValue;
}