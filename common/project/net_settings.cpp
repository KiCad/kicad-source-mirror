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

// Netclasses were originally only stored in board files.  The IU context is PCBNEW.
#ifndef PCBNEW
#define PCBNEW
#endif
#include <base_units.h>


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

                    nlohmann::json netJson = {
                        { "name",               netclass->GetName().ToUTF8() },
                        { "clearance",          Iu2Millimeter( netclass->GetClearance() ) },
                        { "track_width",        Iu2Millimeter( netclass->GetTrackWidth() ) },
                        { "via_diameter",       Iu2Millimeter( netclass->GetViaDiameter() ) },
                        { "via_drill",          Iu2Millimeter( netclass->GetViaDrill() ) },
                        { "microvia_diameter",  Iu2Millimeter( netclass->GetuViaDiameter() ) },
                        { "microvia_drill",     Iu2Millimeter( netclass->GetuViaDrill() ) },
                        { "diff_pair_width",    Iu2Millimeter( netclass->GetDiffPairWidth() ) },
                        { "diff_pair_gap",      Iu2Millimeter( netclass->GetDiffPairGap() ) },
                        { "diff_pair_via_gap",  Iu2Millimeter( netclass->GetDiffPairViaGap() ) }
                        };

                    nlohmann::json nets = nlohmann::json::array();

                    for( NETCLASS::const_iterator i = netclass->begin(); i != netclass->end(); ++i )
                        if( !i->empty() )
                            nets.push_back( std::string( i->ToUTF8() ) );

                    netJson["nets"] = nets;

                    ret.push_back( netJson );
                }

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                m_NetClasses.Clear();
                NETCLASSPTR netclass;
                NETCLASSPTR defaultClass = m_NetClasses.GetDefault();

                auto get =
                        []( const nlohmann::json& aObj, const std::string& aKey, int aDefault )
                        {
                            if( aObj.contains( aKey ) )
                                return Millimeter2iu( aObj[aKey].get<double>() );
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

                    netclass->SetClearance( get( aJson, "clearance", netclass->GetClearance() ) );
                    netclass->SetTrackWidth(
                            get( aJson, "track_width", netclass->GetTrackWidth() ) );
                    netclass->SetViaDiameter(
                            get( aJson, "via_diameter", netclass->GetViaDiameter() ) );
                    netclass->SetViaDrill( get( aJson, "via_drill", netclass->GetViaDrill() ) );
                    netclass->SetuViaDiameter(
                            get( aJson, "microvia_diameter", netclass->GetuViaDiameter() ) );
                    netclass->SetuViaDrill(
                            get( aJson, "microvia_drill", netclass->GetuViaDrill() ) );
                    netclass->SetDiffPairWidth(
                            get( aJson, "diff_pair_width", netclass->GetDiffPairWidth() ) );
                    netclass->SetDiffPairGap(
                            get( aJson, "diff_pair_gap", netclass->GetDiffPairGap() ) );
                    netclass->SetDiffPairViaGap(
                            get( aJson, "diff_pair_via_gap", netclass->GetDiffPairViaGap() ) );

                    if( netclass != defaultClass )
                        m_NetClasses.Add( netclass );
                }
            }, {} ) );
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
