/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Jon Evans <jon@craftyjon.com>
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

#include <nlohmann/json.hpp>


#include <project/net_settings.h>
#include <settings/parameters.h>
#include <settings/json_settings_internals.h>
#include <settings/settings_manager.h>
#include <string_utils.h>
#include <convert_to_biu.h>


// const int netSettingsSchemaVersion = 0;
const int netSettingsSchemaVersion = 1;     // new overbar syntax


static OPT<int> getInPcbUnits( const nlohmann::json& aObj, const std::string& aKey,
                               OPT<int> aDefault = OPT<int>() )
{
    if( aObj.contains( aKey ) && aObj[aKey].is_number() )
        return PcbMillimeter2iu( aObj[aKey].get<double>() );
    else
        return aDefault;
};


NET_SETTINGS::NET_SETTINGS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "net_settings", netSettingsSchemaVersion, aParent, aPath ),
        m_NetClasses()
{
    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "classes",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                NETCLASSPTR                netclass = m_NetClasses.GetDefault();
                NETCLASSES::const_iterator nc       = m_NetClasses.begin();

                for( unsigned int idx = 0; idx <= m_NetClasses.GetCount(); idx++ )
                {
                    if( idx > 0 )
                    {
                        netclass = nc->second;
                        ++nc;
                    }

                    // Note: we're in common/, but we do happen to know which of these fields
                    // are used in which units system.
                    nlohmann::json netclassJson = {
                        { "name",              netclass->GetName().ToUTF8() },
                        { "wire_width",        SchIu2Mils( netclass->GetWireWidth() ) },
                        { "bus_width",         SchIu2Mils( netclass->GetBusWidth() ) },
                        { "line_style",        netclass->GetLineStyle() },
                        { "schematic_color",   netclass->GetSchematicColor() },
                        { "pcb_color",         netclass->GetPcbColor() }
                        };


                    if( netclass->HasClearance() )
                        netclassJson.push_back( { "clearance",
                                PcbIu2Millimeter( netclass->GetClearance() ) } );

                    if( netclass->HasTrackWidth() )
                        netclassJson.push_back( { "track_width",
                                PcbIu2Millimeter( netclass->GetTrackWidth() ) } );

                    if( netclass->HasViaDiameter() )
                        netclassJson.push_back( { "via_diameter",
                                PcbIu2Millimeter( netclass->GetViaDiameter() ) } );

                    if( netclass->HasViaDrill() )
                        netclassJson.push_back( { "via_drill",
                                PcbIu2Millimeter( netclass->GetViaDrill() ) } );

                    if( netclass->HasuViaDiameter() )
                        netclassJson.push_back( { "microvia_diameter",
                                PcbIu2Millimeter( netclass->GetuViaDiameter() ) } );

                    if( netclass->HasuViaDrill() )
                        netclassJson.push_back( { "microvia_drill",
                                PcbIu2Millimeter( netclass->GetuViaDrill() ) } );

                    if( netclass->HasDiffPairWidth() )
                        netclassJson.push_back( { "diff_pair_width",
                                PcbIu2Millimeter( netclass->GetDiffPairWidth() ) } );

                    if( netclass->HasDiffPairGap() )
                        netclassJson.push_back( { "diff_pair_gap",
                                PcbIu2Millimeter( netclass->GetDiffPairGap() ) } );

                    if( netclass->HasDiffPairViaGap() )
                        netclassJson.push_back( { "diff_pair_via_gap",
                                PcbIu2Millimeter( netclass->GetDiffPairViaGap() ) } );

                    if( idx > 0 )   // No need to store members of Default netclass
                    {
                        nlohmann::json membersJson = nlohmann::json::array();

                        for( const wxString& member : *netclass )
                        {
                            if( !member.empty() )
                                membersJson.push_back( member );
                        }

                        netclassJson["nets"] = membersJson;
                    }

                    ret.push_back( netclassJson );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_NetClasses.Clear();
                m_NetClassAssignments.clear();
                NETCLASSPTR netclass;
                NETCLASSPTR defaultClass = m_NetClasses.GetDefault();

                auto getInSchematicUnits =
                        []( const nlohmann::json& aObj, const std::string& aKey, int aDefault )
                        {
                            if( aObj.contains( aKey ) && aObj[aKey].is_number() )
                                return SchMils2iu( aObj[aKey].get<double>() );
                            else
                                return aDefault;
                        };

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() || !entry.contains( "name" ) )
                        continue;

                    wxString name = entry["name"];

                    if( name == defaultClass->GetName() )
                        netclass = defaultClass;
                    else
                        netclass = std::make_shared<NETCLASS>( name );

                    if( auto value = getInPcbUnits( entry, "clearance" ) )
                        netclass->SetClearance( *value );

                    if( auto value = getInPcbUnits( entry, "track_width" ) )
                        netclass->SetTrackWidth( *value );

                    if( auto value = getInPcbUnits( entry, "via_diameter" ) )
                        netclass->SetViaDiameter( *value );

                    if( auto value = getInPcbUnits( entry, "via_drill" ) )
                        netclass->SetViaDrill( *value );

                    if( auto value = getInPcbUnits( entry, "microvia_diameter" ) )
                        netclass->SetuViaDiameter( *value );

                    if( auto value = getInPcbUnits( entry, "microvia_drill" ) )
                        netclass->SetuViaDrill( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_width" ) )
                        netclass->SetDiffPairWidth( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_gap" ) )
                        netclass->SetDiffPairGap( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_via_gap" ) )
                        netclass->SetDiffPairViaGap( *value );

                    netclass->SetWireWidth( getInSchematicUnits( entry, "wire_width",
                                                                 netclass->GetWireWidth() ) );
                    netclass->SetBusWidth( getInSchematicUnits( entry, "bus_width",
                                                                netclass->GetWireWidth() ) );

                    if( entry.contains( "line_style" ) && entry["line_style"].is_number() )
                        netclass->SetLineStyle( entry["line_style"].get<int>() );

                    if( entry.contains( "nets" ) && entry["nets"].is_array() )
                    {
                        for( const auto& net : entry["nets"].items() )
                            netclass->Add( net.value().get<wxString>() );
                    }

                    if( entry.contains( "pcb_color" ) && entry["pcb_color"].is_string() )
                        netclass->SetPcbColor( entry["pcb_color"].get<KIGFX::COLOR4D>() );

                    if( entry.contains( "schematic_color" )
                      && entry["schematic_color"].is_string() )
                        netclass->SetSchematicColor(
                                entry["schematic_color"].get<KIGFX::COLOR4D>() );

                    if( netclass != defaultClass )
                        m_NetClasses.Add( netclass );

                    for( const wxString& net : *netclass )
                        m_NetClassAssignments[ net ] = netclass->GetName();
                }

                ResolveNetClassAssignments();
            },
            {} ) );

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>( "net_colors",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = {};

                for( const auto& pair : m_PcbNetColors )
                {
                    std::string key( pair.first.ToUTF8() );
                    ret[key] = pair.second;
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_object() )
                    return;

                m_PcbNetColors.clear();

                for( const auto& pair : aJson.items() )
                {
                    wxString key( pair.key().c_str(), wxConvUTF8 );
                    m_PcbNetColors[key] = pair.value().get<KIGFX::COLOR4D>();
                }
            },
            {} ) );

    registerMigration( 0, 1, std::bind( &NET_SETTINGS::migrateSchema0to1, this ) );
}


NET_SETTINGS::~NET_SETTINGS()
{
    // Release early before destroying members
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


bool NET_SETTINGS::migrateSchema0to1()
{
    if( m_internals->contains( "classes" ) && m_internals->At( "classes" ).is_array() )
    {
        for( auto& netClass : m_internals->At( "classes" ).items() )
        {
            if( netClass.value().contains( "nets" ) && netClass.value()["nets"].is_array() )
            {
                nlohmann::json migrated = nlohmann::json::array();

                for( auto& net : netClass.value()["nets"].items() )
                    migrated.push_back( ConvertToNewOverbarNotation( net.value().get<wxString>() ) );

                netClass.value()["nets"] = migrated;
            }
        }
    }

    return true;
}


const wxString& NET_SETTINGS::GetNetclassName( const wxString& aNetName ) const
{
    static wxString defaultNetname = NETCLASS::Default;

    auto it = m_NetClassAssignments.find( aNetName );

    if( it == m_NetClassAssignments.end() )
        return defaultNetname;
    else
        return it->second;
}


static bool isSuperSubOverbar( wxChar c )
{
    return c == '_' || c == '^' || c == '~';
}


bool NET_SETTINGS::ParseBusVector( const wxString& aBus, wxString* aName,
                                   std::vector<wxString>* aMemberList )
{
    auto isDigit = []( wxChar c )
                   {
                       static   wxString digits( wxT( "0123456789" ) );
                       return digits.Contains( c );
                   };

    size_t   busLen = aBus.length();
    size_t   i = 0;
    wxString prefix;
    wxString suffix;
    wxString tmp;
    long     begin = 0;
    long     end = 0;
    int      braceNesting = 0;

    prefix.reserve( busLen );

    // Parse prefix
    //
    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '{' )
        {
            if( i > 0 && isSuperSubOverbar( aBus[i-1] ) )
                braceNesting++;
            else
                return false;
        }
        else if( aBus[i] == '}' )
        {
            braceNesting--;
        }

        if( aBus[i] == ' ' || aBus[i] == ']' )
            return false;

        if( aBus[i] == '[' )
            break;

        prefix += aBus[i];
    }

    // Parse start number
    //
    i++;  // '[' character

    if( i >= busLen )
        return false;

    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '.' && i + 1 < busLen && aBus[i+1] == '.' )
        {
            tmp.ToLong( &begin );
            i += 2;
            break;
        }

        if( !isDigit( aBus[i] ) )
            return false;

        tmp += aBus[i];
    }

    // Parse end number
    //
    tmp = wxEmptyString;

    if( i >= busLen )
        return false;

    for( ; i < busLen; ++i )
    {
        if( aBus[i] == ']' )
        {
            tmp.ToLong( &end );
            ++i;
            break;
        }

        if( !isDigit( aBus[i] ) )
            return false;

        tmp += aBus[i];
    }

    // Parse suffix
    //
    for( ; i < busLen; ++i )
    {
        if( aBus[i] == '}' )
        {
            braceNesting--;
            suffix += aBus[i];
        }
        else
        {
            return false;
        }
    }

    if( braceNesting != 0 )
        return false;

    if( begin == end )
        return false;
    else if( begin > end )
        std::swap( begin, end );

    if( aName )
        *aName = prefix;

    if( aMemberList )
    {
        for( long idx = begin; idx <= end; ++idx )
        {
            wxString str = prefix;
            str << idx;
            str << suffix;

            aMemberList->emplace_back( str );
        }
    }

    return true;
}


bool NET_SETTINGS::ParseBusGroup( const wxString& aGroup, wxString* aName,
                                  std::vector<wxString>* aMemberList )
{
    size_t   groupLen = aGroup.length();
    size_t   i = 0;
    wxString prefix;
    wxString suffix;
    wxString tmp;
    int      braceNesting = 0;

    prefix.reserve( groupLen );

    // Parse prefix
    //
    for( ; i < groupLen; ++i )
    {
        if( aGroup[i] == '{' )
        {
            if( i > 0 && isSuperSubOverbar( aGroup[i-1] ) )
                braceNesting++;
            else
                break;
        }
        else if( aGroup[i] == '}' )
        {
            braceNesting--;
        }

        if( aGroup[i] == ' ' || aGroup[i] == '[' || aGroup[i] == ']' )
            return false;

        prefix += aGroup[i];
    }

    if( braceNesting != 0 )
        return false;

    if( aName )
        *aName = prefix;

    // Parse members
    //
    i++;  // '{' character

    if( i >= groupLen )
        return false;

    for( ; i < groupLen; ++i )
    {
        if( aGroup[i] == '{' )
        {
            if( i > 0 && isSuperSubOverbar( aGroup[i-1] ) )
                braceNesting++;
            else
                return false;
        }
        else if( aGroup[i] == '}' )
        {
            if( braceNesting )
            {
                braceNesting--;
            }
            else
            {
                if( aMemberList && !tmp.IsEmpty() )
                    aMemberList->push_back( EscapeString( tmp, CTX_NETNAME ) );

                return true;
            }
        }

        // Commas aren't strictly legal, but we can be pretty sure what the author had in mind.
        if( aGroup[i] == ' ' || aGroup[i] == ',' )
        {
            if( aMemberList && !tmp.IsEmpty() )
                aMemberList->push_back( EscapeString( tmp, CTX_NETNAME ) );

            tmp.Clear();
            continue;
        }

        tmp += aGroup[i];
    }

    return false;
}


void NET_SETTINGS::ResolveNetClassAssignments( bool aRebuildFromScratch )
{
    std::map<wxString, wxString> baseList;

    if( aRebuildFromScratch )
    {
        for( const std::pair<const wxString, NETCLASSPTR>& netclass : m_NetClasses )
        {
            for( const wxString& net : *netclass.second )
                baseList[ net ] = netclass.second->GetName();
        }
    }
    else
    {
        baseList = m_NetClassAssignments;
    }

    m_NetClassAssignments.clear();

    for( const auto& ii : baseList )
    {
        m_NetClassAssignments[ ii.first ] = ii.second;

        wxString unescaped = UnescapeString( ii.first );
        wxString prefix;
        std::vector<wxString> members;

        if( ParseBusVector( unescaped, &prefix, &members ) )
        {
            prefix = wxEmptyString;
        }
        else if( ParseBusGroup( unescaped, &prefix, &members ) )
        {
            if( !prefix.IsEmpty() )
                prefix += wxT( "." );
        }

        for( wxString& member : members )
            m_NetClassAssignments[ prefix + member ] = ii.second;
    }
}
