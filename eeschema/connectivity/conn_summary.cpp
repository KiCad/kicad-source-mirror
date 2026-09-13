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

#include "conn_summary.h"

#include <algorithm>
#include <iterator>

namespace SCH_CONNECTIVITY
{
SUMMARY MakeSummary( const RECORD_KEY& aKey, const ISLAND_RECORD& aRecord )
{
    SUMMARY result;
    result.netclasses = aRecord.netclasses;

    if( !aRecord.claims.empty() )
        result.best = aRecord.claims.front();

    const auto key = [&]( const std::optional<KIID>& aItem ) -> std::optional<ITEM_KEY>
    {
        return aItem ? std::optional( ITEM_KEY{ *aItem, aKey.inst } ) : std::nullopt;
    };

    std::ranges::transform( aRecord.atoms.pinWitnesses, result.pinWitnesses.begin(), key );
    result.noConnect = key( aRecord.atoms.noConnect );
    return result;
}

SUMMARY SUMMARY::Join( const SUMMARY& aLeft, const SUMMARY& aRight, const SESSION_KEYS& aKeys )
{
    SUMMARY result;
    result.best = !aLeft.best || ( aRight.best && CLAIM_LESS{ aKeys }( *aLeft.best, *aRight.best ) ) ? aRight.best
                                                                                                    : aLeft.best;

    std::array<ITEM_KEY, 4> witnesses;
    auto                    end = witnesses.begin();

    for( const SUMMARY* summary : { &aLeft, &aRight } )
    {
        for( const auto& witness : summary->pinWitnesses )
        {
            if( witness )
                *end++ = *witness;
        }
    }

    std::sort( witnesses.begin(), end, KEY_LESS{ aKeys } );
    end = std::unique( witnesses.begin(), end );
    std::copy( witnesses.begin(), std::min( end, witnesses.begin() + result.pinWitnesses.size() ),
               result.pinWitnesses.begin() );

    result.noConnect = !aLeft.noConnect || ( aRight.noConnect && !aKeys.Less( *aLeft.noConnect, *aRight.noConnect ) )
                               ? aRight.noConnect
                               : aLeft.noConnect;
    result.netclasses.reserve( aLeft.netclasses.size() + aRight.netclasses.size() );
    std::set_union( aLeft.netclasses.begin(), aLeft.netclasses.end(), aRight.netclasses.begin(),
                    aRight.netclasses.end(), std::back_inserter( result.netclasses ), NAME_LESS{ &aKeys } );
    return result;
}
} // namespace SCH_CONNECTIVITY
