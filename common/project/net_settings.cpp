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
        return PcbMm2iu( aObj[aKey].get<double>() );
    else
        return aDefault;
};


static int getInSchUnits( const nlohmann::json& aObj, const std::string& aKey, int aDefault )
{
    if( aObj.contains( aKey ) && aObj[aKey].is_number() )
        return SchMils2iu( aObj[aKey].get<double>() );
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

                NETCLASSPTR                nc    = m_NetClasses.GetDefault();
                NETCLASSES::const_iterator nc_ii = m_NetClasses.begin();

                for( unsigned int idx = 0; idx <= m_NetClasses.GetCount(); idx++ )
                {
                    if( idx > 0 )
                    {
                        nc = nc_ii->second;
                        ++nc_ii;
                    }

                    // Note: we're in common/, but we do happen to know which of these fields
                    // are used in which units system.
                    nlohmann::json nc_json = {
                        { "name",            nc->GetName().ToUTF8() },
                        { "wire_width",      SchIu2Mils( nc->GetWireWidth() ) },
                        { "bus_width",       SchIu2Mils( nc->GetBusWidth() ) },
                        { "line_style",      nc->GetLineStyle() },
                        { "schematic_color", nc->GetSchematicColor() },
                        { "pcb_color",       nc->GetPcbColor() }
                    };

                    auto saveInPcbUnits =
                            []( nlohmann::json& json, const std::string& aKey, int aValue )
                            {
                                json.push_back( { aKey, PcbIu2mm( aValue ) } );
                            };

                    if( nc->HasClearance() )
                        saveInPcbUnits( nc_json, "clearance", nc->GetClearance() );

                    if( nc->HasTrackWidth() )
                        saveInPcbUnits( nc_json, "track_width", nc->GetTrackWidth() );

                    if( nc->HasViaDiameter() )
                        saveInPcbUnits( nc_json, "via_diameter", nc->GetViaDiameter() );

                    if( nc->HasViaDrill() )
                        saveInPcbUnits( nc_json, "via_drill", nc->GetViaDrill() );

                    if( nc->HasuViaDiameter() )
                        saveInPcbUnits( nc_json, "microvia_diameter", nc->GetuViaDiameter() );

                    if( nc->HasuViaDrill() )
                        saveInPcbUnits( nc_json, "microvia_drill", nc->GetuViaDrill()  );

                    if( nc->HasDiffPairWidth() )
                        saveInPcbUnits( nc_json, "diff_pair_width", nc->GetDiffPairWidth() );

                    if( nc->HasDiffPairGap() )
                        saveInPcbUnits( nc_json, "diff_pair_gap", nc->GetDiffPairGap() );

                    if( nc->HasDiffPairViaGap() )
                        saveInPcbUnits( nc_json, "diff_pair_via_gap", nc->GetDiffPairViaGap() );

                    if( idx > 0 )   // No need to store members of Default nc
                    {
                        nlohmann::json membersJson = nlohmann::json::array();

                        for( const wxString& member : *nc )
                        {
                            if( !member.empty() )
                                membersJson.push_back( member );
                        }

                        nc_json["nets"] = membersJson;
                    }

                    ret.push_back( nc_json );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_NetClasses.Clear();
                m_NetClassAssignments.clear();
                NETCLASSPTR nc;
                NETCLASSPTR defaultClass = m_NetClasses.GetDefault();

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() || !entry.contains( "name" ) )
                        continue;

                    wxString name = entry["name"];

                    if( name == defaultClass->GetName() )
                        nc = defaultClass;
                    else
                        nc = std::make_shared<NETCLASS>( name );

                    if( auto value = getInPcbUnits( entry, "clearance" ) )
                        nc->SetClearance( *value );

                    if( auto value = getInPcbUnits( entry, "track_width" ) )
                        nc->SetTrackWidth( *value );

                    if( auto value = getInPcbUnits( entry, "via_diameter" ) )
                        nc->SetViaDiameter( *value );

                    if( auto value = getInPcbUnits( entry, "via_drill" ) )
                        nc->SetViaDrill( *value );

                    if( auto value = getInPcbUnits( entry, "microvia_diameter" ) )
                        nc->SetuViaDiameter( *value );

                    if( auto value = getInPcbUnits( entry, "microvia_drill" ) )
                        nc->SetuViaDrill( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_width" ) )
                        nc->SetDiffPairWidth( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_gap" ) )
                        nc->SetDiffPairGap( *value );

                    if( auto value = getInPcbUnits( entry, "diff_pair_via_gap" ) )
                        nc->SetDiffPairViaGap( *value );

                    nc->SetWireWidth( getInSchUnits( entry, "wire_width", nc->GetWireWidth() ) );
                    nc->SetBusWidth( getInSchUnits( entry, "bus_width", nc->GetBusWidth() ) );

                    if( entry.contains( "line_style" ) && entry["line_style"].is_number() )
                        nc->SetLineStyle( entry["line_style"].get<int>() );

                    if( entry.contains( "nets" ) && entry["nets"].is_array() )
                    {
                        for( const auto& net : entry["nets"].items() )
                            nc->Add( net.value().get<wxString>() );
                    }

                    if( entry.contains( "pcb_color" ) && entry["pcb_color"].is_string() )
                        nc->SetPcbColor( entry["pcb_color"].get<KIGFX::COLOR4D>() );

                    if( entry.contains( "schematic_color" )
                            && entry["schematic_color"].is_string() )
                    {
                        nc->SetSchematicColor( entry["schematic_color"].get<KIGFX::COLOR4D>() );
                    }

                    if( nc != defaultClass )
                        m_NetClasses.Add( nc );

                    for( const wxString& net : *nc )
                        m_NetClassAssignments[ net ] = nc->GetName();
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
