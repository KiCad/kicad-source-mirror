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

#include "footprint_utils.h"

#include <trigo.h>

#include <footprint.h>
#include <item_realignment.h>
#include <pad.h>


bool ComputeFootprintShift( const FOOTPRINT& aExisting, const FOOTPRINT& aNew, VECTOR2I& aShift,
                            EDA_ANGLE& aAngleShift )
{
    // First, we will collect a list of "useful" corresponding points in the two footprints.
    // To be useful, a point from a pad must have a unique pad number, as in the general case, we
    // won't know which of multiple same-named pads in one footprint correspond to which in the
    // other.
    //
    // Theoretically, we could use graphics items and other hints like pad shape matching
    // but these would be hard to unambiguously match in the general case and pad numbers should
    // cover 99% of cases.

    const auto getUniquelyNumberedPads = []( const FOOTPRINT& fp ) -> std::unordered_map<wxString, VECTOR2I>
    {
        std::unordered_map<wxString, VECTOR2I> result;
        std::unordered_set<wxString>           seenDuplicate;

        for( PAD* pad : fp.Pads() )
        {
            const wxString& number = pad->GetNumber();

            // Already seen a pad with this number, so not unique
            // (and it won't be found in result)
            if( seenDuplicate.find( number ) != seenDuplicate.end() )
                continue;

            // Already have a pad with this number, so not unique.
            // Remove the previous entry from the result and mark this number as seen.
            if( result.find( number ) != result.end() )
            {
                result.erase( number );
                seenDuplicate.insert( number );
                continue;
            }

            result[number] = pad->GetFPRelativePosition();
        }

        return result;
    };

    std::unordered_map<wxString, VECTOR2I> existingPads = getUniquelyNumberedPads( aExisting );
    std::unordered_map<wxString, VECTOR2I> newPads = getUniquelyNumberedPads( aNew );

    std::vector<VECTOR2I> existingPoints;
    std::vector<VECTOR2I> newPoints;

    // The matching points are the ones with the same unique pad number in both footprints
    // If a pad is missing in the new footprint, it won't be included.
    for( const auto& [number, pos] : existingPads )
    {
        auto it = newPads.find( number );

        if( it == newPads.end() )
            continue;

        existingPoints.push_back( pos );
        newPoints.push_back( it->second );
    }

    ORTHO_ITEM_REALIGNER aligner;

    std::optional<ITEM_REALIGNER_BASE::TRANSFORM> transform =
            aligner.GetTransform( existingPoints, newPoints );

    if( !transform.has_value() )
    {
        // Failed to compute a transform - perhaps there were no useful pads
        aShift = VECTOR2I( 0, 0 );
        aAngleShift = ANGLE_0;
        return false;
    }

    aShift = GetRotated( transform->m_Translation, aExisting.GetOrientation() );
    aAngleShift = transform->m_Rotation;

    return true;
}
