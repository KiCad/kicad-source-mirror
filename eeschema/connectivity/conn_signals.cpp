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

#include "conn_signals.h"
#include <algorithm>
#include <stdexcept>

namespace SCH_CONNECTIVITY
{
namespace
{
    // Unsuffixed name only, bus-member names are finalized during publication
    NAME_ID SignalBaseName( const SUMMARY& aSummary )
    {
        if( !aSummary.best )
            return INVALID_ID;

        const CLAIM& claim = *aSummary.best;

        if( claim.priority == PRIORITY::PIN && ( !aSummary.pinWitnesses[1] || aSummary.noConnect ) )
            return claim.ncName;

        return claim.fullName;
    }
} // namespace

SIGNAL_RESULT DeriveSignal( const PARTITION& aPartition, const RECORD_STORE::RECORD_CACHE& aRecords,
                            const SLOT_STORE::SLOT_CACHE& aSlots, const SESSION_KEYS& aKeys )
{
    SIGNAL_RESULT result;

    for( const auto& [node, version] : aPartition.identity )
    {
        const NODE_KEY&         key = aKeys.Node( node );
        SUMMARY                 source;
        std::optional<SLOT_KEY> sourceSlot;

        if( std::holds_alternative<NAME_KEY>( key ) && version == 0 )
            continue;

        if( const auto* record = std::get_if<RECORD_NODE>( &key ) )
        {
            const auto* entry = aRecords.Find( record->record );

            if( record->kind != KIND::SIGNAL || !entry || entry->version != version
                || entry->value.kind != KIND::SIGNAL )
                throw std::invalid_argument( "Signal partition does not match current records" );

            source = MakeSummary( record->record, entry->value );

            for( const KIID& item : entry->value.items )
                result.items.push_back( { item, record->record.inst } );
        }
        else if( const auto* slot = std::get_if<SLOT_KEY>( &key ) )
        {
            const auto* entry = aSlots.Find( *slot );

            if( !entry || entry->version != version || entry->value.key != *slot )
                throw std::invalid_argument( "Signal partition does not match current slots" );

            source.best = entry->value.claim;
            source.netclasses = entry->value.parentNetclasses;
            sourceSlot = *slot;
            result.slots.push_back( *slot );
        }
        else
        {
            throw std::invalid_argument( "Signal partition contains an invalid name version" );
        }

        SUMMARY joined = SUMMARY::Join( result.summary, source, aKeys );

        if( joined.best != result.summary.best )
            result.nameSlot = sourceSlot;
        else if( sourceSlot && source.best == joined.best
                 && ( !result.nameSlot || aKeys.Less( *sourceSlot, *result.nameSlot ) ) )
            result.nameSlot = sourceSlot;

        result.summary = std::move( joined );
    }

    result.baseName = SignalBaseName( result.summary );
    const KEY_LESS less{ aKeys };
    std::ranges::sort( result.items, less );
    std::ranges::sort( result.slots, less );
    return result;
}
} // namespace SCH_CONNECTIVITY
