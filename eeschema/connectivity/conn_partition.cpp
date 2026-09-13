/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
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
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "conn_partition.h"
#include "conn_tasks.h"

#include <cassert>
#include <numeric>
#include <stdexcept>

namespace SCH_CONNECTIVITY
{
NODE_ID PARTITIONER::Find( NODE_ID aNode )
{
    while( m_parent[aNode] != aNode )
    {
        m_parent[aNode] = m_parent[m_parent[aNode]];
        aNode = m_parent[aNode];
    }

    return aNode;
}

void PARTITIONER::Unite( NODE_ID aLeft, NODE_ID aRight )
{
    aLeft = Find( aLeft );
    aRight = Find( aRight );

    if( aLeft == aRight )
        return;

    if( m_rank[aLeft] < m_rank[aRight] )
        std::swap( aLeft, aRight );

    m_parent[aRight] = aLeft;

    if( m_rank[aLeft] == m_rank[aRight] )
        ++m_rank[aLeft];
}

std::vector<PARTITION> PARTITIONER::Build( std::span<const NODE_INPUT> aInputs, const SESSION_KEYS& aKeys )
{
    const size_t count = aKeys.NodeCount();
    m_parent.resize( count );
    std::iota( m_parent.begin(), m_parent.end(), NODE_ID( 0 ) );
    m_rank.assign( count, 0 );
    m_active.assign( count, 0 );
    m_versions.assign( count, 0 );
    m_groupOfRoot.assign( count, INVALID_ID );

    for( const NODE_INPUT& input : aInputs )
    {
        if( input.node >= count )
            throw std::invalid_argument( "Connectivity partition node outside session keys" );

        assert( m_versions[input.node] == 0 );
        m_active[input.node] = 1;
        m_versions[input.node] = input.version;

        for( NODE_ID edge : input.edges )
        {
            if( edge >= count )
                throw std::invalid_argument( "Connectivity partition edge outside session keys" );

            m_active[edge] = 1;
            Unite( input.node, edge );
        }
    }

    std::vector<PARTITION> result;

    for( size_t i = 0; i < count; ++i )
    {
        if( !m_active[i] )
            continue;

        const NODE_ID node = static_cast<NODE_ID>( i );
        const NODE_ID root = Find( node );
        NODE_ID&      group = m_groupOfRoot[root];

        if( group == INVALID_ID )
        {
            group = static_cast<NODE_ID>( result.size() );
            result.push_back( { node, {}, 14695981039346656037ULL } );
        }

        PARTITION& partition = result[group];
        partition.identity.emplace_back( node, m_versions[node] );
    }

    ParallelFor( result.size(), [&]( size_t i )
    {
        PARTITION& partition = result[i];

        for( const auto& [node, version] : partition.identity )
        {
            partition.hash = ( ( partition.hash ^ node ) * 1099511628211ULL ^ version ) * 1099511628211ULL;

            if( aKeys.Less( aKeys.Node( node ), aKeys.Node( partition.anchor ) ) )
                partition.anchor = node;
        }
    } );

    return result;
}
} // namespace SCH_CONNECTIVITY
