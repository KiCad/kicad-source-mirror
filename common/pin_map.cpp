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
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <pin_map.h>

#include <json_common.h>

#include <algorithm>
#include <utility>


#if defined( _MSC_VER ) && wxCHECK_VERSION( 3, 3, 0 )
extern template class WXDLLIMPEXP_BASE std::vector<wxString>;
#endif


PIN_MAP::PIN_MAP( const wxString& aName ) :
        m_name( aName )
{
}


std::vector<PIN_MAP_ENTRY>::iterator PIN_MAP::findEntry( const wxString& aPinNumber )
{
    return std::find_if( m_entries.begin(), m_entries.end(),
                         [&]( const PIN_MAP_ENTRY& e )
                         {
                             return e.m_PinNumber == aPinNumber;
                         } );
}


std::vector<PIN_MAP_ENTRY>::const_iterator PIN_MAP::findEntry( const wxString& aPinNumber ) const
{
    return std::find_if( m_entries.begin(), m_entries.end(),
                         [&]( const PIN_MAP_ENTRY& e )
                         {
                             return e.m_PinNumber == aPinNumber;
                         } );
}


void PIN_MAP::SetEntry( const wxString& aPinNumber, const wxString& aPadNumber )
{
    auto it = findEntry( aPinNumber );

    if( it == m_entries.end() )
        m_entries.push_back( { aPinNumber, aPadNumber } );
    else
        it->m_PadNumber = aPadNumber;
}


void PIN_MAP::ClearEntry( const wxString& aPinNumber )
{
    auto it = findEntry( aPinNumber );

    if( it != m_entries.end() )
        m_entries.erase( it );
}


bool PIN_MAP::HasEntry( const wxString& aPinNumber ) const
{
    return findEntry( aPinNumber ) != m_entries.end();
}


const wxString& PIN_MAP::GetPadNumber( const wxString& aPinNumber ) const
{
    static const wxString empty;

    auto it = findEntry( aPinNumber );

    return it == m_entries.end() ? empty : it->m_PadNumber;
}


bool PIN_MAP::IsIdentity( const std::vector<wxString>& aPinNumbers ) const
{
    return std::all_of( aPinNumbers.begin(), aPinNumbers.end(),
                        [&]( const wxString& pin )
                        {
                            auto it = findEntry( pin );

                            return it == m_entries.end() || it->m_PadNumber == pin;
                        } );
}


bool PIN_MAP::operator==( const PIN_MAP& aOther ) const
{
    return m_name == aOther.m_name && m_entries == aOther.m_entries;
}


void PIN_MAP_SET::AddOrReplace( PIN_MAP aMap )
{
    if( PIN_MAP* existing = FindByName( aMap.GetName() ) )
        *existing = std::move( aMap );
    else
        m_maps.push_back( std::move( aMap ) );
}


void PIN_MAP_SET::Remove( const wxString& aName )
{
    m_maps.erase( std::remove_if( m_maps.begin(), m_maps.end(),
                                  [&]( const PIN_MAP& m )
                                  {
                                      return m.GetName() == aName;
                                  } ),
                  m_maps.end() );
}


const PIN_MAP* PIN_MAP_SET::FindByName( const wxString& aName ) const
{
    auto it = std::find_if( m_maps.begin(), m_maps.end(),
                            [&]( const PIN_MAP& m )
                            {
                                return m.GetName() == aName;
                            } );

    return it == m_maps.end() ? nullptr : &*it;
}


PIN_MAP* PIN_MAP_SET::FindByName( const wxString& aName )
{
    return const_cast<PIN_MAP*>( std::as_const( *this ).FindByName( aName ) );
}


PIN_MAP MakeLegacyPinMap( const wxString& aName,
                          const std::unordered_map<wxString, std::vector<wxString>>& aAssignments )
{
    PIN_MAP map( aName );

    for( const auto& [pinNumber, pads] : aAssignments )
    {
        if( pads.empty() )
            continue;

        if( pads.size() == 1 )
        {
            map.SetEntry( pinNumber, pads.front() );
        }
        else
        {
            wxString stacked = wxS( "[" );

            for( size_t ii = 0; ii < pads.size(); ++ii )
            {
                if( ii > 0 )
                    stacked += wxS( "," );

                stacked += pads[ii];
            }

            stacked += wxS( "]" );
            map.SetEntry( pinNumber, stacked );
        }
    }

    return map;
}


std::unordered_map<wxString, std::vector<wxString>>
ParseLegacyPinAssignments( const nlohmann::json& aArray )
{
    std::unordered_map<wxString, std::vector<wxString>> assignments;

    if( !aArray.is_array() )
        return assignments;

    for( const auto& entry : aArray )
    {
        if( !entry.contains( "sym" ) || !entry.contains( "fp" ) )
            continue;

        wxString              symbolPin = wxString::FromUTF8( entry["sym"].get<std::string>() );
        std::vector<wxString> pads;
        const auto&           fp = entry["fp"];

        if( fp.is_array() )
        {
            for( const auto& pad : fp )
                pads.push_back( wxString::FromUTF8( pad.get<std::string>() ) );
        }
        else if( fp.is_string() )
        {
            pads.push_back( wxString::FromUTF8( fp.get<std::string>() ) );
        }

        if( !symbolPin.empty() && !pads.empty() )
            assignments[symbolPin] = std::move( pads );
    }

    return assignments;
}


PIN_MAP_SET ParsePinMapSet( const nlohmann::json& aParent )
{
    PIN_MAP_SET set;

    if( !aParent.contains( "pin_maps" ) || !aParent["pin_maps"].is_array() )
        return set;

    for( const auto& mapJson : aParent["pin_maps"] )
    {
        if( !mapJson.contains( "name" ) )
            continue;

        PIN_MAP map( wxString::FromUTF8( mapJson["name"].get<std::string>() ) );

        if( mapJson.contains( "entries" ) && mapJson["entries"].is_array() )
        {
            for( const auto& entryJson : mapJson["entries"] )
            {
                if( entryJson.contains( "pin" ) && entryJson.contains( "pad" ) )
                {
                    map.SetEntry( wxString::FromUTF8( entryJson["pin"].get<std::string>() ),
                                  wxString::FromUTF8( entryJson["pad"].get<std::string>() ) );
                }
            }
        }

        set.AddOrReplace( std::move( map ) );
    }

    return set;
}


std::vector<ASSOCIATED_FOOTPRINT> ParseAssociatedFootprints( const nlohmann::json& aParent )
{
    std::vector<ASSOCIATED_FOOTPRINT> associations;

    if( !aParent.contains( "associated_footprints" ) || !aParent["associated_footprints"].is_array() )
        return associations;

    for( const auto& assocJson : aParent["associated_footprints"] )
    {
        if( !assocJson.contains( "footprint" ) )
            continue;

        ASSOCIATED_FOOTPRINT assoc;
        assoc.m_FootprintLibId.Parse( wxString::FromUTF8( assocJson["footprint"].get<std::string>() ) );

        if( assocJson.contains( "map" ) )
            assoc.m_MapName = wxString::FromUTF8( assocJson["map"].get<std::string>() );

        associations.push_back( std::move( assoc ) );
    }

    return associations;
}
