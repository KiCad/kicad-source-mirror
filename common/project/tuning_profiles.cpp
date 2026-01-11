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
#include <project/tuning_profiles.h>
#include <nlohmann/json.hpp>
#include <settings/parameters.h>


constexpr int tuningParametersSchemaVersion = 0;

TUNING_PROFILES::TUNING_PROFILES( JSON_SETTINGS* aParent, const std::string& aPath ) :
        NESTED_SETTINGS( "tuning_profiles", tuningParametersSchemaVersion, aParent, aPath, false )
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
            [saveViaOverrideConfigurationLine]( nlohmann::json& json_array, const TUNING_PROFILE& item )
    {
        nlohmann::json layer_entries = nlohmann::json::array();

        for( const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& trackEntry : item.m_TrackPropagationEntries )
        {
            nlohmann::json layer_json;

            layer_json["signal_layer"] = LSET::Name( trackEntry.m_signalLayer );
            layer_json["top_reference_layer"] = LSET::Name( trackEntry.m_topReferenceLayer );
            layer_json["bottom_reference_layer"] = LSET::Name( trackEntry.m_bottomReferenceLayer );
            layer_json["width"] = trackEntry.m_width;
            layer_json["diff_pair_gap"] = trackEntry.m_diffPairGap;
            layer_json["delay"] = trackEntry.m_delay;

            layer_entries.push_back( layer_json );
        }

        nlohmann::json via_overrides = nlohmann::json::array();

        for( const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& viaOverride : item.m_ViaOverrides )
        {
            saveViaOverrideConfigurationLine( via_overrides, viaOverride );
        }

        const nlohmann::json item_json = { { "profile_name", item.m_ProfileName.ToUTF8() },
                                           { "type", static_cast<int>( item.m_Type ) },
                                           { "target_impedance", item.m_TargetImpedance },
                                           { "enable_time_domain_tuning", item.m_EnableTimeDomainTuning },
                                           { "layer_entries", layer_entries },
                                           { "via_prop_delay", item.m_ViaPropagationDelay },
                                           { "via_overrides", via_overrides } };

        json_array.push_back( item_json );
    };

    auto readUserDefinedProfileConfigurationLine = [readViaOverrideConfigurationLine]( const nlohmann::json& entry )
    {
        const wxString                     profileName = entry["profile_name"];
        const TUNING_PROFILE::PROFILE_TYPE profileType = static_cast<TUNING_PROFILE::PROFILE_TYPE>( entry["type"] );
        const double                       targetImpedance = entry["target_impedance"];
        const bool                         enableTimeDomainTuning = entry["enable_time_domain_tuning"];
        const int                          viaPropDelay = entry["via_prop_delay"];
        std::vector<DELAY_PROFILE_TRACK_PROPAGATION_ENTRY>            trackEntries;
        std::map<PCB_LAYER_ID, DELAY_PROFILE_TRACK_PROPAGATION_ENTRY> trackEntriesMap;

        for( const nlohmann::json& layerEntry : entry["layer_entries"] )
        {
            if( !layerEntry.is_object() )
                continue;

            wxString signalLayer = layerEntry["signal_layer"];
            wxString topRefLayer = layerEntry["top_reference_layer"];
            wxString bottomRefLayer = layerEntry["bottom_reference_layer"];
            int      signalLayerId = LSET::NameToLayer( signalLayer );
            int      topRefLayerId = LSET::NameToLayer( topRefLayer );
            int      bottomRefLayerId = LSET::NameToLayer( bottomRefLayer );

            DELAY_PROFILE_TRACK_PROPAGATION_ENTRY trackEntry;
            trackEntry.m_signalLayer = static_cast<PCB_LAYER_ID>( signalLayerId );
            trackEntry.m_topReferenceLayer = static_cast<PCB_LAYER_ID>( topRefLayerId );
            trackEntry.m_bottomReferenceLayer = static_cast<PCB_LAYER_ID>( bottomRefLayerId );
            trackEntry.m_width = layerEntry["width"];
            trackEntry.m_diffPairGap = layerEntry["diff_pair_gap"];
            trackEntry.m_delay = layerEntry["delay"];
            trackEntry.m_enableTimeDomainTuning = enableTimeDomainTuning;

            trackEntries.push_back( trackEntry );
            trackEntriesMap[static_cast<PCB_LAYER_ID>( signalLayerId )] = trackEntry;
        }

        std::vector<DELAY_PROFILE_VIA_OVERRIDE_ENTRY> viaOverrides;

        for( const nlohmann::json& viaEntry : entry["via_overrides"] )
        {
            if( !viaEntry.is_object() || !viaEntry.contains( "signal_layer_from" ) )
                continue;

            viaOverrides.push_back( readViaOverrideConfigurationLine( viaEntry ) );
        }

        TUNING_PROFILE item{ profileName,
                             profileType,
                             targetImpedance,
                             enableTimeDomainTuning,
                             std::move( trackEntries ),
                             viaPropDelay,
                             std::move( viaOverrides ),
                             std::move( trackEntriesMap ) };

        return item;
    };

    m_params.emplace_back( new PARAM_LAMBDA<nlohmann::json>(
            "tuning_profiles_impedance_geometric",
            [this, saveUserDefinedProfileConfigurationLine]() -> nlohmann::json
            {
                nlohmann::json ret = nlohmann::json::array();

                for( const auto& entry : m_tuningProfiles )
                    saveUserDefinedProfileConfigurationLine( ret, entry );

                return ret;
            },
            [this, readUserDefinedProfileConfigurationLine]( const nlohmann::json& aJson )
            {
                if( !aJson.is_array() )
                    return;

                ClearTuningProfiles();

                for( const nlohmann::json& entry : aJson )
                {
                    if( !entry.is_object() || !entry.contains( "profile_name" ) )
                        continue;

                    m_tuningProfiles.emplace_back( readUserDefinedProfileConfigurationLine( entry ) );
                }
            },
            {} ) );
}


TUNING_PROFILES::~TUNING_PROFILES()
{
    // Release early before destroying members
    if( m_parent )
    {
        m_parent->ReleaseNestedSettings( this );
        m_parent = nullptr;
    }
}


TUNING_PROFILE& TUNING_PROFILES::GetTuningProfile( wxString aProfileName )
{
    auto itr = std::find_if( m_tuningProfiles.begin(), m_tuningProfiles.end(),
                             [&aProfileName]( const TUNING_PROFILE& aProfile )
                             {
                                 return aProfile.m_ProfileName == aProfileName;
                             } );

    if( itr == m_tuningProfiles.end() )
        return m_nullDelayProfile;

    return *itr;
}


bool TUNING_PROFILES::operator==( const TUNING_PROFILES& aOther ) const
{
    return m_tuningProfiles == aOther.m_tuningProfiles;
}
