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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <board.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <length_delay_calculation/tuning_profile_parameters_user_defined.h>
#include <project/project_file.h>


void TUNING_PROFILE_PARAMETERS_USER_DEFINED::OnSettingsChanged()
{
    rebuildCaches();
}


std::vector<int64_t>
TUNING_PROFILE_PARAMETERS_USER_DEFINED::GetPropagationDelays( const std::vector<LENGTH_DELAY_CALCULATION_ITEM>& aItems,
                                                              const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext )
{
    if( aItems.empty() )
        return {};

    const wxString        delayProfileName = aItems.front().GetEffectiveNetClass()->GetTuningProfile();
    const TUNING_PROFILE* delayProfile = GetTuningProfile( delayProfileName );

    if( !delayProfile )
        return std::vector<int64_t>( aItems.size(), 0 );

    std::vector<int64_t> propagationDelays;
    propagationDelays.reserve( aItems.size() );

    for( const LENGTH_DELAY_CALCULATION_ITEM& item : aItems )
        propagationDelays.emplace_back( getPropagationDelay( item, aContext, delayProfile ) );

    return propagationDelays;
}


int64_t TUNING_PROFILE_PARAMETERS_USER_DEFINED::GetPropagationDelay( const LENGTH_DELAY_CALCULATION_ITEM&   aItem,
                                                                     const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext )
{
    if( aItem.GetMergeStatus() == LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED )
        return 0;

    const wxString        delayProfileName = aItem.GetEffectiveNetClass()->GetTuningProfile();
    const TUNING_PROFILE* delayProfile = GetTuningProfile( delayProfileName );

    if( !delayProfile )
        return 0;

    return getPropagationDelay( aItem, aContext, delayProfile );
}


int64_t TUNING_PROFILE_PARAMETERS_USER_DEFINED::getPropagationDelay( const LENGTH_DELAY_CALCULATION_ITEM&   aItem,
                                                                     const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext,
                                                                     const TUNING_PROFILE* aDelayProfile ) const
{
    if( aItem.GetMergeStatus() == LENGTH_DELAY_CALCULATION_ITEM::MERGE_STATUS::MERGED_RETIRED )
        return 0;

    const LENGTH_DELAY_CALCULATION_ITEM::TYPE itemType = aItem.Type();

    if( itemType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::LINE )
    {
        double delayUnit = 0.0;

        if( aDelayProfile->m_TrackPropagationEntriesMap.contains( aItem.GetStartLayer() ) )
        {
            const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& entry =
                    aDelayProfile->m_TrackPropagationEntriesMap.at( aItem.GetStartLayer() );
            delayUnit = entry.GetDelay();
        }

        return static_cast<int64_t>( delayUnit * ( static_cast<double>( aItem.GetLine().Length() ) / PCB_IU_PER_MM ) );
    }

    if( itemType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::VIA )
    {
        if( !aDelayProfile->m_EnableTimeDomainTuning )
            return 0;

        const PCB_LAYER_ID signalStartLayer = aItem.GetStartLayer();
        const PCB_LAYER_ID signalEndLayer = aItem.GetEndLayer();
        const PCB_LAYER_ID viaStartLayer = aItem.GetVia()->Padstack().StartLayer();
        const PCB_LAYER_ID viaEndLayer = aItem.GetVia()->Padstack().EndLayer();

        // First check for a layer-to-layer override - this assumes that the layers are already in CuStack() order
        auto& viaOverrides = m_viaOverridesCache.at( aDelayProfile->m_ProfileName );

        const auto viaItr = viaOverrides.find(
                VIA_OVERRIDE_CACHE_KEY{ signalStartLayer, signalEndLayer, viaStartLayer, viaEndLayer } );

        if( viaItr != viaOverrides.end() )
            return viaItr->second;

        // Otherwise, return the tuning profile default
        const double distance = m_lengthCalculation->StackupHeight( signalStartLayer, signalEndLayer );
        return static_cast<int64_t>( aDelayProfile->m_ViaPropagationDelay * ( distance / PCB_IU_PER_MM ) );
    }

    if( itemType == LENGTH_DELAY_CALCULATION_ITEM::TYPE::PAD )
    {
        return aItem.GetPad()->GetPadToDieDelay();
    }

    return 0;
}


const TUNING_PROFILE* TUNING_PROFILE_PARAMETERS_USER_DEFINED::GetTuningProfile( const wxString& aDelayProfileName )
{
    auto itr = m_delayProfilesCache.find( aDelayProfileName );

    if( itr != m_delayProfilesCache.end() )
        return itr->second;

    return nullptr;
}


int64_t TUNING_PROFILE_PARAMETERS_USER_DEFINED::GetTrackLengthForPropagationDelay(
        int64_t aDelay, const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext )
{
    const wxString        delayProfileName = aContext.NetClass->GetTuningProfile();
    const TUNING_PROFILE* profile = GetTuningProfile( delayProfileName );

    if( !profile )
        return 0;

    double delayUnit = 0.0;

    if( profile->m_TrackPropagationEntriesMap.contains( aContext.Layer ) )
    {
        const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& entry = profile->m_TrackPropagationEntriesMap.at( aContext.Layer );
        delayUnit = entry.GetDelay();
    }

    const double lengthInMM = static_cast<double>( aDelay ) / delayUnit; // MM
    return static_cast<int64_t>( lengthInMM * PCB_IU_PER_MM );           // Length IU
}


int64_t TUNING_PROFILE_PARAMETERS_USER_DEFINED::CalculatePropagationDelayForShapeLineChain(
        const SHAPE_LINE_CHAIN& aShape, const TUNING_PROFILE_GEOMETRY_CONTEXT& aContext )
{
    const wxString        delayProfileName = aContext.NetClass->GetTuningProfile();
    const TUNING_PROFILE* profile = GetTuningProfile( delayProfileName );

    if( !profile )
        return 0;

    double delayUnit = 0.0;

    if( profile->m_TrackPropagationEntriesMap.contains( aContext.Layer ) )
    {
        const DELAY_PROFILE_TRACK_PROPAGATION_ENTRY& entry = profile->m_TrackPropagationEntriesMap.at( aContext.Layer );
        delayUnit = entry.GetDelay();
    }

    return static_cast<int64_t>( delayUnit * ( static_cast<double>( aShape.Length() ) / PCB_IU_PER_MM ) );
}


void TUNING_PROFILE_PARAMETERS_USER_DEFINED::rebuildCaches()
{
    m_delayProfilesCache.clear();
    m_viaOverridesCache.clear();

    if( const PROJECT* project = m_board->GetProject() )
    {
        TUNING_PROFILES* params = project->GetProjectFile().TuningProfileParameters().get();

        for( const TUNING_PROFILE& profile : params->GetTuningProfiles() )
        {
            m_delayProfilesCache[profile.m_ProfileName] = &profile;
            std::map<VIA_OVERRIDE_CACHE_KEY, int64_t>& viaOverrides = m_viaOverridesCache[profile.m_ProfileName];

            for( const DELAY_PROFILE_VIA_OVERRIDE_ENTRY& viaOverride : profile.m_ViaOverrides )
            {
                viaOverrides[VIA_OVERRIDE_CACHE_KEY{ viaOverride.m_SignalLayerFrom, viaOverride.m_SignalLayerTo,
                                                     viaOverride.m_ViaLayerFrom, viaOverride.m_ViaLayerTo }] =
                        viaOverride.m_Delay;
            }
        }
    }
}
