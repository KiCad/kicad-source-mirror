/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
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

#include <project/net_settings.h>
#include <settings/parameters.h>
#include <settings/settings_manager.h>
#include <kicad_string.h>
#include <convert_to_biu.h>

const int netSettingsSchemaVersion = 0;


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
                    nlohmann::json netJson = {
                        { "name",              netclass->GetName().ToUTF8() },
                        { "clearance",         PcbIu2Millimeter( netclass->GetClearance() ) },
                        { "track_width",       PcbIu2Millimeter( netclass->GetTrackWidth() ) },
                        { "via_diameter",      PcbIu2Millimeter( netclass->GetViaDiameter() ) },
                        { "via_drill",         PcbIu2Millimeter( netclass->GetViaDrill() ) },
                        { "microvia_diameter", PcbIu2Millimeter( netclass->GetuViaDiameter() ) },
                        { "microvia_drill",    PcbIu2Millimeter( netclass->GetuViaDrill() ) },
                        { "diff_pair_width",   PcbIu2Millimeter( netclass->GetDiffPairWidth() ) },
                        { "diff_pair_gap",     PcbIu2Millimeter( netclass->GetDiffPairGap() ) },
                        { "diff_pair_via_gap", PcbIu2Millimeter( netclass->GetDiffPairViaGap() ) }
                        };

                    if( netclass->GetPcbColor() != KIGFX::COLOR4D::UNSPECIFIED )
                        netJson["pcb_color"] = netclass->GetPcbColor();

                    if( idx > 0 )
                    {
                        nlohmann::json membersJson = nlohmann::json::array();

                        for( const auto& ii : *netclass )
                        {
                            if( !ii.empty() )
                                membersJson.push_back( ii );
                        }

                        netJson["nets"] = membersJson;
                        ret.push_back( netJson );
                    }
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

                auto get =
                        []( const nlohmann::json& aObj, const std::string& aKey, int aDefault )
                        {
                            if( aObj.contains( aKey ) )
                                return PcbMillimeter2iu( aObj[aKey].get<double>() );
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

                    netclass->SetClearance( get( entry, "clearance", netclass->GetClearance() ) );
                    netclass->SetTrackWidth(
                            get( entry, "track_width", netclass->GetTrackWidth() ) );
                    netclass->SetViaDiameter(
                            get( entry, "via_diameter", netclass->GetViaDiameter() ) );
                    netclass->SetViaDrill( get( entry, "via_drill", netclass->GetViaDrill() ) );
                    netclass->SetuViaDiameter(
                            get( entry, "microvia_diameter", netclass->GetuViaDiameter() ) );
                    netclass->SetuViaDrill(
                            get( entry, "microvia_drill", netclass->GetuViaDrill() ) );
                    netclass->SetDiffPairWidth(
                            get( entry, "diff_pair_width", netclass->GetDiffPairWidth() ) );
                    netclass->SetDiffPairGap(
                            get( entry, "diff_pair_gap", netclass->GetDiffPairGap() ) );
                    netclass->SetDiffPairViaGap(
                            get( entry, "diff_pair_via_gap", netclass->GetDiffPairViaGap() ) );

                    if( entry.contains( "nets" ) && entry["nets"].is_array() )
                    {
                        for( const auto& net : entry["nets"].items() )
                            netclass->Add( net.value().get<wxString>() );
                    }

                    if( entry.contains( "pcb_color" ) && entry["pcb_color"].is_string() )
                        netclass->SetPcbColor( entry["pcb_color"].get<KIGFX::COLOR4D>() );

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

    m_params.emplace_back( new PARAM_LIST<wxString>( "hidden_nets", &m_HiddenNets, {} ) );
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


static bool isSuperSub( wxChar c )
{
    return c == '_' || c == '^';
};


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
            if( i > 0 && isSuperSub( aBus[i-1] ) )
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
        else if( aBus[i] == '~' )
        {
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


bool NET_SETTINGS::ParseBusGroup( wxString aGroup, wxString* aName,
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
            if( i > 0 && isSuperSub( aGroup[i-1] ) )
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
            if( i > 0 && isSuperSub( aGroup[i-1] ) )
                braceNesting++;
            else
                return false;
        }
        else if( aGroup[i] == '}' )
        {
            if( braceNesting )
                braceNesting--;
            else
            {
                if( aMemberList )
                    aMemberList->push_back( tmp );

                return true;
            }
        }

        if( aGroup[i] == ' ' )
        {
            if( aMemberList )
                aMemberList->push_back( tmp );

            tmp.Clear();
            continue;
        }

        tmp += aGroup[i];
    }

    return false;
}


void NET_SETTINGS::ResolveNetClassAssignments()
{
    std::map<wxString, wxString> existing = m_NetClassAssignments;

    m_NetClassAssignments.clear();

    for( const auto& ii : existing )
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
