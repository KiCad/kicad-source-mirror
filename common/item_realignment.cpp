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

#include "item_realignment.h"

#include <trigo.h>

#include <wx/debug.h>


static EDA_ANGLE Snap90Degrees( const EDA_ANGLE& aAngle )
{
    const EDA_ANGLE a = aAngle.Normalized();

    if( a < ANGLE_45 )
        return ANGLE_0;
    if( a < ANGLE_135 )
        return ANGLE_90;
    if( a < EDA_ANGLE{ 225, DEGREES_T } )
        return ANGLE_180;
    if( a < EDA_ANGLE( 315, DEGREES_T ) )
        return ANGLE_270;

    return ANGLE_0;
}


std::optional<ITEM_REALIGNER_BASE::TRANSFORM>
ORTHO_ITEM_REALIGNER::GetTransform( const std::vector<VECTOR2I>& aPtsA,
                                    const std::vector<VECTOR2I>& aPtsB ) const
{
    wxCHECK_MSG( aPtsA.size() == aPtsB.size(), std::nullopt, "Point lists must be the same length" );

    const int epsilon = 10;

    std::optional<EDA_ANGLE> bestRotation;

    // First find a good baseline - two matching points in each set that are not conincident
    for( size_t firstPtIdx = 0; firstPtIdx < aPtsA.size(); firstPtIdx++ )
    {
        for( size_t secondPtIdx = firstPtIdx; secondPtIdx < aPtsA.size(); secondPtIdx++ )
        {
            const VECTOR2I& ptA1 = aPtsA[firstPtIdx];
            const VECTOR2I& ptA2 = aPtsA[secondPtIdx];
            const VECTOR2I  candidateBaselineA = ptA2 - ptA1;

            const VECTOR2I& ptB1 = aPtsB[firstPtIdx];
            const VECTOR2I& ptB2 = aPtsB[secondPtIdx];
            const VECTOR2I  candidateBaselineB = ptB2 - ptB1;

            // If either baseline is too short, it's not a good candidate for alignment
            if( candidateBaselineA.EuclideanNorm() < epsilon )
                continue;

            if( candidateBaselineB.EuclideanNorm() < epsilon )
                continue;

            // Now we have two useful baselines
            bestRotation = EDA_ANGLE( candidateBaselineB ) - EDA_ANGLE( candidateBaselineA );
            break;
        }

        if( bestRotation.has_value() )
            break;
    }

    // Failed to find any suitable baseline
    if( !bestRotation.has_value() )
        return std::nullopt;

    // Snap the baseline rotation to the nearest 90 degrees, if it's close enough
    *bestRotation = Snap90Degrees( *bestRotation );

    // Now we can rotate the new points backwards so that it's rotationally aligned to the
    // existing one.
    std::vector<VECTOR2I> rotatedPtsB = aPtsB;
    for( VECTOR2I& pt : rotatedPtsB )
    {
        RotatePoint( pt, { 0, 0 }, *bestRotation );
    }

    // Now we need to find a transform that minimises errors to the pads

    // The first thing to do if find if there is a transform that aligns more than
    // half of the points. Less that half is no good because that can happen if you widen
    // a DIP footprint or two-row symbol, for example.
    // If we find such a case, we lock to the matching points and ignore the outliers

    // Fist find a pad that has the most matches when we fix it
    std::optional<size_t> bestPadIdx;
    size_t                bestPadMatches = 0;

    for( size_t padIdx = 0; padIdx < aPtsA.size(); padIdx++ )
    {
        const VECTOR2I translation = aPtsA[padIdx] - rotatedPtsB[padIdx];

        size_t matches = 0;
        for( size_t i = 0; i < aPtsA.size(); i++ )
        {
            if( ( aPtsA[i] - ( rotatedPtsB[i] + translation ) ).EuclideanNorm() < epsilon )
                matches++;
        }

        if( matches > bestPadMatches )
        {
            bestPadMatches = matches;
            bestPadIdx = padIdx;
        }
    }

    if( bestPadMatches > aPtsA.size() / 2 )
    {
        // We can fix the best point and be reasonably sure it's the right thing to do, so do that.
        const VECTOR2I translation = aPtsA[*bestPadIdx] - rotatedPtsB[*bestPadIdx];
        return TRANSFORM{ *bestRotation, translation };
    }

    // If we can't find a transform that matches more than half the points,
    // then we can't assume fixing that point is the right thing to do, so
    // so we'll have to find a transform that minimises the total error instead.

    // Compute the mean displacement between original and rotated points.
    // This is the least-squares minimising translation.
    VECTOR2I totalDisplacement = { 0, 0 };
    for( size_t i = 0; i < aPtsA.size(); i++ )
    {
        totalDisplacement += aPtsA[i] - rotatedPtsB[i];
    }

    VECTOR2I minErrorTranslation = totalDisplacement / static_cast<int>( aPtsA.size() );

    return TRANSFORM{ *bestRotation, minErrorTranslation };
}
