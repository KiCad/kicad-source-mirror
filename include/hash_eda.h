/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
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

/**
 * @brief Hashing functions for EDA_ITEMs.
 */

#include <cstdlib>

class EDA_ITEM;

///> Enables/disables properties that will be used for calculating the hash.
///> The properties might be combined using the bitwise 'or' operator.
enum HASH_FLAGS
{
    POSITION    = 0x01,

    ///> use coordinates relative to the parent object
    REL_COORD   = 0x02,
    ROTATION    = 0x04,
    LAYER       = 0x08,
    NET         = 0x10,
    REFERENCE   = 0x20,
    VALUE       = 0x40,
    ALL         = 0xff
};

/**
 * Calculates hash of an EDA_ITEM.
 * @param aItem is the item for which the hash will be computed.
 * @return Hash value.
 */
std::size_t hash_eda( const EDA_ITEM* aItem, int aFlags = HASH_FLAGS::ALL );
