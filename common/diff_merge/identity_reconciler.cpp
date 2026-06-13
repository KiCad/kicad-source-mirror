/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 */

#include <diff_merge/identity_reconciler.h>

#include <algorithm>
#include <cmath>


namespace KICAD_DIFF
{

namespace
{

constexpr double POSITION_WEIGHT = 0.40;
constexpr double BBOX_WEIGHT     = 0.20;
constexpr double KEYPROPS_WEIGHT = 0.40;


bool positionsEqual( const VECTOR2I& aA, const VECTOR2I& aB, unsigned int aTolerance )
{
    if( aTolerance == 0 )
        return aA == aB;

    // Widen to 64-bit before subtracting: VECTOR2I components are int32 and
    // coordinates near opposite extremes would overflow a 32-bit difference.
    const int64_t tol = aTolerance;

    return std::abs( static_cast<int64_t>( aA.x ) - aB.x ) <= tol
        && std::abs( static_cast<int64_t>( aA.y ) - aB.y ) <= tol;
}


bool bboxesEqual( const BOX2I& aA, const BOX2I& aB, unsigned int aTolerance )
{
    if( aTolerance == 0 )
        return aA == aB;

    return positionsEqual( aA.GetOrigin(), aB.GetOrigin(), aTolerance )
        && positionsEqual( aA.GetEnd(),    aB.GetEnd(),    aTolerance );
}


/**
 * Symmetric overlap of two key-property lists. Returns matches / max(|A|,|B|),
 * so score(A,B) == score(B,A) regardless of which side has more keys.
 *
 * The denominator is `max` rather than `|A|` (which would produce an
 * asymmetric score and make the reconciler order-sensitive) and not `min`
 * (which would over-credit a list that's a strict subset of the other).
 */
double keyPropOverlap( const std::vector<std::pair<wxString, std::string>>& aA,
                       const std::vector<std::pair<wxString, std::string>>& aB )
{
    // Absent identifying properties supply no positive evidence — fall back to
    // position/bbox alone. Returning 1.0 here would let two generic items at
    // the same position false-match at the maximum possible score.
    if( aA.empty() || aB.empty() )
        return 0.0;

    std::size_t matches = 0;

    for( const auto& [name, value] : aA )
    {
        auto it = std::find_if( aB.begin(), aB.end(),
                                [&]( const std::pair<wxString, std::string>& aPair )
                                {
                                    return aPair.first == name && aPair.second == value;
                                } );

        if( it != aB.end() )
            ++matches;
    }

    std::size_t denom = std::max( aA.size(), aB.size() );
    return static_cast<double>( matches ) / static_cast<double>( denom );
}

} // namespace


double IDENTITY_RECONCILER::ScoreSimilarity( const ITEM_DESCRIPTOR& aA,
                                             const ITEM_DESCRIPTOR& aB ) const
{
    if( aA.type != aB.type )
        return 0.0;

    double score = 0.0;

    if( positionsEqual( aA.position, aB.position, m_config.positionTolerance ) )
        score += POSITION_WEIGHT;

    if( bboxesEqual( aA.bbox, aB.bbox, m_config.positionTolerance ) )
        score += BBOX_WEIGHT;

    score += keyPropOverlap( aA.keyProps, aB.keyProps ) * KEYPROPS_WEIGHT;

    return score;
}


RECONCILIATION IDENTITY_RECONCILER::Reconcile( const std::vector<ITEM_DESCRIPTOR>& aA,
                                               const std::vector<ITEM_DESCRIPTOR>& aB ) const
{
    RECONCILIATION result;

    // Index A and B by KIID_PATH. Track duplicates while building the indices.
    std::map<KIID_PATH, std::size_t> aIndex;
    std::map<KIID_PATH, std::size_t> bIndex;
    std::set<KIID_PATH>              aSeen;
    std::set<KIID_PATH>              bSeen;

    for( std::size_t i = 0; i < aA.size(); ++i )
    {
        const KIID_PATH& id = aA[i].id;

        if( aSeen.count( id ) )
        {
            if( m_config.detectDuplicates
                && std::find( result.duplicatesA.begin(), result.duplicatesA.end(), id )
                       == result.duplicatesA.end() )
            {
                result.duplicatesA.push_back( id );
            }

            continue; // first-seen wins for indexing
        }

        aSeen.insert( id );
        aIndex[id] = i;
    }

    for( std::size_t i = 0; i < aB.size(); ++i )
    {
        const KIID_PATH& id = aB[i].id;

        if( bSeen.count( id ) )
        {
            if( m_config.detectDuplicates
                && std::find( result.duplicatesB.begin(), result.duplicatesB.end(), id )
                       == result.duplicatesB.end() )
            {
                result.duplicatesB.push_back( id );
            }

            continue;
        }

        bSeen.insert( id );
        bIndex[id] = i;
    }

    // Items with duplicate KIID_PATHs are reported separately and excluded
    // from normal matching — once identity is ambiguous, any "match" is
    // arbitrary and would mask the real problem from the user.
    for( const KIID_PATH& dup : result.duplicatesA )
        aIndex.erase( dup );

    for( const KIID_PATH& dup : result.duplicatesB )
        bIndex.erase( dup );

    // Pass 1: KIID_PATH direct matches.
    std::set<std::size_t> matchedB;

    for( const auto& [idA, indexA] : aIndex )
    {
        auto it = bIndex.find( idA );

        if( it == bIndex.end() )
            continue;

        // Same KIID_PATH in both sides — direct match, even if other properties differ.
        result.aToB[idA] = idA;
        result.bToA[idA] = idA;
        matchedB.insert( it->second );
    }

    // Pass 2: similarity fallback for unmatched items.
    if( m_config.enableSimilarity )
    {
        // Collect unmatched A and B items
        std::vector<std::size_t> unmatchedA;

        for( const auto& [idA, indexA] : aIndex )
        {
            if( result.aToB.find( idA ) == result.aToB.end() )
                unmatchedA.push_back( indexA );
        }

        std::vector<std::size_t> unmatchedB;

        for( const auto& [idB, indexB] : bIndex )
        {
            if( matchedB.count( indexB ) == 0 )
                unmatchedB.push_back( indexB );
        }

        // Score every cross pair and greedily take the best ones above threshold.
        // Deterministic order: sort candidate pairs by (score desc, A index asc, B index asc).
        struct Candidate
        {
            double      score;
            std::size_t aIdx;
            std::size_t bIdx;
        };

        // Bucket B by type first so the cross-product only walks type-compatible
        // pairs. ScoreSimilarity returns 0 for mismatched types anyway, but
        // building those zero scores is the dominant cost at scale (5000 PCB
        // items can produce 25M zero-score evaluations).
        std::map<wxString, std::vector<std::size_t>> bByType;

        for( std::size_t bIdx : unmatchedB )
            bByType[aB[bIdx].type].push_back( bIdx );

        // Candidates is grown without pre-reserve: a pessimistic
        // |A|*|B| reserve was 600 MB at the plan budget (5000 items).
        // Most pairs fail the type filter or the threshold; the vector
        // ends up tiny in practice.
        std::vector<Candidate> candidates;

        for( std::size_t aIdx : unmatchedA )
        {
            auto it = bByType.find( aA[aIdx].type );

            if( it == bByType.end() )
                continue;

            for( std::size_t bIdx : it->second )
            {
                double s = ScoreSimilarity( aA[aIdx], aB[bIdx] );

                if( s >= m_config.similarityThreshold )
                    candidates.push_back( { s, aIdx, bIdx } );
            }
        }

        std::sort( candidates.begin(), candidates.end(),
                   []( const Candidate& aL, const Candidate& aR )
                   {
                       if( aL.score != aR.score )
                           return aL.score > aR.score;

                       if( aL.aIdx != aR.aIdx )
                           return aL.aIdx < aR.aIdx;

                       return aL.bIdx < aR.bIdx;
                   } );

        std::set<std::size_t> usedA;
        std::set<std::size_t> usedB;

        for( const Candidate& c : candidates )
        {
            if( usedA.count( c.aIdx ) || usedB.count( c.bIdx ) )
                continue;

            result.aToB[aA[c.aIdx].id] = aB[c.bIdx].id;
            result.bToA[aB[c.bIdx].id] = aA[c.aIdx].id;
            usedA.insert( c.aIdx );
            usedB.insert( c.bIdx );
            matchedB.insert( c.bIdx );
            ++result.similarityMatches;
        }
    }

    // Anything still unmatched goes into aOnly / bOnly.
    for( const auto& [idA, indexA] : aIndex )
    {
        if( result.aToB.find( idA ) == result.aToB.end() )
            result.aOnly.insert( idA );
    }

    for( const auto& [idB, indexB] : bIndex )
    {
        if( result.bToA.find( idB ) == result.bToA.end() )
            result.bOnly.insert( idB );
    }

    return result;
}

} // namespace KICAD_DIFF
