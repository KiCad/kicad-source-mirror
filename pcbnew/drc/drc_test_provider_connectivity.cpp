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
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <drc/drc_item.h>
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
    {}

    virtual ~DRC_TEST_PROVIDER_CONNECTIVITY() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "connectivity" ); };
};


bool DRC_TEST_PROVIDER_CONNECTIVITY::Run()
{
    if( !reportPhase( _( "Checking pad, via and zone connections..." ) ) )
        return false;   // DRC cancelled

    BOARD*                             board = m_drcEngine->GetBoard();
    std::shared_ptr<CONNECTIVITY_DATA> connectivity = board->GetConnectivity();

    int progressDelta = 250;
    int ii = 0;
    int count = board->Tracks().size() + board->m_ZoneIsolatedIslandsMap.size();

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
        else if( ( track->Type() == PCB_TRACE_T || track->Type() == PCB_ARC_T ) && exceedT )
            continue;

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        // Test for dangling items
        int code = track->Type() == PCB_VIA_T ? DRCE_DANGLING_VIA : DRCE_DANGLING_TRACK;
        VECTOR2I pos;

        if( connectivity->TestTrackEndpointDangling( track, true, &pos ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem;

            if( track->Type() == PCB_VIA_T )
            {
                auto constraint = m_drcEngine->EvalRules( VIA_DANGLING_CONSTRAINT, track, nullptr,
                                                         track->GetLayer() );

                if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
                    continue;

                drcItem = DRC_ITEM::Create( code );
                drcItem->SetViolatingRule( constraint.GetParentRule() );
            }
            else
            {
                drcItem = DRC_ITEM::Create( code );
            }

            drcItem->SetItems( track );
            reportViolation( drcItem, pos, track->GetLayer() );
        }
    }

    // Test for tracks connecting to post-machined or backdrilled layers
    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_ON_POST_MACHINED_LAYER ) )
    {
        for( PCB_TRACK* track : board->Tracks() )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_ON_POST_MACHINED_LAYER ) )
                break;

            // Only check traces and arcs (not vias)
            if( track->Type() != PCB_TRACE_T && track->Type() != PCB_ARC_T )
                continue;

            PCB_LAYER_ID layer = track->GetLayer();

            // Get items connected to this track
            const std::list<CN_ITEM*>& items = connectivity->GetConnectivityAlgo()->ItemEntry( track ).GetItems();

            if( items.empty() )
                continue;

            CN_ITEM* citem = items.front();

            if( !citem->Valid() )
                continue;

            for( CN_ITEM* connected : citem->ConnectedItems() )
            {
                BOARD_CONNECTED_ITEM* item = connected->Parent();

                if( item->GetFlags() & IS_DELETED )
                    continue;

                bool isPostMachined = false;

                if( item->Type() == PCB_PAD_T )
                {
                    PAD* pad = static_cast<PAD*>( item );
                    isPostMachined = pad->IsBackdrilledOrPostMachined( layer );
                }
                else if( item->Type() == PCB_VIA_T )
                {
                    PCB_VIA* via = static_cast<PCB_VIA*>( item );
                    isPostMachined = via->IsBackdrilledOrPostMachined( layer );
                }

                if( isPostMachined )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACK_ON_POST_MACHINED_LAYER );
                    drcItem->SetItems( track, item );

                    VECTOR2I pos = ( track->GetStart() + track->GetEnd() ) / 2;

                    // Use the endpoint that's closer to the pad/via
                    if( item->HitTest( track->GetStart() ) )
                        pos = track->GetStart();
                    else if( item->HitTest( track->GetEnd() ) )
                        pos = track->GetEnd();

                    reportViolation( drcItem, pos, layer );
                    break;  // Only report once per track
                }
            }
        }
    }

    /* test starved zones */
    for( const auto& [ zone, zoneIslands ] : board->m_ZoneIsolatedIslandsMap )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_ISOLATED_COPPER ) )
            break;

        if( !reportProgress( ii++, count, progressDelta ) )
            return false;   // DRC cancelled

        for( const auto& [ layer, layerIslands ] : zoneIslands )
        {
            for( int polyIdx : layerIslands.m_IsolatedOutlines )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_ISOLATED_COPPER ) )
                    break;

                std::shared_ptr<SHAPE_POLY_SET> poly = zone->GetFilledPolysList( layer );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ISOLATED_COPPER );
                drcItem->SetItems( zone );
                reportViolation( drcItem, poly->Outline( polyIdx ).CPoint( 0 ), layer );
            }
        }
    }

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
        return true;    // continue with other tests

    if( !reportPhase( _( "Checking net connections..." ) ) )
        return false;   // DRC cancelled

    ii = 0;
    count = connectivity->GetUnconnectedCount( false );

    connectivity->RunOnUnconnectedEdges(
            [&]( CN_EDGE& edge )
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_UNCONNECTED_ITEMS ) )
                    return false;

                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;   // DRC cancelled

                wxCHECK( edge.GetSourceNode() && !edge.GetSourceNode()->Dirty(), true );
                wxCHECK( edge.GetTargetNode() && !edge.GetTargetNode()->Dirty(), true );

                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
                drcItem->SetItems( edge.GetSourceNode()->Parent(), edge.GetTargetNode()->Parent() );
                reportViolation( drcItem, edge.GetSourceNode()->Pos(), UNDEFINED_LAYER );

                return true;
            } );

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_CONNECTIVITY> dummy;
}
