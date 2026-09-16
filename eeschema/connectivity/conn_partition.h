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

#pragma once

#include "conn_keys.h"
#include <span>

namespace SCH_CONNECTIVITY
{
struct NODE_INPUT
{
    NODE_ID              node = INVALID_ID;
    uint64_t             version = 0;
    std::vector<NODE_ID> edges;
};

/**
 * Exact identity of one connected component. Edges are not part of it, because they change only
 * membership, which the node list records. Evaluators must therefore read nodes and never edges.
 */
struct PARTITION
{
    NODE_ID                                   anchor = INVALID_ID;
    std::vector<std::pair<NODE_ID, uint64_t>> identity;
    uint64_t                                  hash = 0;
    bool                                      operator==( const PARTITION& ) const = default;
};

/**
 * Fresh connectivity for one stratum. Scratch arrays retain capacity, never old unions.
 *
 * @see @ref sch_conn_modify for why every update partitions all nodes.
 */
class PARTITIONER
{
public:
    std::vector<PARTITION> Build( std::span<const NODE_INPUT> aInputs, const SESSION_KEYS& aKeys );

private:
    NODE_ID Find( NODE_ID aNode );
    void    Unite( NODE_ID aLeft, NODE_ID aRight );

    std::vector<NODE_ID>  m_parent;
    std::vector<uint8_t>  m_rank;
    std::vector<uint8_t>  m_active;
    std::vector<uint64_t> m_versions;
    std::vector<NODE_ID>  m_groupOfRoot;
};
} // namespace SCH_CONNECTIVITY
