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

#include "ref_suffix_split.h"

#include <set>
#include <sstream>

#include <footprint.h>
#include <mail_type.h>


std::optional<SHEET_REF_SUFFIXES> ParseRefSuffixes( const std::string& aPayload )
{
    std::istringstream stream( aPayload );
    std::string        line;

    if( !std::getline( stream, line ) || line != MAIL_SCH_REF_SUFFIXES_TAG )
        return std::nullopt;

    SHEET_REF_SUFFIXES suffixes;
    size_t             rank = 0;

    while( std::getline( stream, line ) )
    {
        size_t tab = line.find( '\t' );

        if( tab == std::string::npos )
            continue;

        SHEET_REF_SUFFIX& entry = suffixes[wxString::FromUTF8( line.substr( 0, tab ) )];
        entry.Rank = rank++;
        entry.Suffix = wxString::FromUTF8( line.substr( tab + 1 ) );
    }

    return suffixes;
}


std::map<KIID, FOOTPRINT_REF_SPLIT> SplitRefSuffixes( const FOOTPRINTS&         aFootprints,
                                                      const SHEET_REF_SUFFIXES* aSuffixes, bool aSuffixMode )
{
    struct GROUP
    {
        size_t Rank = 0;
        KIID   Representative;
        bool   Suffixed = false;
    };

    std::map<KIID, FOOTPRINT_REF_SPLIT>        splits;
    std::map<std::pair<KIID, wxString>, GROUP> groups;
    std::set<KIID>                             grouped;

    auto groupKey =
            []( const FOOTPRINT* aFootprint, const FOOTPRINT_REF_SPLIT& aSplit )
            {
                return std::make_pair( aFootprint->GetPath().back(), aSplit.Base );
            };

    for( const FOOTPRINT* footprint : aFootprints )
    {
        FOOTPRINT_REF_SPLIT&    split = splits[footprint->m_Uuid];
        const wxString          ref = footprint->GetReference();
        KIID_PATH               sheet = footprint->GetPath();
        const SHEET_REF_SUFFIX* entry = nullptr;

        split.Base = ref;
        split.Representative = footprint->m_Uuid;

        if( aSuffixes && !sheet.empty() )
        {
            // A package spanning sheets names its own suffix, which can differ from its sheet's
            if( auto it = aSuffixes->find( sheet.AsString() ); it != aSuffixes->end() )
            {
                entry = &it->second;
            }
            else
            {
                sheet.pop_back();

                if( it = aSuffixes->find( sheet.AsString() + wxS( "/" ) ); it != aSuffixes->end() )
                    entry = &it->second;
            }
        }

        if( !aSuffixes && aSuffixMode && footprint->GetPath().size() > 1 )
        {
            // With no schematic to ask, a numeric suffix cannot be told from part of the number
            split.Unknown = true;
            continue;
        }

        const wxString suffix = entry ? entry->Suffix : wxString();

        if( !suffix.IsEmpty() && ( ref.length() <= suffix.length() || !ref.EndsWith( suffix ) ) )
        {
            split.Unknown = true;
            continue;
        }

        const wxString base = ref.Left( ref.length() - suffix.length() );
        const size_t   firstDigit = base.find_first_of( wxS( "0123456789" ) );

        // Unannotated and empty references are not renumbered, so they need no split
        if( firstDigit == wxString::npos )
            continue;

        // Text after the number that no suffix accounts for would be dropped by renumbering
        if( base.find_first_not_of( wxS( "0123456789" ), firstDigit ) != wxString::npos )
        {
            split.Unknown = true;
            continue;
        }

        if( !entry )
            continue;

        split.Base = base;
        split.Suffix = suffix;

        grouped.insert( footprint->m_Uuid );

        auto [it, added] = groups.try_emplace( groupKey( footprint, split ) );
        GROUP& group = it->second;

        if( added || entry->Rank < group.Rank )
        {
            group.Rank = entry->Rank;
            group.Representative = footprint->m_Uuid;
        }

        group.Suffixed |= !suffix.IsEmpty();
    }

    // Copies share a base only where a suffix tells them apart, since equal bare references are duplicates
    for( const FOOTPRINT* footprint : aFootprints )
    {
        FOOTPRINT_REF_SPLIT& split = splits[footprint->m_Uuid];

        if( !grouped.count( footprint->m_Uuid ) )
            continue;

        if( auto it = groups.find( groupKey( footprint, split ) ); it != groups.end() && it->second.Suffixed )
        {
            split.Representative = it->second.Representative;
            split.Shared = true;
        }
    }

    return splits;
}
