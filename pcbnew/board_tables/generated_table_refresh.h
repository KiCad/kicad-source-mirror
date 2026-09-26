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

#ifndef GENERATED_TABLE_REFRESH_H
#define GENERATED_TABLE_REFRESH_H

#include <memory>
#include <type_traits>
#include <typeindex>
#include <typeinfo>
#include <utility>
#include <vector>

class BOARD;


/**
 * Board state a table rebuild wants to change, held back until the whole refresh has succeeded.
 */
class GENERATED_TABLE_PENDING
{
public:
    virtual ~GENERATED_TABLE_PENDING() = default;

    virtual void Commit( BOARD& aBoard ) = 0;
};


/**
 * One pass of rebuilding tables. Board state they accumulate reaches the board only on Commit.
 *
 * Each kind of pending state exists once per pass, so several tables sharing it build on each
 * other's changes rather than each keeping only its own.
 */
class GENERATED_TABLE_REFRESH
{
public:
    explicit GENERATED_TABLE_REFRESH( BOARD& aBoard );

    GENERATED_TABLE_REFRESH( const GENERATED_TABLE_REFRESH& ) = delete;
    GENERATED_TABLE_REFRESH& operator=( const GENERATED_TABLE_REFRESH& ) = delete;

    /**
     * The pass's T, created from the board on first use.
     */
    template <typename T>
    T& Pending()
    {
        static_assert( std::is_base_of_v<GENERATED_TABLE_PENDING, T>,
                       "Pending state must derive from GENERATED_TABLE_PENDING" );

        const std::type_index type( typeid( T ) );

        for( auto& [pendingType, pending] : m_pending )
        {
            if( pendingType == type )
                return static_cast<T&>( *pending );
        }

        m_pending.emplace_back( type, std::make_unique<T>( m_board ) );

        return static_cast<T&>( *m_pending.back().second );
    }

    /**
     * Write every pending state back to the board, in the order it was first asked for.
     */
    void Commit();

private:
    BOARD& m_board;

    std::vector<std::pair<std::type_index, std::unique_ptr<GENERATED_TABLE_PENDING>>> m_pending;
};


/**
 * Bring every generated table on the board up to date and return how many were rebuilt.
 *
 * A generated table is derived data. Anything that reads one calls this first, so a table can
 * never show a board that has moved on.
 */
int RefreshGeneratedTables( BOARD& aBoard );

#endif // GENERATED_TABLE_REFRESH_H
