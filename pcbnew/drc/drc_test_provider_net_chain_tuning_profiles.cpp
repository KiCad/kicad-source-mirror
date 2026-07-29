/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <thread_pool.h>
#include "geometry/eda_angle.h"
#include <numbers>
#include <pcb_track.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <connectivity/connectivity_data.h>


/*
    Net chain tuning profile checks. Ensures that all nets in a net chain
    have a consistent tuning profile assigned
*/

class DRC_TEST_PROVIDER_NET_CHAIN_TUNING_PROFILES : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_NET_CHAIN_TUNING_PROFILES() {}

    virtual ~DRC_TEST_PROVIDER_NET_CHAIN_TUNING_PROFILES() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "net chain tuning profiles" ); };
};


bool DRC_TEST_PROVIDER_NET_CHAIN_TUNING_PROFILES::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CHAIN_TUNING_PROFILES ) )
    {
        REPORT_AUX( wxT( "Net chain tuning profiles ignored. Tests not run." ) );
        return true; // continue with other tests
    }

    if( !reportPhase( _( "Checking net chain tuning profiles..." ) ) )
        return false; // DRC cancelled

    // Map of netchains to all declared tuning profile names that are a member of that net

    const NETINFO_LIST& netsList = m_drcEngine->GetBoard()->GetNetInfo();

    // Map of net chain name : net chain nets tuning profiles names
    std::unordered_map<wxString, std::unordered_set<wxString>> netChainTuningProfileMap;

    for( const auto net : netsList )
    {
        const wxString& netChain = net->GetNetChain();

        if( netChain == wxEmptyString )
            continue;

        const NETCLASS* netClass = net->GetNetClass();
        netChainTuningProfileMap[netChain].insert( netClass->GetTuningProfile() );
    }

    for( const auto& [netChain, tuningProfiles] : netChainTuningProfileMap )
    {
        if( tuningProfiles.size() == 1 )
            continue;

        if( !m_drcEngine->IsErrorLimitExceeded( DRCE_NET_CHAIN_TUNING_PROFILES ) )
        {
            wxString msg;
            msg.Printf( _( "Net chain %s nets have multiple tuning profiles assigned" ), netChain );

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_NET_CHAIN_TUNING_PROFILES );

            drcItem->SetErrorMessage( msg );
            reportViolation( drcItem, VECTOR2I(), UNDEFINED_LAYER );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_NET_CHAIN_TUNING_PROFILES> dummy;
}
