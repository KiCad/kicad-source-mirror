/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <common.h>
#include <numeric>
#include <board.h>
#include <board_design_settings.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <zone.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_length_report.h>
#include <drc/drc_chain_topology.h>
#include <length_delay_calculation/length_delay_calculation.h>
#include <lset.h>
#include <net_chain_bridging.h>
#include <geometry/shape_poly_set.h>
#include <string_utils.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>


/*
    Single-ended matched length + skew + via count test.
    Errors generated:
    - DRCE_LENGTH_OUT_OF_RANGE
    - DRCE_SKEW_OUT_OF_RANGE
    - DRCE_TOO_MANY_VIAS
    Todo: arc support
*/

class DRC_TEST_PROVIDER_MATCHED_LENGTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_MATCHED_LENGTH () :
        m_board( nullptr )
    {}

    virtual ~DRC_TEST_PROVIDER_MATCHED_LENGTH() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "length" ); };

private:

    bool runInternal( bool aDelayReportMode = false );

    using CONNECTION = DRC_LENGTH_REPORT::ENTRY;

    void checkLengths( const DRC_CONSTRAINT& aConstraint,
                       const std::vector<CONNECTION>& aMatchedConnections );
    void checkSkews( const DRC_CONSTRAINT& aConstraint,
                     const std::vector<CONNECTION>& aMatchedConnections );
    void checkViaCounts( const DRC_CONSTRAINT& aConstraint,
                         const std::vector<CONNECTION>& aMatchedConnections );
    void checkStubLengths( const DRC_CONSTRAINT& aConstraint,
                           DRC_RULE* aRule,
                           const std::vector<CONNECTION>& aMatchedConnections );
    void checkReturnPath( const DRC_CONSTRAINT& aConstraint,
                          DRC_RULE* aRule,
                          const std::map<wxString, CONNECTION>& aChainAgg );

    // Lazily-built shared topology view of a named chain, built from every
    // item on the board that carries that chain.  Cached for the duration of
    // a single DRC pass so two rules constraining the same chain don't
    // rebuild the graph (the topology is a property of the routed copper,
    // not of which rule matched).
    std::shared_ptr<CHAIN_TOPOLOGY> chainTopologyFor( const wxString& aChain );

    // Cached zone-coverage union per (reference layer, net pattern) tuple
    // for the duration of a DRC pass — return-path checks can reuse it
    // across rules.
    SHAPE_POLY_SET* zoneUnionFor( PCB_LAYER_ID aRefLayer, const wxString& aRefNet );

private:
    BOARD*                                                  m_board;
    DRC_LENGTH_REPORT                                       m_report;
    std::map<wxString, std::shared_ptr<CHAIN_TOPOLOGY>>     m_chainTopoCache;
    std::map<std::pair<PCB_LAYER_ID, wxString>, SHAPE_POLY_SET> m_refUnionCache;
};

void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkLengths( const DRC_CONSTRAINT& aConstraint,
                                                     const std::vector<CONNECTION>& aMatchedConnections )
{
    for( const DRC_LENGTH_REPORT::ENTRY& ent : aMatchedConnections )
    {
        bool minViolation = false;
        bool maxViolation = false;
        int  minLen = 0;
        int  maxLen = 0;

        const bool          isTimeDomain = aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
        const EDA_DATA_TYPE dataType = isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

        if( !isTimeDomain )
        {
            if( aConstraint.GetValue().HasMin() && ent.total < aConstraint.GetValue().Min() )
            {
                minViolation = true;
                minLen = aConstraint.GetValue().Min();
            }
            else if( aConstraint.GetValue().HasMax() && ent.total > aConstraint.GetValue().Max() )
            {
                maxViolation = true;
                maxLen = aConstraint.GetValue().Max();
            }
        }
        else
        {
            if( aConstraint.GetValue().HasMin() && ent.totalDelay < aConstraint.GetValue().Min() )
            {
                minViolation = true;
                minLen = aConstraint.GetValue().Min();
            }
            else if( aConstraint.GetValue().HasMax() && ent.totalDelay > aConstraint.GetValue().Max() )
            {
                maxViolation = true;
                maxLen = aConstraint.GetValue().Max();
            }
        }

        if( ( minViolation || maxViolation ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LENGTH_OUT_OF_RANGE );

            if( minViolation )
            {
                drcItem->SetErrorDetail( formatMsg( _( "(%s min length %s; actual %s)" ),
                                                    aConstraint.GetName(),
                                                    minLen,
                                                    aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN )
                                                            ? ent.totalDelay
                                                            : ent.total,
                                                    dataType ) );
            }
            else if( maxViolation )
            {
                drcItem->SetErrorDetail( formatMsg( _( "(%s max length %s; actual %s)" ),
                                                    aConstraint.GetName(),
                                                    maxLen,
                                                    aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN )
                                                            ? ent.totalDelay
                                                            : ent.total,
                                                    dataType ) );
            }

            for( auto offendingTrack : ent.items )
                drcItem->AddItem( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, ( *ent.items.begin() )->GetPosition(), ( *ent.items.begin() )->GetLayer() );
        }
    }
}

void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkSkews( const DRC_CONSTRAINT& aConstraint,
                                                   const std::vector<CONNECTION>& aMatchedConnections )
{
    auto checkSkewsImpl = [this, &aConstraint]( const std::vector<CONNECTION>& connections )
    {
        const bool          isTimeDomain = aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
        const EDA_DATA_TYPE dataType = isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;

        double   maxLength = 0;
        wxString maxNetname;

        if( !isTimeDomain )
        {
            for( const DRC_LENGTH_REPORT::ENTRY& ent : connections )
            {
                if( ent.total > maxLength )
                {
                    maxLength = ent.total;
                    maxNetname = ent.netname;
                }
            }
        }
        else
        {
            for( const DRC_LENGTH_REPORT::ENTRY& ent : connections )
            {
                if( ent.totalDelay > maxLength )
                {
                    maxLength = ent.totalDelay;
                    maxNetname = ent.netname;
                }
            }
        }

        for( const DRC_LENGTH_REPORT::ENTRY& ent : connections )
        {
            int skew = isTimeDomain ? KiROUND( ent.totalDelay - maxLength )
                                    : KiROUND( ent.total - maxLength );

            bool fail_min = false;
            bool fail_max = false;

            if( aConstraint.GetValue().HasMax() && abs( skew ) > aConstraint.GetValue().Max() )
                fail_max = true;
            else if( aConstraint.GetValue().HasMin() && abs( skew ) < aConstraint.GetValue().Min() )
                fail_min = true;

            if( fail_min || fail_max )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SKEW_OUT_OF_RANGE );
                wxString                  msg;

                double reportTotal = isTimeDomain ? ent.totalDelay : ent.total;

                if( fail_min )
                {
                    msg.Printf( _( "(%s min skew %s; actual %s; target net length %s (from %s); actual %s)" ),
                                aConstraint.GetName(),
                                MessageTextFromValue( aConstraint.GetValue().Min(), true, dataType ),
                                MessageTextFromValue( skew, true, dataType ),
                                MessageTextFromValue( maxLength, true, dataType ),
                                maxNetname,
                                MessageTextFromValue( reportTotal, true, dataType ) );
                }
                else
                {
                    msg.Printf( _( "(%s max skew %s; actual %s; target net length %s (from %s); actual %s)" ),
                                aConstraint.GetName(),
                                MessageTextFromValue( aConstraint.GetValue().Max(), true, dataType ),
                                MessageTextFromValue( skew, true, dataType ),
                                MessageTextFromValue( maxLength, true, dataType ),
                                maxNetname,
                                MessageTextFromValue( reportTotal, true, dataType ) );
                }

                drcItem->SetErrorDetail( msg );

                for( BOARD_CONNECTED_ITEM* offendingTrack : ent.items )
                    drcItem->SetItems( offendingTrack );

                drcItem->SetViolatingRule( aConstraint.GetParentRule() );

                reportViolation( drcItem, ( *ent.items.begin() )->GetPosition(), ( *ent.items.begin() )->GetLayer() );
            }
        }
    };

    if( aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::SKEW_WITHIN_DIFF_PAIRS ) )
    {
        // Find all pairs of nets in the matched connections
        std::map<int, CONNECTION> netcodeMap;

        for( const DRC_LENGTH_REPORT::ENTRY& ent : aMatchedConnections )
            netcodeMap[ent.netcode] = ent;

        std::vector<std::vector<CONNECTION>> matchedDiffPairs;

        for( auto& [netcode, connection] : netcodeMap )
        {
            NETINFO_ITEM* matchedNet = m_board->DpCoupledNet( connection.netinfo );

            if( matchedNet )
            {
                int matchedNetcode = matchedNet->GetNetCode();

                if( netcodeMap.count( matchedNetcode ) )
                {
                    std::vector<CONNECTION> pair{ connection, netcodeMap[matchedNetcode] };
                    matchedDiffPairs.emplace_back( std::move( pair ) );
                    netcodeMap.erase( matchedNetcode );
                }
            }
        }

        // Test all found pairs of nets
        for( const std::vector<CONNECTION>& matchedDiffPair : matchedDiffPairs )
            checkSkewsImpl( matchedDiffPair );
    }
    else
    {
        // Test all matched nets as a group
        checkSkewsImpl( aMatchedConnections );
    }
}


void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkViaCounts( const DRC_CONSTRAINT& aConstraint,
                                                       const std::vector<CONNECTION>& aMatchedConnections )
{
    for( const auto& ent : aMatchedConnections )
    {
        std::shared_ptr<DRC_ITEM> drcItem = nullptr;

        if( aConstraint.GetValue().HasMax() && ent.viaCount > aConstraint.GetValue().Max() )
        {
            drcItem = DRC_ITEM::Create( DRCE_VIA_COUNT_OUT_OF_RANGE );
            wxString msg = wxString::Format( _( "(%s max count %d; actual %d)" ),
                                             aConstraint.GetName(),
                                             aConstraint.GetValue().Max(),
                                             ent.viaCount );

            drcItem->SetErrorMessage( _( "Too many vias on a connection" ) + wxS( " " ) + msg );
        }
        else if( aConstraint.GetValue().HasMin() && ent.viaCount < aConstraint.GetValue().Min() )
        {
            drcItem = DRC_ITEM::Create( DRCE_VIA_COUNT_OUT_OF_RANGE );
            wxString msg = wxString::Format( _( "(%s min count %d; actual %d)" ),
                                             aConstraint.GetName(),
                                             aConstraint.GetValue().Min(),
                                             ent.viaCount );

            drcItem->SetErrorMessage( _( "Too few vias on a connection" ) + wxS( " " ) + msg );
        }

        if( drcItem )
        {
            for( const BOARD_CONNECTED_ITEM* offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, ( *ent.items.begin() )->GetPosition(), ( *ent.items.begin() )->GetLayer() );
        }
    }
}


bool DRC_TEST_PROVIDER_MATCHED_LENGTH::Run()
{
    return runInternal( false );
}


bool DRC_TEST_PROVIDER_MATCHED_LENGTH::runInternal( bool aDelayReportMode )
{
    m_board = m_drcEngine->GetBoard();
    m_report.Clear();
    m_chainTopoCache.clear();
    m_refUnionCache.clear();

    if( !aDelayReportMode )
    {
        if( !reportPhase( _( "Gathering length-constrained connections..." ) ) )
            return false;
    }

    LSET boardCopperLayers = LSET::AllCuMask( m_board->GetCopperLayerCount() );
    std::map<DRC_RULE*, std::set<BOARD_CONNECTED_ITEM*> > itemSets;

    std::shared_ptr<FROM_TO_CACHE> ftCache = m_board->GetConnectivity()->GetFromToCache();

    ftCache->Rebuild( m_board );

    const size_t progressDelta = 100;
    size_t       count = 0;
    size_t       ii = 0;

    forEachGeometryItem( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T }, boardCopperLayers,
            [&]( BOARD_ITEM *item ) -> bool
            {
                count++;
                return true;
            } );

    forEachGeometryItem( { PCB_TRACE_T, PCB_ARC_T, PCB_VIA_T, PCB_PAD_T }, boardCopperLayers,
            [&]( BOARD_ITEM *item ) -> bool
            {
                if( !reportProgress( ii++, count, progressDelta ) )
                    return false;

                for( DRC_CONSTRAINT_T jj : { LENGTH_CONSTRAINT, NET_CHAIN_LENGTH_CONSTRAINT,
                                             NET_CHAIN_STUB_LENGTH_CONSTRAINT,
                                             NET_CHAIN_RETURN_PATH_CONSTRAINT,
                                             SKEW_CONSTRAINT, VIA_COUNT_CONSTRAINT } )
                {
                    DRC_CONSTRAINT constraint = m_drcEngine->EvalRules( jj, item, nullptr, item->GetLayer() );

                    if( constraint.IsNull() )
                        continue;

                    BOARD_CONNECTED_ITEM* citem = static_cast<BOARD_CONNECTED_ITEM*>( item );

                    itemSets[ constraint.GetParentRule() ].insert( citem );
                }

                return true;
            } );

    LENGTH_DELAY_CALCULATION* calc = m_board->GetLengthCalculation();

    std::map< DRC_RULE*, std::vector<CONNECTION> > matches;

    for( const auto& [rule, ruleItems] : itemSets )
    {
        std::map<int, std::set<BOARD_CONNECTED_ITEM*> > netMap;

        for( BOARD_CONNECTED_ITEM* item : ruleItems )
            netMap[item->GetNetCode()].insert( item );

        for( const auto& [netCode, netItems] : netMap )
        {
            std::vector<LENGTH_DELAY_CALCULATION_ITEM> lengthItems;
            lengthItems.reserve( netItems.size() );

            CONNECTION ent;
            ent.items = netItems;
            ent.netcode = netCode;
            ent.netname = m_board->GetNetInfo().GetNetItem( ent.netcode )->GetNetname();
            ent.netinfo = m_board->GetNetInfo().GetNetItem( ent.netcode );

            ent.viaCount = 0;
            ent.totalRoute = 0;
            ent.totalVia = 0;
            ent.totalPadToDie = 0;
            ent.fromItem = nullptr;
            ent.toItem = nullptr;

            for( BOARD_CONNECTED_ITEM* item : netItems )
            {
                LENGTH_DELAY_CALCULATION_ITEM lengthItem = calc->GetLengthCalculationItem( item );

                if( lengthItem.Type() != LENGTH_DELAY_CALCULATION_ITEM::TYPE::UNKNOWN )
                    lengthItems.emplace_back( lengthItem );
            }

            constexpr PATH_OPTIMISATIONS opts = {
                .OptimiseVias = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
            };
            LENGTH_DELAY_STATS details = calc->CalculateLengthDetails( lengthItems, opts, nullptr, nullptr,
                                                                       LENGTH_DELAY_LAYER_OPT::NO_LAYER_DETAIL,
                                                                       LENGTH_DELAY_DOMAIN_OPT::WITH_DELAY_DETAIL );
            ent.viaCount = details.NumVias;
            ent.totalVia = details.ViaLength;
            ent.totalViaDelay = details.ViaDelay;
            ent.totalRoute = static_cast<double>( details.TrackLength );
            ent.totalRouteDelay = static_cast<double>( details.TrackDelay );
            ent.totalPadToDie = details.PadToDieLength;
            ent.totalPadToDieDelay = details.PadToDieDelay;
            ent.total = ent.totalRoute + ent.totalVia + ent.totalPadToDie;
            ent.totalDelay = ent.totalRouteDelay + static_cast<double>( ent.totalViaDelay )
                             + static_cast<double>( ent.totalPadToDieDelay );
            ent.matchingRule = rule;

            if( FROM_TO_CACHE::FT_PATH* ftPath = ftCache->QueryFromToPath( ent.items ) )
            {
                ent.from = ftPath->fromName;
                ent.to = ftPath->toName;
            }
            else
            {
                ent.from = ent.to = _( "<unconstrained>" );
            }

            m_report.Add( ent );
            matches[rule].push_back( ent );
        }
    }

    if( !aDelayReportMode )
    {
        if( !reportPhase( _( "Checking length constraints..." ) ) )
            return false;

        ii = 0;
        count = matches.size();

        for( std::pair< DRC_RULE* const, std::vector<CONNECTION> > it : matches )
        {
            DRC_RULE *rule = it.first;
            auto& matchedConnections = it.second;

            if( !reportProgress( ii++, count, progressDelta ) )
                return false;

            std::sort( matchedConnections.begin(), matchedConnections.end(),
                       [] ( const CONNECTION&a, const CONNECTION&b ) -> int
                       {
                           return a.netname < b.netname;
                       } );

            if( getLogReporter() )
            {
                REPORT_AUX( wxString::Format( wxT( "Length-constrained traces for rule '%s':" ),
                                              it.first->m_Name ) );

                for( const DRC_LENGTH_REPORT::ENTRY& ent : matchedConnections )
                {
                    REPORT_AUX( wxString::Format( wxT( " - net: %s, from: %s, to: %s, %d matching items, "
                                                       "total: %s (tracks: %s, vias: %s, pad-to-die: %s), "
                                                       "vias: %d" ),
                                                  ent.netname,
                                                  ent.from, ent.to,
                                                  static_cast<int>( ent.items.size() ),
                                                  MessageTextFromValue( ent.total ),
                                                  MessageTextFromValue( ent.totalRoute ),
                                                  MessageTextFromValue( ent.totalVia ),
                                                  MessageTextFromValue( ent.totalPadToDie ),
                                                  ent.viaCount ) );
                }
            }

            // Check both net-level and net-chain-level length constraints
            std::optional<DRC_CONSTRAINT> lengthConstraint = rule->FindConstraint( LENGTH_CONSTRAINT );
            std::optional<DRC_CONSTRAINT> netChainLengthConstraint = rule->FindConstraint( NET_CHAIN_LENGTH_CONSTRAINT );

            // Build per-chain aggregates (kept for the length report).  The
            // shared CHAIN_TOPOLOGY is fetched lazily from the provider's
            // cache so two rules constraining the same chain don't rebuild
            // the graph.
            std::map<wxString, CONNECTION>      chainAgg;
            std::optional<DRC_CONSTRAINT>       stubConstraint =
                    rule->FindConstraint( NET_CHAIN_STUB_LENGTH_CONSTRAINT );
            std::optional<DRC_CONSTRAINT>       returnPathConstraint =
                    rule->FindConstraint( NET_CHAIN_RETURN_PATH_CONSTRAINT );

            const bool needsChainAgg =
                    ( netChainLengthConstraint
                      && netChainLengthConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                 || ( stubConstraint && stubConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                 || ( returnPathConstraint
                      && returnPathConstraint->GetSeverity() != RPT_SEVERITY_IGNORE );

            if( needsChainAgg )
            {
                for( const CONNECTION& conn : matchedConnections )
                {
                    if( !conn.netinfo )
                        continue;

                    wxString chainName = conn.netinfo->GetNetChain();

                    if( chainName.IsEmpty() )
                        continue;

                    auto aggIt = chainAgg.find( chainName );

                    if( aggIt == chainAgg.end() )
                    {
                        CONNECTION agg = conn;
                        agg.netname = chainName;
                        chainAgg[chainName] = agg;
                    }
                    else
                    {
                        CONNECTION& agg = aggIt->second;
                        agg.total += conn.total;
                        agg.totalDelay += conn.totalDelay;
                        agg.totalRoute += conn.totalRoute;
                        agg.totalRouteDelay += conn.totalRouteDelay;
                        agg.totalVia += conn.totalVia;
                        agg.totalViaDelay += conn.totalViaDelay;
                        agg.totalPadToDie += conn.totalPadToDie;
                        agg.totalPadToDieDelay += conn.totalPadToDieDelay;
                        agg.viaCount += conn.viaCount;
                        agg.items.insert( conn.items.begin(), conn.items.end() );
                    }
                }
            }

            if( netChainLengthConstraint && netChainLengthConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
            {
                std::vector<CONNECTION> chainConnections;
                chainConnections.reserve( chainAgg.size() );

                for( auto& [chainName, agg] : chainAgg )
                {
                    CONNECTION constraintInput = agg;
                    std::shared_ptr<CHAIN_TOPOLOGY> topo = chainTopologyFor( chainName );

                    if( topo && topo->IsValid() )
                    {
                        // Trunk semantics: constrain the matched-length signal
                        // path between terminal pads, not the sum of all member
                        // segments.  Bridging is folded into the topology graph.
                        constraintInput.total = topo->TrunkLength();
                        constraintInput.totalDelay = topo->TrunkDelay();
                        constraintInput.totalRoute = topo->TrunkLength();
                        constraintInput.totalRouteDelay = topo->TrunkDelay();
                    }
                    else
                    {
                        // Aggregate fallback (legacy) — preserves behavior for
                        // chains without terminal pads or with looped routing.
                        // Add bridging length/delay through series passives so
                        // the time-domain rule sees a compensated chain.
                        auto [bridging, bridgingDelay] = BoardChainBridging( m_board, chainName );

                        constraintInput.total += bridging;
                        constraintInput.totalRoute += bridging;
                        constraintInput.totalDelay += bridgingDelay;
                        constraintInput.totalRouteDelay += bridgingDelay;
                    }

                    chainConnections.push_back( std::move( constraintInput ) );
                }

                checkLengths( *netChainLengthConstraint, chainConnections );
            }

            if( lengthConstraint && lengthConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
            {
                checkLengths( *lengthConstraint, matchedConnections );
            }

            std::optional<DRC_CONSTRAINT> skewConstraint = rule->FindConstraint( SKEW_CONSTRAINT );

            if( skewConstraint && skewConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkSkews( *skewConstraint, matchedConnections );

            std::optional<DRC_CONSTRAINT> viaCountConstraint = rule->FindConstraint( VIA_COUNT_CONSTRAINT );

            if( viaCountConstraint && viaCountConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkViaCounts( *viaCountConstraint, matchedConnections );

            if( returnPathConstraint
                && returnPathConstraint->GetSeverity() != RPT_SEVERITY_IGNORE
                && !returnPathConstraint->m_ReferenceLayer.IsEmpty() )
            {
                checkReturnPath( *returnPathConstraint, rule, chainAgg );
            }

            if( stubConstraint && stubConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkStubLengths( *stubConstraint, rule, matchedConnections );
        }
    }

    return !m_drcEngine->IsCancelled();
}


std::shared_ptr<CHAIN_TOPOLOGY>
DRC_TEST_PROVIDER_MATCHED_LENGTH::chainTopologyFor( const wxString& aChain )
{
    auto it = m_chainTopoCache.find( aChain );

    if( it != m_chainTopoCache.end() )
        return it->second;

    std::set<BOARD_CONNECTED_ITEM*> items;

    for( PCB_TRACK* t : m_board->Tracks() )
    {
        if( t->GetNet() && t->GetNet()->GetNetChain() == aChain )
            items.insert( t );
    }

    for( FOOTPRINT* fp : m_board->Footprints() )
    {
        for( PAD* p : fp->Pads() )
        {
            if( p->GetNet() && p->GetNet()->GetNetChain() == aChain )
                items.insert( p );
        }
    }

    auto topo = std::make_shared<CHAIN_TOPOLOGY>( m_board, aChain, items );
    m_chainTopoCache.emplace( aChain, topo );
    return topo;
}


SHAPE_POLY_SET*
DRC_TEST_PROVIDER_MATCHED_LENGTH::zoneUnionFor( PCB_LAYER_ID aRefLayer,
                                                const wxString& aRefNet )
{
    auto key = std::make_pair( aRefLayer, aRefNet );
    auto it = m_refUnionCache.find( key );

    if( it != m_refUnionCache.end() )
        return &it->second;

    SHAPE_POLY_SET refUnion;

    for( ZONE* zone : m_board->Zones() )
    {
        if( !zone || !zone->IsOnLayer( aRefLayer ) || zone->GetIsRuleArea() )
            continue;

        if( !aRefNet.IsEmpty() )
        {
            wxString zoneNet = zone->GetNetname();

            if( !WildCompareString( aRefNet, zoneNet, false ) )
                continue;
        }

        // Prefer the cached filled-fill polys (respect holes / islands).
        // Fall back to the zone outline for synthetic / unfilled boards
        // (test fixtures, freshly imported designs).  GetFill returns
        // nullptr when the zone hasn't been filled for this layer; using
        // GetFilledPolysList here would assert.
        if( SHAPE_POLY_SET* filled = zone->GetFill( aRefLayer );
            filled && filled->OutlineCount() > 0 )
        {
            refUnion.BooleanAdd( *filled );
        }
        else if( zone->Outline() )
        {
            refUnion.BooleanAdd( *zone->Outline() );
        }
    }

    refUnion.Simplify();
    return &m_refUnionCache.emplace( key, std::move( refUnion ) ).first->second;
}


void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkStubLengths(
        const DRC_CONSTRAINT& aConstraint, DRC_RULE* aRule,
        const std::vector<CONNECTION>& aMatchedConnections )
{
    const bool isTimeDomain =
            aConstraint.GetOption( DRC_CONSTRAINT::OPTIONS::TIME_DOMAIN );
    const EDA_DATA_TYPE dataType =
            isTimeDomain ? EDA_DATA_TYPE::TIME : EDA_DATA_TYPE::DISTANCE;
    const MINOPTMAX<int>& range = aConstraint.GetValue();

    if( !range.HasMax() && !range.HasMin() )
        return;

    auto outOfRange = [&]( double aMeasured )
    {
        return ( range.HasMax() && aMeasured > range.Max() )
            || ( range.HasMin() && aMeasured < range.Min() );
    };

    // Collect the distinct chains touched by the rule's matched items, then
    // dispatch each chain through the topology if it's valid, falling back
    // to the legacy proxy otherwise.
    std::set<wxString> chainNames;

    for( const CONNECTION& conn : aMatchedConnections )
    {
        if( conn.netinfo && !conn.netinfo->GetNetChain().IsEmpty() )
            chainNames.insert( conn.netinfo->GetNetChain() );
    }

    std::set<wxString> handledByTopology;

    for( const wxString& chainName : chainNames )
    {
        std::shared_ptr<CHAIN_TOPOLOGY> topoPtr = chainTopologyFor( chainName );

        if( !topoPtr || !topoPtr->IsValid() )
            continue;

        handledByTopology.insert( chainName );

        for( const CHAIN_TOPOLOGY::STUB& stub : topoPtr->Stubs() )
        {
            const double measured = isTimeDomain ? stub.delay : stub.length;

            if( !outOfRange( measured ) )
                continue;

            std::shared_ptr<DRC_ITEM> item =
                    DRC_ITEM::Create( DRCE_NET_CHAIN_STUB_TOO_LONG );
            item->SetErrorMessage( wxString::Format(
                    _( "Stub length (%s) out of range for net chain '%s'." ),
                    MessageTextFromValue( measured, true, dataType ),
                    chainName ) );
            item->SetViolatingRule( aRule );

            for( BOARD_CONNECTED_ITEM* it : stub.items )
                item->AddItem( it );

            reportViolation( item, stub.branchPoint, stub.branchLayer );
        }
    }

    // Legacy proxy fallback for chains where the topology builder reported an
    // invalid status (no terminal pads, disconnected, or cycle).  This
    // preserves behavior on rule sets that pre-date the terminal-pad model.
    for( const CONNECTION& conn : aMatchedConnections )
    {
        NETINFO_ITEM* netInfo = m_board->GetNetInfo().GetNetItem( conn.netcode );

        if( !netInfo || netInfo->GetNetChain().IsEmpty() )
            continue;

        if( handledByTopology.count( netInfo->GetNetChain() ) )
            continue;

        const bool onTrunk =
                ( netInfo->GetTerminalPad( 0 )
                  && netInfo->GetTerminalPad( 0 )->GetNetCode() == conn.netcode )
             || ( netInfo->GetTerminalPad( 1 )
                  && netInfo->GetTerminalPad( 1 )->GetNetCode() == conn.netcode );

        if( onTrunk )
            continue;

        const double measured = isTimeDomain ? conn.totalDelay
                                             : static_cast<double>( conn.total );

        if( !outOfRange( measured ) )
            continue;

        std::shared_ptr<DRC_ITEM> item =
                DRC_ITEM::Create( DRCE_NET_CHAIN_STUB_TOO_LONG );
        item->SetErrorMessage( wxString::Format(
                _( "Stub length (%s) out of range for net chain '%s' on net '%s'." ),
                MessageTextFromValue( measured, true, dataType ),
                netInfo->GetNetChain(),
                netInfo->GetNetname() ) );
        item->SetViolatingRule( aRule );

        for( BOARD_CONNECTED_ITEM* connItem : conn.items )
            item->AddItem( connItem );

        VECTOR2I     pos;
        PCB_LAYER_ID layer = UNDEFINED_LAYER;

        if( !conn.items.empty() )
        {
            pos = ( *conn.items.begin() )->GetPosition();
            layer = ( *conn.items.begin() )->GetLayer();
        }

        reportViolation( item, pos, layer );
    }
}


void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkReturnPath(
        const DRC_CONSTRAINT& aConstraint, DRC_RULE* aRule,
        const std::map<wxString, CONNECTION>& aChainAgg )
{
    const wxString& refLayerName = aConstraint.m_ReferenceLayer;
    const wxString& refNetPattern = aConstraint.m_ReferenceNet;

    PCB_LAYER_ID refLayer = m_board->GetLayerID( refLayerName );

    if( refLayer == UNDEFINED_LAYER )
        return;

    SHAPE_POLY_SET&  refUnion = *zoneUnionFor( refLayer, refNetPattern );
    const BOX2I      refBbox = refUnion.OutlineCount() ? refUnion.BBox() : BOX2I();

    struct FlaggedRegion
    {
        SHAPE_POLY_SET                     poly;
        std::vector<BOARD_CONNECTED_ITEM*> items;
        PCB_LAYER_ID                       layer = UNDEFINED_LAYER;
    };

    auto flagItem = [&]( BOARD_CONNECTED_ITEM* aItem,
                         std::vector<FlaggedRegion>& aFlagged,
                         SHAPE_POLY_SET&& aRegion )
    {
        for( int o = 0; o < aRegion.OutlineCount(); ++o )
        {
            FlaggedRegion fr;
            fr.poly.AddOutline( aRegion.Outline( o ) );

            for( int h = 0; h < aRegion.HoleCount( o ); ++h )
                fr.poly.AddHole( aRegion.Hole( o, h ) );

            fr.items.push_back( aItem );
            fr.layer = aItem->GetLayer();
            aFlagged.push_back( std::move( fr ) );
        }
    };

    for( const auto& [chainName, agg] : aChainAgg )
    {
        std::vector<FlaggedRegion> flagged;

        for( BOARD_CONNECTED_ITEM* item : agg.items )
        {
            if( !item )
                continue;

            if( item->Type() != PCB_TRACE_T && item->Type() != PCB_ARC_T )
                continue;

            BOX2I itemBbox = item->GetBoundingBox();

            if( refUnion.OutlineCount() == 0 || !itemBbox.Intersects( refBbox ) )
            {
                // Nothing on the reference layer covers this item.
                SHAPE_POLY_SET itemPoly;
                item->TransformShapeToPolygon( itemPoly, item->GetLayer(),
                                               0, ARC_HIGH_DEF, ERROR_INSIDE );
                flagItem( item, flagged, std::move( itemPoly ) );
                continue;
            }

            SHAPE_POLY_SET itemPoly;
            item->TransformShapeToPolygon( itemPoly, item->GetLayer(),
                                           0, ARC_HIGH_DEF, ERROR_INSIDE );

            SHAPE_POLY_SET diff = itemPoly;
            diff.BooleanSubtract( refUnion );
            diff.Simplify();

            if( diff.OutlineCount() == 0 )
                continue;

            flagItem( item, flagged, std::move( diff ) );
        }

        if( flagged.empty() )
            continue;

        // Coalesce flagged regions by spatial adjacency: two regions belong to
        // the same group if their bounding boxes intersect (within epsilon).
        // Provider-local union-find over the flagged set only — does NOT use
        // CONNECTIVITY_DATA, which would merge across covered tracks.
        std::vector<int> parent( flagged.size() );
        std::iota( parent.begin(), parent.end(), 0 );

        auto find = [&]( int x )
        {
            while( parent[x] != x )
            {
                parent[x] = parent[parent[x]];
                x = parent[x];
            }

            return x;
        };

        for( size_t i = 0; i < flagged.size(); ++i )
        {
            BOX2I bi = flagged[i].poly.BBox();
            bi.Inflate( 1 );

            for( size_t j = i + 1; j < flagged.size(); ++j )
            {
                BOX2I bj = flagged[j].poly.BBox();

                // Cheap reject: if the inflated bboxes don't even touch,
                // the polygons cannot be adjacent.
                if( !bi.Intersects( bj ) )
                    continue;

                // Confirm polygon-level adjacency: two flagged regions
                // belong to the same group only if their copper polygons
                // actually intersect (after a 1 IU inflate to bridge
                // touching boundaries).  Bbox-touch alone can merge
                // unrelated runs whose AABBs overlap but copper does not.
                SHAPE_POLY_SET probe = flagged[i].poly;
                probe.Inflate( 1, CORNER_STRATEGY::ROUND_ALL_CORNERS, ARC_HIGH_DEF );
                probe.BooleanIntersection( flagged[j].poly );
                probe.Simplify();

                if( probe.OutlineCount() == 0 )
                    continue;

                int ri = find( static_cast<int>( i ) );
                int rj = find( static_cast<int>( j ) );

                if( ri != rj )
                    parent[ri] = rj;
            }
        }

        std::map<int, std::vector<int>> groups;

        for( size_t i = 0; i < flagged.size(); ++i )
            groups[find( static_cast<int>( i ) )].push_back( static_cast<int>( i ) );

        for( const auto& [root, members] : groups )
        {
            SHAPE_POLY_SET                          unionPoly;
            std::vector<BOARD_CONNECTED_ITEM*>      items;
            std::set<BOARD_CONNECTED_ITEM*>         seen;
            PCB_LAYER_ID                            markerLayer = UNDEFINED_LAYER;

            for( int idx : members )
            {
                unionPoly.BooleanAdd( flagged[idx].poly );

                for( BOARD_CONNECTED_ITEM* it : flagged[idx].items )
                {
                    if( seen.insert( it ).second )
                        items.push_back( it );
                }

                if( markerLayer == UNDEFINED_LAYER )
                    markerLayer = flagged[idx].layer;
            }

            unionPoly.Simplify();

            VECTOR2I markerPos = unionPoly.OutlineCount()
                                         ? unionPoly.BBox().Centre()
                                         : ( !items.empty()
                                                   ? items.front()->GetPosition()
                                                   : VECTOR2I() );

            std::shared_ptr<DRC_ITEM> drcItem =
                    DRC_ITEM::Create( DRCE_NET_CHAIN_RETURN_PATH_BREAK );

            wxString msg = wxString::Format(
                    _( "Net chain '%s' has no copper return path on reference layer '%s'." ),
                    chainName, refLayerName );

            if( !refNetPattern.IsEmpty() )
                msg += wxString::Format( _( " (net '%s')" ), refNetPattern );

            drcItem->SetErrorMessage( msg );
            drcItem->SetViolatingRule( aRule );

            for( BOARD_CONNECTED_ITEM* it : items )
                drcItem->AddItem( it );

            reportViolation( drcItem, markerPos, markerLayer );
        }
    }
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MATCHED_LENGTH> dummy;
}
