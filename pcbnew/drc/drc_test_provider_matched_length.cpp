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
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_length_report.h>
#include <length_delay_calculation/length_delay_calculation.h>

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

private:
    BOARD*            m_board;
    DRC_LENGTH_REPORT m_report;
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

                for( DRC_CONSTRAINT_T jj : { LENGTH_CONSTRAINT, SKEW_CONSTRAINT, VIA_COUNT_CONSTRAINT } )
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
                .OptimiseViaLayers = true, .MergeTracks = true, .OptimiseTracesInPads = true, .InferViaInPad = false
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

            std::optional<DRC_CONSTRAINT> lengthConstraint = rule->FindConstraint( LENGTH_CONSTRAINT );

            if( lengthConstraint && lengthConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkLengths( *lengthConstraint, matchedConnections );

            std::optional<DRC_CONSTRAINT> skewConstraint = rule->FindConstraint( SKEW_CONSTRAINT );

            if( skewConstraint && skewConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkSkews( *skewConstraint, matchedConnections );

            std::optional<DRC_CONSTRAINT> viaCountConstraint = rule->FindConstraint( VIA_COUNT_CONSTRAINT );

            if( viaCountConstraint && viaCountConstraint->GetSeverity() != RPT_SEVERITY_IGNORE )
                checkViaCounts( *viaCountConstraint, matchedConnections );
        }
    }

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MATCHED_LENGTH> dummy;
}
