/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 */

#ifndef DRC_ZONE_INDEX_H
#define DRC_ZONE_INDEX_H

#include <algorithm>
#include <map>
#include <optional>
#include <utility>
#include <vector>

#include <geometry/rtree/packed_rtree.h>
#include <math/box2.h>
#include <zone.h>

/**
 * Enumerate the pairs of @p aOrderedIndices whose boxes overlap, in the order a nested loop over
 * the vector would have produced them.
 *
 * @p aBox supplies each entry's box and returns no value for an entry which has none; such an
 * entry appears in no pair.  Boxes are normalized before comparison and overlap is inclusive, so
 * the result matches BOX2I::Intersects() including the degenerate origin box a zone with no
 * cached bounds reports.
 */
template <typename BOX_FN>
std::vector<std::pair<size_t, size_t>> CollectOverlappingPairs( const std::vector<size_t>& aOrderedIndices,
                                                                BOX_FN aBox )
{
    using PAIR_TREE = KIRTREE::PACKED_RTREE<size_t, int, 2>;

    std::vector<std::pair<size_t, size_t>> pairs;
    std::vector<BOX2I>                     boxes( aOrderedIndices.size() );
    std::vector<char>                      present( aOrderedIndices.size(), 0 );
    PAIR_TREE::Builder                     builder;

    builder.Reserve( aOrderedIndices.size() );

    for( size_t pos = 0; pos < aOrderedIndices.size(); ++pos )
    {
        std::optional<BOX2I> box = aBox( aOrderedIndices[pos] );

        if( !box )
            continue;

        box->Normalize();
        boxes[pos] = *box;
        present[pos] = 1;

        const int min[2] = { box->GetX(), box->GetY() };
        const int max[2] = { box->GetRight(), box->GetBottom() };
        builder.Add( min, max, pos );
    }

    PAIR_TREE           tree = builder.Build();
    std::vector<size_t> overlaps;
    size_t              pos = 0;

    // Positions, not caller indices, so the emission order matches the vector
    auto visitor =
            [&]( size_t aOther )
            {
                if( aOther > pos )
                    overlaps.push_back( aOther );

                return true;
            };

    for( pos = 0; pos < aOrderedIndices.size(); ++pos )
    {
        if( !present[pos] )
            continue;

        overlaps.clear();

        const int min[2] = { boxes[pos].GetX(), boxes[pos].GetY() };
        const int max[2] = { boxes[pos].GetRight(), boxes[pos].GetBottom() };

        tree.Search( min, max, visitor );

        std::sort( overlaps.begin(), overlaps.end() );

        for( size_t other : overlaps )
            pairs.emplace_back( aOrderedIndices[pos], aOrderedIndices[other] );
    }

    return pairs;
}


class DRC_ZONE_INDEX
{
public:
    void Build( const std::map<PCB_LAYER_ID, std::vector<ZONE*>>& aLayers )
    {
        Clear();

        for( const auto& [layer, zones] : aLayers )
        {
            TREE::Builder builder;
            builder.Reserve( zones.size() );
            m_zones[layer] = zones;

            for( size_t ordinal = 0; ordinal < zones.size(); ++ordinal )
            {
                BOX2I bbox = zones[ordinal]->GetBoundingBox();
                bbox.Normalize();
                int min[2] = { bbox.GetX(), bbox.GetY() };
                int max[2] = { bbox.GetRight(), bbox.GetBottom() };
                builder.Add( min, max, ordinal );
            }

            m_trees.emplace( layer, builder.Build() );
        }
    }

    /// True when the layer holds at least one zone, so a caller can skip building a query box
    bool HasLayer( PCB_LAYER_ID aLayer ) const { return m_trees.find( aLayer ) != m_trees.end(); }

    void Query( PCB_LAYER_ID aLayer, const BOX2I& aBox, std::vector<ZONE*>& aOut ) const
    {
        aOut.clear();
        auto treeIt = m_trees.find( aLayer );
        auto zonesIt = m_zones.find( aLayer );

        if( treeIt == m_trees.end() || zonesIt == m_zones.end() )
            return;

        BOX2I bbox = aBox;
        bbox.Normalize();
        int                 min[2] = { bbox.GetX(), bbox.GetY() };
        int                 max[2] = { bbox.GetRight(), bbox.GetBottom() };
        std::vector<size_t> ordinals;

        auto visitor =
                [&]( size_t aOrdinal )
                {
                    ordinals.push_back( aOrdinal );
                    return true;
                };

        treeIt->second.Search( min, max, visitor );

        std::sort( ordinals.begin(), ordinals.end() );
        aOut.reserve( ordinals.size() );

        for( size_t ordinal : ordinals )
            aOut.push_back( zonesIt->second[ordinal] );
    }

    void Clear()
    {
        m_trees.clear();
        m_zones.clear();
    }

private:
    using TREE = KIRTREE::PACKED_RTREE<size_t, int, 2>;

    std::map<PCB_LAYER_ID, TREE>               m_trees;
    std::map<PCB_LAYER_ID, std::vector<ZONE*>> m_zones;
};

#endif // DRC_ZONE_INDEX_H
