/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2020 CERN
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

#include <lset.h>
#include <netclass.h>

#include <io/cadstar/cadstar_archive_parser.h>
#include <project/time_domain_parameters.h>
#include <nlohmann/json.hpp>
#include <settings/parameters.h>


constexpr int timeDomainParametersSchemaVersion = 0;

TIME_DOMAIN_PARAMETERS::TIME_DOMAIN_PARAMETERS( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "time_domain_parameters", timeDomainParametersSchemaVersion, aParent,
                         aPath, false )
{
    auto saveViaOverrideConfigurationLine =
            []( nlohmann::json& json_array, const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& item )
    {
        const nlohmann::json item_json = { { "signal_layer_from", LSET::Name( item.m_SignalLayerFrom ) },
                                           { "signal_layer_to", LSET::Name( item.m_SignalLayerTo ) },
                                           { "via_layer_from", LSET::Name( item.m_ViaLayerFrom ) },
                                           { "via_layer_to", LSET::Name( item.m_ViaLayerTo ) },
                                           { "delay", item.m_Delay } };

        json_array.push_back( item_json );
    };

    auto readViaOverrideConfigurationLine = []( const nlohmann::json& entry )
    {
        wxString signalLayerFromName = entry["signal_layer_from"];
        int      signalLayerFromId = LSET::NameToLayer( signalLayerFromName );

        wxString signalLayerToName = entry["signal_layer_to"];
        int      signalLayerToId = LSET::NameToLayer( signalLayerToName );

        wxString viaLayerFromName = entry["via_layer_from"];
        int      viaLayerFromId = LSET::NameToLayer( viaLayerFromName );

        wxString viaLayerToName = entry["via_layer_to"];
        int      viaLayerToId = LSET::NameToLayer( viaLayerToName );

        int delay = entry["delay"];

        DELAY_PROFILE_VIA_OVERRIDE_ENTRY item{ static_cast<PCB_LAYER_ID>( signalLayerFromId ),
                                               static_cast<PCB_LAYER_ID>( signalLayerToId ),
                                               static_cast<PCB_LAYER_ID>( viaLayerFromId ),
                                               static_cast<PCB_LAYER_ID>( viaLayerToId ), delay };

        return item;
    };

    auto saveUserDefinedProfileConfigurationLine =
            [&saveViaOverrideConfigurationLine]( nlohmann::json& json_array, const DELAY_PROFILE& item )
    {
        nlohmann::json layer_velocities = nlohmann::json::array();

        for( const auto& [layerId, velocity] : item.m_LayerPropagationDelays )
        {
            nlohmann::json layer_json = { { "layer", LSET::Name( layerId ) }, { "delay", velocity } };
            layer_velocities.push_back( layer_json );
        }

        nlohmann::json via_overrides = nlohmann::json::array();

        for( const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& viaOverride : item.m_ViaOverrides )
        {
            saveViaOverrideConfigurationLine( via_overrides, viaOverride );
        }

        const nlohmann::json item_json = { { "profile_name", item.m_ProfileName.ToUTF8() },
                                           { "via_prop_delay", item.m_ViaPropagationDelay },
                                           { "layer_delays", layer_velocities },
                                           { "via_overrides", via_overrides } };

        json_array.push_back( item_json );
    };

    auto readUserDefinedProfileConfigurationLine = [&readViaOverrideConfigurationLine]( const nlohmann::json& entry )
    {
        const wxString              profileName = entry["profile_name"];
        const int                   viaPropDelay = entry["via_prop_delay"];
        std::map<PCB_LAYER_ID, int> traceDelays;

        for( const nlohmann::json& layerEntry : entry["layer_delays"] )
        {
            if( !layerEntry.is_object() || !layerEntry.contains( "layer" ) )
                continue;

            wxString  layerName = layerEntry["layer"];
            int       layerId = LSET::NameToLayer( layerName );
            const int velocity = layerEntry["delay"];
            traceDelays[static_cast<PCB_LAYER_ID>( layerId )] = velocity;
        }

        std::vector<DELAY_PROFILE_VIA_OVERRIDE_ENTRY> viaOverrides;

        for( const nlohmann::json& viaEntry : entry["via_overrides"] )
        {
            if( !viaEntry.is_object() || !viaEntry.contains( "signal_layer_from" ) )
                continue;

            viaOverrides.push_back( readViaOverrideConfigurationLine( viaEntry ) );
        }

        DELAY_PROFILE item{ profileName, viaPropDelay, std::move( traceDelays ), std::move( viaOverrides ) };

        return item;
    };

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "delay_profiles_user_defined",
            [&]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const auto& entry : m_delayProfiles )
                    saveUserDefinedProfileConfigurationLine( ret, entry );

                return ret;
            },
            [&]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                ClearDelayProfiles();

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() || !entry.contains( "profile_name" ) )
                        continue;

                    m_delayProfiles.emplace_back( readUserDefinedProfileConfigurationLine( entry ) );
                }
            },
            {} ) );
}


TIME_DOMAIN_PARAMETERS::~TIME_DOMAIN_PARAMETERS()
{
    // Release early before destroying members
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


bool TIME_DOMAIN_PARAMETERS::operator==( const TIME_DOMAIN_PARAMETERS& aOther ) const
{
    /*
    if( !std::equal( std::begin( m_netClasses ), std::end( m_netClasses ),
                     std::begin( aOther.m_netClasses ) ) )
        return false;

    if( !std::equal( std::begin( m_netClassPatternAssignments ),
                     std::end( m_netClassPatternAssignments ),
                     std::begin( aOther.m_netClassPatternAssignments ) ) )
        return false;

    if( !std::equal( std::begin( m_netClassLabelAssignments ),
                     std::end( m_netClassLabelAssignments ),
                     std::begin( aOther.m_netClassLabelAssignments ) ) )
        return false;


    if( !std::equal( std::begin( m_netColorAssignments ), std::end( m_netColorAssignments ),
                     std::begin( aOther.m_netColorAssignments ) ) )
        return false;
     */

    return true;
}
