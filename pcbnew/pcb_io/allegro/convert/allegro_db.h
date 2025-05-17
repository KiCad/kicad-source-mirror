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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#pragma once

#include <cstdint>
#include <memory>
#include <unordered_map>


namespace ALLEGRO
{

class BLOCK_BASE;
class DB_OBJ;

class DB_OBJ_RESOLVER
{
public:
    virtual ~DB_OBJ_RESOLVER() {}
    virtual DB_OBJ* Resolve( uint32_t aKey ) const = 0;
};


/**
 * A DB_OBJ represents one object in an Allegro database.
 */
class DB_OBJ
{
public:
    explicit DB_OBJ( const BLOCK_BASE& aBlock );

    virtual ~DB_OBJ() {}

    /**
     * Called when all objects in the DB are read and can be resolved by their IDs by other objects.
     *
     * Exactly what fields a given object needs to resolve and what happens if the resolution fails is up to that object.
     */
    virtual void ResolveRefs( const DB_OBJ_RESOLVER& aResolver ) = 0;

    uint32_t GetKey() const { return m_Key; }

private:
    // uint8_t m_BlockType;
    uint32_t m_Key;
    size_t   m_Offset;
};


/**
 * An ALLEGRO::DB is the represention of the actual data stored within
 * an Allegro file, without specific reference to the format on disk. Mostly,
 * this DB handles objects in lists.
 */
class DB
{
public:
    DB();

    ~DB();

    void AddObject( std::unique_ptr<DB_OBJ> aObject );

private:
    // Main store of DB objects.
    std::unordered_map<uint32_t, std::unique_ptr<DB_OBJ>> m_Objects;

    std::unique_ptr<DB_OBJ_RESOLVER> m_Resolver;
};

} // namespace ALLEGRO
