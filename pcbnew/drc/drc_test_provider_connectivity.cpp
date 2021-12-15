/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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
#include <common.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/connectivity_algo.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    Connectivity test provider. Not rule-driven.
    Errors generated:
    - DRCE_DANGLING_TRACK
    - DRCE_DANGLING_VIA
    - DRCE_ZONE_HAS_EMPTY_NET
*/

class DRC_TEST_PROVIDER_CONNECTIVITY : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_CONNECTIVITY()
    {
    }

    virtual ~DRC_TEST_PROVIDER_CONNECTIVITY()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "connectivity";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests board connectivity";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_CONNECTIVITY::Run()
{
    if( !reportPhase( _( "Checking pad, via and zone connections..." ) ) )
        return false;   // DRC cancelled

    BOARD* board = m_drcEngine->GetBoard();

    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    // Rebuild just in case. This really needs to be reliable.
    connectivity->Clear();
    connectivity->Build( board, m_drcEngine->GetProgressReporter() );

    int delta = 100;  // This is the number of tests between 2 calls to the progress bar
    int ii = 0;
    int count = board->Tracks().size() + board->Zones().size();

    ii += count;      // We gave half of this phase to CONNECTIVITY_DATA::Build()
    count += count;

    for( PCB_TRACK* track : board->Tracks() )
    {
        bool exceedT = m_drcEngine->IsErrorLimitExceeded( DRCE_DANGLING_TRACK );
        bool exceedV = m_drcEngine->IsErrorLimitExceeded( DRCE_DANGLING_VIA );

        if( exceedV && exceedT )
            break;
        else if( track->Type() == PCB_VIA_T && exceedV )
            continue;
        else if( track->Type() == PCB_TRACE_T && exceedT )
            continue;

        if( !reportProgress( ii++, count, delta ) )
            break;

        // Test for dangling items
        int code = track->Type() == PCB_VIA_T ? DRCE_DANGLING_VIA : DRCE_DANGLING_TRACK;
        wxPoint pos;

        if( connectivity->TestTrackEndpointDangling( track, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( code );
            drcItem->SetItems( track );
            reportViolation( drcItem, pos );
        }
    }

    /* test starved zones */
    for( ZONE* zone : board->Zones() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_ZONE_HAS_EMPTY_NET ) )
            break;

        if( !zone->IsOnCopperLayer() )
            continue;

        if( !reportProgress( ii++, count, delta ) )
            return false;   // DRC cancelled

        int netcode = zone->GetNetCode();
        // a netcode < 0 or > 0 and no pad in net is a error or strange
        // perhaps a "dead" net, which happens when all pads in this net were removed
        // Remark: a netcode < 0 should not happen (this is more a bug somewhere)
        int pads_in_net = ( netcode > 0 ) ? connectivity->GetPadCount( netcode ) : 1;

        if( ( netcode < 0 ) || pads_in_net == 0 )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ZONE_HAS_EMPTY_NET );
            drcItem->SetItems( zone );
            reportViolation( drcItem, zone->GetPosition() );
        }
    }

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
        return true;    // continue with other tests

    if( !reportPhase( _( "Checking net connections..." ) ) )
        return false;   // DRC cancelled

    std::vector<CN_EDGE> edges;
    connectivity->GetUnconnectedEdges( edges );

    delta = 250;
    ii = 0;
    count = edges.size();

    for( const CN_EDGE& edge : edges )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
            break;

        if( !reportProgress( ii++, count, delta ) )
            return false;   // DRC cancelled

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
        drcItem->SetItems( edge.GetSourceNode()->Parent(), edge.GetTargetNode()->Parent() );
        reportViolation( drcItem, (wxPoint) edge.GetSourceNode()->Pos() );
    }

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_CONNECTIVITY::GetNumPhases() const
{
    return 3;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_CONNECTIVITY::GetConstraintTypes() const
{
    return {};
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTIVITY> dummy;
}
