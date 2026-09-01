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

#ifndef KICAD_CORE_UNION_FIND_H
#define KICAD_CORE_UNION_FIND_H

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <stdexcept>
#include <utility>
#include <vector>

/**
 * Lock-free disjoint-set over a dense range of indices.
 *
 * Each tree is rooted at the smallest index it contains.  Parent indices decrease along the
 * path to a root.  Unite() climbs both paths and advances the side with the larger parent, so
 * the set needs no rank array and no lock.
 *
 * Call Unite() from any number of threads.  Find() and Connected() are safe against each other
 * but not against Unite(), so complete all unions for a phase before you query.
 *
 * Dhulipala, L., Hong, C., & Shun, J. (2021). ConnectIt: A framework for static and
 * incremental parallel graph connectivity algorithms. Proceedings of the VLDB Endowment,
 * 14(4), 653-667. https://doi.org/10.14778/3436905.3436923
 *
 * Dijkstra, E. W. (1976). A Discipline of Programming. Prentice-Hall, Ch. 23.
 */
class KI_UNION_FIND
{
public:
    explicit KI_UNION_FIND( size_t aCount ) { Reset( aCount ); }

    KI_UNION_FIND( const KI_UNION_FIND& ) = delete;
    KI_UNION_FIND& operator=( const KI_UNION_FIND& ) = delete;

    ///< Discard all unions and resize the set to \a aCount single-element components.
    void Reset( size_t aCount )
    {
        // Parents are uint32_t to keep the array dense, so a larger set would alias index
        // 2^32 onto index zero
        if( aCount > static_cast<size_t>( std::numeric_limits<uint32_t>::max() ) )
            throw std::length_error( "KI_UNION_FIND exceeds its 2^32 element limit" );

        std::vector<std::atomic<uint32_t>> fresh( aCount );
        m_parent.swap( fresh );

        for( size_t ii = 0; ii < aCount; ++ii )
            m_parent[ii].store( static_cast<uint32_t>( ii ), std::memory_order_relaxed );

        m_components.store( aCount, std::memory_order_relaxed );
    }

    size_t Size() const { return m_parent.size(); }

    ///< @return the number of components that remain.
    size_t ComponentCount() const { return m_components.load( std::memory_order_relaxed ); }

    /**
     * Merge the components that hold \a aA and \a aB.
     *
     * @return true if this call made the link.  Exactly one caller sees true for each merge.
     */
    bool Unite( size_t aA, size_t aB )
    {
        uint32_t u = static_cast<uint32_t>( aA );
        uint32_t v = static_cast<uint32_t>( aB );
        uint32_t pu = m_parent[u].load( std::memory_order_relaxed );
        uint32_t pv = m_parent[v].load( std::memory_order_relaxed );

        while( pu != pv )
        {
            // Work on the side with the larger parent so that a root always links downwards
            if( pu < pv )
            {
                std::swap( u, v );
                std::swap( pu, pv );
            }

            if( u == pu )
            {
                // u is a root and pv is below it, so this link is legal
                if( m_parent[u].compare_exchange_strong( pu, pv, std::memory_order_acq_rel,
                                                         std::memory_order_relaxed ) )
                {
                    m_components.fetch_sub( 1, std::memory_order_relaxed );
                    return true;
                }

                // The exchange reloaded pu with the value that won, so try again
            }
            else
            {
                // Lift u one level and climb.  A lost exchange costs only the shortcut, so
                // the result is ignored
                uint32_t grandparent = m_parent[pu].load( std::memory_order_relaxed );
                uint32_t expected = pu;

                m_parent[u].compare_exchange_strong( expected, grandparent,
                                                     std::memory_order_acq_rel,
                                                     std::memory_order_relaxed );

                u = pu;
                pu = m_parent[u].load( std::memory_order_relaxed );
            }
        }

        return false;
    }

    ///< @return the representative of the component that holds \a aX.
    size_t Find( size_t aX ) const
    {
        uint32_t x = static_cast<uint32_t>( aX );
        uint32_t p = m_parent[x].load( std::memory_order_relaxed );

        while( p != x )
        {
            x = p;
            p = m_parent[x].load( std::memory_order_relaxed );
        }

        return x;
    }

    /**
     * Shorten the path from \a aX to its root so that later queries walk less of it.  Do not
     * call this while another thread is inside Unite().
     *
     * @return the representative of the component that holds \a aX.
     */
    size_t FindCompress( size_t aX )
    {
        uint32_t x = static_cast<uint32_t>( aX );
        uint32_t p = m_parent[x].load( std::memory_order_relaxed );

        while( p != x )
        {
            uint32_t grandparent = m_parent[p].load( std::memory_order_relaxed );

            m_parent[x].store( grandparent, std::memory_order_relaxed );
            x = p;
            p = grandparent;
        }

        return x;
    }

    bool Connected( size_t aA, size_t aB ) const { return Find( aA ) == Find( aB ); }

private:
    std::vector<std::atomic<uint32_t>> m_parent;
    std::atomic<size_t>                m_components{ 0 };
};

#endif // KICAD_CORE_UNION_FIND_H
