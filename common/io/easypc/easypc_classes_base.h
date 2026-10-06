/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#ifndef EASYPC_CLASSES_BASE_H
#define EASYPC_CLASSES_BASE_H

#include <vector>

#include <io/easypc/easypc_archive.h>


namespace EASYPC
{

/// The newest item format a file can carry (30001)
constexpr int NEWEST_ITEM_FORMAT = 0x7531;


/// A list: owner tag, then the items
struct SOURCE_DESIGN_LIST : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override;

    OBJECT*              Parent = nullptr;
    std::vector<OBJECT*> Items;
};


/// A design data member: owner tag only
struct SOURCE_DESIGN_DATA : OBJECT
{
    LOAD_TASK Load( ARCHIVE& aAr ) override;
    int       ItemFormat( const ARCHIVE& aAr ) const override;

    OBJECT* Parent = nullptr;
};

} // namespace EASYPC

#endif // EASYPC_CLASSES_BASE_H
