/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2004-2020 KiCad Developers.
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
#include <pad.h>
#include <pcb_track.h>

#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>
#include <drc/drc_length_report.h>

#include <connectivity/connectivity_data.h>
#include <connectivity/from_to_cache.h>

#include <pcb_expr_evaluator.h>

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
    {
    }

    virtual ~DRC_TEST_PROVIDER_MATCHED_LENGTH()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "length";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests matched track lengths.";
    }

    virtual int GetNumPhases() const override
    {
        return 1;
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    DRC_LENGTH_REPORT BuildLengthReport() const;

private:

    bool runInternal( bool aDelayReportMode = false );

    using CONNECTION = DRC_LENGTH_REPORT::ENTRY;

    void checkLengths( DRC_CONSTRAINT& aConstraint, std::vector<CONNECTION>& aMatchedConnections );
    void checkSkews( DRC_CONSTRAINT& aConstraint, std::vector<CONNECTION>& aMatchedConnections );
    void checkViaCounts( DRC_CONSTRAINT& aConstraint, std::vector<CONNECTION>& aMatchedConnections );

    BOARD* m_board;
    DRC_LENGTH_REPORT m_report;
};


static int computeViaThruLength( PCB_VIA *aVia, const std::set<BOARD_CONNECTED_ITEM*> &conns )
{
    return 0; // fixme: not yet there...
}


void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkLengths( DRC_CONSTRAINT& aConstraint,
                                                     std::vector<CONNECTION>& aMatchedConnections )
{
    for( const DRC_LENGTH_REPORT::ENTRY& ent : aMatchedConnections )
    {
        bool minViolation = false;
        bool maxViolation = false;
        int  minLen = 0;
        int  maxLen = 0;

        if( aConstraint.GetValue().HasMin() && ent.total < aConstraint.GetValue().Min() )
        {
            minViolation = true;
            minLen       = aConstraint.GetValue().Min();
        }
        else if( aConstraint.GetValue().HasMax() && ent.total > aConstraint.GetValue().Max() )
        {
            maxViolation = true;
            maxLen       = aConstraint.GetValue().Max();
        }

        if( ( minViolation || maxViolation ) )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_LENGTH_OUT_OF_RANGE );

            if( minViolation )
            {
                m_msg.Printf( _( "(%s min length: %s; actual: %s)" ),
                              aConstraint.GetName(),
                              MessageTextFromValue( userUnits(), minLen ),
                              MessageTextFromValue( userUnits(), ent.total ) );
            }
            else if( maxViolation )
            {
                m_msg.Printf( _( "(%s max length: %s; actual: %s)" ),
                              aConstraint.GetName(),
                              MessageTextFromValue( userUnits(), maxLen ),
                              MessageTextFromValue( userUnits(), ent.total ) );
            }

            drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );

            for( auto offendingTrack : ent.items )
                drcItem->AddItem( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
        }
    }
}

void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkSkews( DRC_CONSTRAINT& aConstraint,
                                                   std::vector<CONNECTION>& aMatchedConnections )
{
    int avgLength = 0;

    for( const DRC_LENGTH_REPORT::ENTRY& ent : aMatchedConnections )
        avgLength += ent.total;

    avgLength /= aMatchedConnections.size();

    for( const auto& ent : aMatchedConnections )
    {
        int skew = ent.total - avgLength;
        if( aConstraint.GetValue().HasMax() && abs( skew ) > aConstraint.GetValue().Max() )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SKEW_OUT_OF_RANGE );

            m_msg.Printf( _( "(%s max skew: %s; actual: %s; average net length: %s; actual: %s)" ),
                          aConstraint.GetName(),
                          MessageTextFromValue( userUnits(), aConstraint.GetValue().Max() ),
                          MessageTextFromValue( userUnits(), skew ),
                          MessageTextFromValue( userUnits(), avgLength ),
                          MessageTextFromValue( userUnits(), ent.total ) );

            drcItem->SetErrorMessage( drcItem->GetErrorText() + " " + m_msg );

            for( BOARD_CONNECTED_ITEM* offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
        }
    }
}


void DRC_TEST_PROVIDER_MATCHED_LENGTH::checkViaCounts( DRC_CONSTRAINT& aConstraint,
                                                       std::vector<CONNECTION>& aMatchedConnections )
{
    for( const auto& ent : aMatchedConnections )
    {
        if( aConstraint.GetValue().HasMax() && ent.viaCount > aConstraint.GetValue().Max() )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TOO_MANY_VIAS );

            m_msg.Printf( _( "(%s max count: %d; actual: %d)" ),
                          aConstraint.GetName(),
                          aConstraint.GetValue().Max(),
                          ent.viaCount );

            drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );

            for( auto offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
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

    std::map<DRC_RULE*, std::set<BOARD_CONNECTED_ITEM*> > itemSets;

    auto evaluateLengthConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                const DRC_CONSTRAINT_T constraintsToCheck[] = {
                        LENGTH_CONSTRAINT,
                        SKEW_CONSTRAINT,
                        VIA_COUNT_CONSTRAINT,
                };

                for( int i = 0; i < 3; i++ )
                {
                    auto constraint = m_drcEngine->EvalRules( constraintsToCheck[i], item, nullptr,
                                                              item->GetLayer() );

                    if( constraint.IsNull() )
                        continue;

                    auto citem = static_cast<BOARD_CONNECTED_ITEM*>( item );

                    itemSets[ constraint.GetParentRule() ].insert( citem );
                }

                return true;
            };

    auto ftCache = m_board->GetConnectivity()->GetFromToCache();

    ftCache->Rebuild( m_board );

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T }, LSET::AllCuMask(),
                         evaluateLengthConstraints );

    std::map<DRC_RULE*, std::vector<CONNECTION> > matches;

    for( auto it : itemSets )
    {
        std::map<int, std::set<BOARD_CONNECTED_ITEM*> > netMap;

        for( auto citem : it.second )
            netMap[ citem->GetNetCode() ].insert( citem );


        for( auto nitem : netMap )
        {
            CONNECTION ent;
            ent.items = nitem.second;
            ent.netcode = nitem.first;
            ent.netname = m_board->GetNetInfo().GetNetItem( ent.netcode )->GetNetname();

            ent.viaCount = 0;
            ent.totalRoute = 0;
            ent.totalVia = 0;
            ent.totalPadToDie = 0;
            ent.fromItem = nullptr;
            ent.toItem = nullptr;

            for( BOARD_CONNECTED_ITEM* citem : nitem.second )
            {
                if( citem->Type() == PCB_VIA_T )
                {
                    ent.viaCount++;
                    ent.totalVia += computeViaThruLength( static_cast<PCB_VIA*>( citem ),
                                                          nitem.second );
                }
                else if( citem->Type() == PCB_TRACE_T )
                {
                    ent.totalRoute += static_cast<PCB_TRACK*>( citem )->GetLength();
                }
                else if ( citem->Type() == PCB_ARC_T )
                {
                    ent.totalRoute += static_cast<PCB_ARC*>( citem )->GetLength();
                }
                else if( citem->Type() == PCB_PAD_T )
                {
                    ent.totalPadToDie += static_cast<PAD*>( citem )->GetPadToDieLength();
                }
            }

            ent.total = ent.totalRoute + ent.totalVia + ent.totalPadToDie;
            ent.matchingRule = it.first;

            // fixme: doesn't seem to work ;-)
            auto ftPath = ftCache->QueryFromToPath( ent.items );

            if( ftPath )
            {
                ent.from = ftPath->fromName;
                ent.to = ftPath->toName;
            }
            else
            {
                ent.from = ent.to = _("<unconstrained>");
            }

            m_report.Add( ent );
            matches[ it.first ].push_back(ent);
        }
    }

    if( !aDelayReportMode )
    {
        for( auto it : matches )
        {
            DRC_RULE *rule = it.first;
            auto& matchedConnections = it.second;

            std::sort( matchedConnections.begin(), matchedConnections.end(),
                       [] ( const CONNECTION&a, const CONNECTION&b ) -> int
                       {
                           return a.netname < b.netname;
                       } );

            reportAux( wxString::Format( "Length-constrained traces for rule '%s':",
                                         it.first->m_Name ) );

            for( auto& ent : matchedConnections )
            {
                reportAux(wxString::Format( " - net: %s, from: %s, to: %s, "
                                            "%d matching items, "
                                            "total: %s (tracks: %s, vias: %s, pad-to-die: %s), "
                                            "vias: %d",
                                            ent.netname,
                                            ent.from,
                                            ent.to,
                                            (int) ent.items.size(),
                                            MessageTextFromValue( userUnits(), ent.total ),
                                            MessageTextFromValue( userUnits(), ent.totalRoute ),
                                            MessageTextFromValue( userUnits(), ent.totalVia ),
                                            MessageTextFromValue( userUnits(), ent.totalPadToDie ),
                                            ent.viaCount ) );
            }


            OPT<DRC_CONSTRAINT> lengthConstraint = rule->FindConstraint( LENGTH_CONSTRAINT );

            if( lengthConstraint )
                checkLengths( *lengthConstraint, matchedConnections );

            OPT<DRC_CONSTRAINT> skewConstraint = rule->FindConstraint( SKEW_CONSTRAINT );

            if( skewConstraint )
                checkSkews( *skewConstraint, matchedConnections );

            OPT<DRC_CONSTRAINT> viaCountConstraint = rule->FindConstraint( VIA_COUNT_CONSTRAINT );

            if( viaCountConstraint )
                checkViaCounts( *viaCountConstraint, matchedConnections );
        }

        reportRuleStatistics();
    }

    return true;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_MATCHED_LENGTH::GetConstraintTypes() const
{
    return { LENGTH_CONSTRAINT, SKEW_CONSTRAINT, VIA_COUNT_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_MATCHED_LENGTH> dummy;
}
