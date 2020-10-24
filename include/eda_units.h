/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 1992-2020 KiCad Developers, see AUTHORS.txt for contributors.
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


#ifndef EDA_UNITS_H
#define EDA_UNITS_H

/**
 * The type of unit.
 */
enum class EDA_DATA_TYPE
{
    DISTANCE = 0,
    AREA     = 1,
    VOLUME   = 2
};

enum class EDA_UNITS
{
    INCHES      = 0,
    MILLIMETRES = 1,
    UNSCALED    = 2,
    DEGREES     = 3,
    PERCENT     = 4,
    MILS        = 5,
};

namespace EDA_UNIT_UTILS
{
    bool IsImperialUnit( EDA_UNITS aUnit );

    bool IsMetricUnit( EDA_UNITS aUnit );
}

#endif