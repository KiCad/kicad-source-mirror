/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2023 Roberto Fernandez Bautista <roberto.fer.bau@gmail.com>
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * file: cadstar_archive_objects.h
 * Contains common object definitions
 */

#ifndef CADSTAR_ARCHIVE_OBJECTS_H_
#define CADSTAR_ARCHIVE_OBJECTS_H_


enum class CADSTAR_PIN_TYPE
{
    UNCOMMITTED,        ///< Uncommitted pin (default)
    PIN_INPUT,              ///< Input pin
    OUTPUT_OR,          ///< Output pin OR tieable
    OUTPUT_NOT_OR,      ///< Output pin not OR tieable
    OUTPUT_NOT_NORM_OR, ///< Output pin not normally OR tieable
    POWER,              ///< Power pin
    GROUND,             ///< Ground pin
    TRISTATE_BIDIR,     ///< Tristate bi-directional driver pin
    TRISTATE_INPUT,     ///< Tristate input pin
    TRISTATE_DRIVER     ///< Tristate output pin
};


/**
 * @brief Positioning of pin names can be in one of four quadrants
 */
enum class CADSTAR_PIN_POSITION
{
    TOP_RIGHT    = 0, ///< Default
    TOP_LEFT     = 1,
    BOTTOM_LEFT  = 2,
    BOTTOM_RIGHT = 3
};


#endif // CADSTAR_ARCHIVE_OBJECTS_H_
