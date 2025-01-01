/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2015-2019 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * @author Tomasz Wlostowski <tomasz.wlostowski@cern.ch>
 * @author Alejandro García Montoro <alejandro.garciamontoro@gmail.com>
 *
 * Point in polygon algorithm adapted from Clipper Library (C) Angus Johnson,
 * subject to Clipper library license.
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

#ifndef CORNER_STRATEGY_H_
#define CORNER_STRATEGY_H_


/// define how inflate transform build inflated polygon
enum class CORNER_STRATEGY
{
    ALLOW_ACUTE_CORNERS,   ///< just inflate the polygon. Acute angles create spikes
    CHAMFER_ACUTE_CORNERS, ///< Acute angles are chamfered
    ROUND_ACUTE_CORNERS,   ///< Acute angles are rounded
    CHAMFER_ALL_CORNERS,   ///< All angles are chamfered.
                           ///< The distance between new and old polygon edges is not
                           ///< constant, but do not change a lot
    ROUND_ALL_CORNERS      ///< All angles are rounded.
                           ///< The distance between new and old polygon edges is constant
};


#endif // CORNER_STRATEGY_H_