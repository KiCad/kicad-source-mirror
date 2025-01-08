/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
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

#ifndef HASH_EDA_H_
#define HASH_EDA_H_

/**
 * @file hash_eda.h
 * @brief Hashing functions for EDA_ITEMs.
 */

#include <cstdlib>
#include <functional>

class EDA_ITEM;

/**
 * Enable/disable properties that will be used for calculating the hash.
 *
 * The properties might be combined using the bitwise 'or' operator.
 */
enum HASH_FLAGS
{
    HASH_POS    = 0x01,

    /// Use coordinates relative to the parent object.
    REL_COORD   = 0x02,

    /// Use coordinates relative to the shape position.
    REL_POS     = 0x04,
    HASH_ROT    = 0x08,
    HASH_LAYER  = 0x10,
    HASH_NET    = 0x20,
    HASH_REF    = 0x40,
    HASH_VALUE  = 0x80,
    HASH_ALL    = 0xff
};

/**
 * Calculate hash of an EDA_ITEM.
 *
 * @param aItem is the item for which the hash will be computed.
 * @return Hash value.
 */
std::size_t hash_fp_item( const EDA_ITEM* aItem, int aFlags = HASH_FLAGS::HASH_ALL );

#endif
