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

#include "conn_engine.h"
#include "conn_tasks.h"
#include <wx/thread.h>
#include <algorithm>

namespace SCH_CONNECTIVITY
{
ENGINE::ENGINE() :
        m_inputs( m_versions, m_keys ),
        m_records( m_versions, m_keys ),
        m_bundles( m_versions ),
        m_slots( m_versions, m_keys ),
        m_signals( m_versions ),
        m_published( m_keys )
{
}

void ENGINE::Update( const SCH_SHEET_LIST& aPaths, uint64_t aTextEpoch, const BUS_ALIASES& aAliases, bool aRebuild )
{
    wxASSERT( wxThread::IsMain() );

    try
    {
        if( aRebuild )
            clearStages( true );

        const auto frame = m_inputs.Capture( aPaths, aTextEpoch );
        m_records.Update( frame, m_inputs, aAliases );
        const auto& records = m_records.Records();
        const auto  bundles = m_partitioner.Build( RecordNodes( records, KIND::BUNDLE, m_keys ), m_keys );
        m_bundles.UpdateBatch( bundles, [&]( const auto& missing )
        {
            std::vector<BUNDLE_INPUT> inputs;
            inputs.reserve( missing.size() );

            for( const PARTITION* partition : missing )
                inputs.push_back( PrepareBundle( *partition, records, m_keys ) );

            std::vector<BUNDLE_BINDING> values( missing.size() );
            ParallelFor( values.size(), [&]( size_t i ) { values[i] = BindBundle( inputs[i], m_keys ); } );
            return values;
        } );
        m_slots.Update( m_bundles );
        const auto signals = m_partitioner.Build( SignalNodes( records, m_slots.Slots(), m_keys ), m_keys );
        m_signals.UpdateBatch( signals, [&]( const auto& missing )
        {
            std::vector<SIGNAL_RESULT> values( missing.size() );
            ParallelFor( values.size(), [&]( size_t i )
            {
                values[i] = DeriveSignal( *missing[i], records, m_slots.Slots(), m_keys );
            } );
            return values;
        } );
        m_published.Update( m_bundles, m_signals, records, frame, m_inputs );
    }
    catch( ... )
    {
        Clear();
        throw;
    }
}

std::vector<CLAIM> ENGINE::DriverCandidates( NODE_ID aComponent ) const
{
    wxASSERT( wxThread::IsMain() );
    const auto component = m_published.Components().find( aComponent );

    if( component == m_published.Components().end() )
        return {};

    const auto& content = *component->second.content;
    std::vector<CLAIM> result;

    for( const RECORD_KEY& record : content.records )
    {
        const auto& claims = m_records.Records().Entries().at( record )->value.claims;

        for( const CLAIM& claim : claims )
        {
            if( content.kind == KIND::SIGNAL || claim.schema )
                result.push_back( claim );
        }
    }

    if( content.kind == KIND::SIGNAL )
    {
        for( const SLOT_KEY& slot : content.slots )
            result.push_back( m_slots.Slots().Entries().at( slot )->value.claim );
    }

    const CLAIM_LESS less{ m_keys };
    std::stable_sort( result.begin(), result.end(), [&]( const CLAIM& a, const CLAIM& b ) { return less( b, a ); } );
    return result;
}

void ENGINE::Clear()
{
    wxASSERT( wxThread::IsMain() );
    clearStages();
    m_published.Clear();
}

void ENGINE::clearStages( bool aRetainSourceValues )
{
    m_signals.Clear();
    m_slots.Clear();
    m_bundles.Clear();
    m_records.Clear();

    if( aRetainSourceValues )
        m_inputs.Invalidate();
    else
        m_inputs.Clear();
}
} // namespace SCH_CONNECTIVITY
