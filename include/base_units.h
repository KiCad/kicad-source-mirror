/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2012 CERN
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

/**
 * @author Wayne Stambaugh <stambaughw@verizon.net>
 * @file base_units.h
 * @brief Implementation of conversion functions that require both schematic and board
 *        internal units.
 */

#ifndef _BASE_UNITS_H_
#define _BASE_UNITS_H_

#include <string>

#include <eda_units.h>
#include <convert_to_biu.h>
#include <math/vector2d.h>
#include <geometry/eda_angle.h>

//TODO: Abstract Base Units to a single class

/**
 * Used for holding indeterminate values, such as with multiple selections
 * holding different values or controls which do not wish to set a value.
 */
#define INDETERMINATE_STATE _( "-- mixed values --" )
#define INDETERMINATE_ACTION _( "-- leave unchanged --" )



#endif   // _BASE_UNITS_H_
