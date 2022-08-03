/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2022 KiCad Developers.
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
    - DRCE_ISOLATED_COPPER
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
        return wxT( "connectivity" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests board connectivity" );
    }
};


bool DRC_TEST_PROVIDER_CONNECTIVITY::Run()
{
    if( !reportPhase( _( "Checking pad, via and zone connections..." ) ) )
        return false;   // DRC cancelled

    BOARD* board = m_drcEngine->GetBoard();

    std::shared_ptr<CONNECTIVITY_DATA>        connectivity = board->GetConnectivity();
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> islandsList;

    for( ZONE* zone : board->Zones() )
    {
        if( !zone->GetIsRuleArea() )
            islandsList.emplace_back( CN_ZONE_ISOLATED_ISLAND_LIST( zone ) );
    }

    // Rebuild just in case. This really needs to be reliable.
    connectivity->Clear();
    connectivity->Build( board, m_drcEngine->GetProgressReporter() );
    connectivity->FindIsolatedCopperIslands( islandsList, true );

    int progressDelta = 250;
    int ii = 0;
    int count = board->Tracks().size() + islandsList.size();

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

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        // Test for dangling items
        int code = track->Type() == PCB_VIA_T ? DRCE_DANGLING_VIA : DRCE_DANGLING_TRACK;
        VECTOR2I pos;

        if( connectivity->TestTrackEndpointDangling( track, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( code );
            drcItem->SetItems( track );
            reportViolation( drcItem, pos, track->GetLayer() );
        }
    }

    /* test starved zones */
    for( CN_ZONE_ISOLATED_ISLAND_LIST& zone : islandsList )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_ISOLATED_COPPER ) )
            break;

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        for( PCB_LAYER_ID layer : zone.m_zone->GetLayerSet().Seq() )
        {
            if( !zone.m_islands.count( layer ) )
                continue;

            std::shared_ptr<SHAPE_POLY_SET> poly = zone.m_zone->GetFilledPolysList( layer );

            for( int idx : zone.m_islands.at( layer ) )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ISOLATED_COPPER ) )
                    break;

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ISOLATED_COPPER );
                drcItem->SetItems( zone.m_zone );
                reportViolation( drcItem, poly->Outline( idx ).CPoint( 0 ), layer );
            }
        }
    }

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
        return true;    // continue with other tests

    if( !reportPhase( _( "Checking net connections..." ) ) )
        return false;   // DRC cancelled

    std::vector<CN_EDGE> edges;
    connectivity->GetUnconnectedEdges( edges );

    ii = 0;
    count = edges.size();

    for( const CN_EDGE& edge : edges )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
            break;

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
        drcItem->SetItems( edge.GetSourceNode()->Parent(), edge.GetTargetNode()->Parent() );
        reportViolation( drcItem, edge.GetSourceNode()->Pos(), UNDEFINED_LAYER );
    }

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTIVITY> dummy;
}
