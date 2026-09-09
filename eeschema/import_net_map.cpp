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

#include <import_net_map.h>
#include <reporter.h>
#include <wx/intl.h>
#include <ostream>
#include <set>


std::ostream& operator<<( std::ostream& aStream, IMPORT_NET_STATUS aStatus )
{
    switch( aStatus )
    {
    case IMPORT_NET_STATUS::RESOLVED:    return aStream << "resolved";
    case IMPORT_NET_STATUS::SPLIT:       return aStream << "split";
    case IMPORT_NET_STATUS::BUS:         return aStream << "bus";
    case IMPORT_NET_STATUS::NO_CONNECT:  return aStream << "no_connect";
    case IMPORT_NET_STATUS::UNCONNECTED: return aStream << "unconnected";
    }

    return aStream << "?";
}


std::map<wxString, wxString> GetBoardNetNameMap( const IMPORT_NET_MAP& aMap, REPORTER& aReporter )
{
    std::map<wxString, std::set<wxString>> candidates;

    for( const IMPORT_NET_MAP_ENTRY& entry : aMap.entries )
    {
        if( entry.status == IMPORT_NET_STATUS::RESOLVED && !entry.generatedName.IsEmpty()
            && !entry.nameAtImport.IsEmpty() )
        {
            candidates[entry.generatedName].insert( entry.nameAtImport );
        }
    }

    std::map<wxString, wxString> result;

    for( const auto& [source, names] : candidates )
    {
        if( names.size() == 1 )
        {
            result.emplace( source, *names.begin() );
        }
        else
        {
            aReporter.Report( wxString::Format( _( "Ambiguous imported net name '%s'; board net rename "
                                                  "omitted." ), source ), RPT_SEVERITY_WARNING );
        }
    }

    // Two source nets that merged into one KiCad net would rename two board nets to the same name.
    // The applier rejects such a batch outright, taking the whole board import down with it.
    std::map<wxString, int> claims;

    for( const auto& [source, target] : result )
        claims[target]++;

    for( auto it = result.begin(); it != result.end(); )
    {
        if( claims[it->second] > 1 )
        {
            aReporter.Report( wxString::Format( _( "Imported net name '%s' is claimed by more than one "
                                                  "source net; board net rename omitted." ), it->second ),
                              RPT_SEVERITY_WARNING );
            it = result.erase( it );
        }
        else
        {
            ++it;
        }
    }

    return result;
}
