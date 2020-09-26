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
#include <class_board.h>
#include <class_drawsegment.h>
#include <class_pad.h>
#include <class_track.h>

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

namespace test {

class DRC_TEST_PROVIDER_MATCHED_LENGTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_MATCHED_LENGTH ()
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

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    DRC_LENGTH_REPORT BuildLengthReport() const;

private:

    bool runInternal( bool aDelayReportMode = false );

    using LENGTH_ENTRY = DRC_LENGTH_REPORT::ENTRY;
    typedef std::set<BOARD_CONNECTED_ITEM*> CITEMS;
    typedef std::vector<LENGTH_ENTRY> LENGTH_ENTRIES;

    void checkLengthViolations( DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& aMatchedConnections );
    void checkSkewViolations( DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& aMatchedConnections );
    void checkViaCountViolations( DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& aMatchedConnections );

    BOARD* m_board;
    DRC_LENGTH_REPORT m_report;
};

};


static int computeViaThruLength( VIA *aVia, const std::set<BOARD_CONNECTED_ITEM*> &conns )
{
    return 0; // fixme: not yet there...
}


void test::DRC_TEST_PROVIDER_MATCHED_LENGTH::checkLengthViolations(
        DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& matchedConnections )
{
    for( const auto& ent : matchedConnections )
    {
        bool minViolation = false;
        bool maxViolation = false;
        int  minLen, maxLen;

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
            wxString                  msg =
                    drcItem->GetErrorText() + " (" + aConstraint.GetParentRule()->m_Name + " ";

            if( minViolation )
            {
                msg += wxString::Format( _( "minimum length: %s; actual: %s)" ),
                        MessageTextFromValue( userUnits(), minLen, true ),
                        MessageTextFromValue( userUnits(), ent.total, true ) );
            }
            else if( maxViolation )
            {
                msg += wxString::Format( _( "maximum length: %s; actual: %s)" ),
                        MessageTextFromValue( userUnits(), maxLen, true ),
                        MessageTextFromValue( userUnits(), ent.total, true ) );
            }

            drcItem->SetErrorMessage( msg );

            for( auto offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
        }
    }
}

void test::DRC_TEST_PROVIDER_MATCHED_LENGTH::checkSkewViolations(
        DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& matchedConnections )
{
    int avgLength = 0;

    for( const auto& ent : matchedConnections )
    {
        avgLength += ent.total;
    }

    avgLength /= matchedConnections.size();

    for( const auto& ent : matchedConnections )
    {
        int skew = ent.total - avgLength;
        if( aConstraint.GetValue().HasMax() && abs( skew ) > aConstraint.GetValue().Max() )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_SKEW_OUT_OF_RANGE );
            wxString                  msg =
                    drcItem->GetErrorText() + " (" + aConstraint.GetParentRule()->m_Name + " ";

            msg += wxString::Format( _( "maximum skew: %s; actual skew: %s; average net length: %s; actual net length: %s)" ),
                        MessageTextFromValue( userUnits(), aConstraint.GetValue().Max(), true ),
                        MessageTextFromValue( userUnits(), skew, true ),
                        MessageTextFromValue( userUnits(), avgLength, true ),
                        MessageTextFromValue( userUnits(), ent.total, true )
                         );

            drcItem->SetErrorMessage( msg );

            for( auto offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
        }
    }
}


void test::DRC_TEST_PROVIDER_MATCHED_LENGTH::checkViaCountViolations(
        DRC_CONSTRAINT& aConstraint, LENGTH_ENTRIES& matchedConnections )
{
    for( const auto& ent : matchedConnections )
    {
        if( aConstraint.GetValue().HasMax() && ent.viaCount > aConstraint.GetValue().Max() )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TOO_MANY_VIAS );
            wxString                  msg =
                    drcItem->GetErrorText() + " (" + aConstraint.GetParentRule()->m_Name + " ";

            msg += wxString::Format( _( "max vias: %d; actual: %d" ),
                        aConstraint.GetValue().Max(), ent.viaCount );

            drcItem->SetErrorMessage( msg );

            for( auto offendingTrack : ent.items )
                drcItem->SetItems( offendingTrack );

            drcItem->SetViolatingRule( aConstraint.GetParentRule() );

            reportViolation( drcItem, (*ent.items.begin() )->GetPosition() );
        }
    }
}


bool test::DRC_TEST_PROVIDER_MATCHED_LENGTH::Run()
{
    return runInternal( false );
}


bool test::DRC_TEST_PROVIDER_MATCHED_LENGTH::runInternal( bool aDelayReportMode )
{
    m_board = m_drcEngine->GetBoard();
    m_report.Clear();

    if( !aDelayReportMode )
    {
        reportPhase(( "Gathering length-constrained connections..." ));
    }

    std::map<DRC_RULE*, CITEMS> itemSets;

     auto evaluateLengthConstraints =
            [&]( BOARD_ITEM *item ) -> bool
            {
                const DRC_CONSTRAINT_TYPE_T constraintsToCheck[] = {
                    DRC_CONSTRAINT_TYPE_LENGTH,
                    DRC_CONSTRAINT_TYPE_SKEW,
                    DRC_CONSTRAINT_TYPE_VIA_COUNT,
                };

                for( int i = 0; i < 3; i++ )
                {
                    auto constraint = m_drcEngine->EvalRulesForItems( constraintsToCheck[i], item );

                    if( constraint.IsNull() )
                        continue;

                    auto citem = static_cast<BOARD_CONNECTED_ITEM*>( item );

                    itemSets[ constraint.GetParentRule() ].insert( citem );
                }

                return true;
            };

    auto ftCache = m_board->GetConnectivity()->GetFromToCache();

    ftCache->Rebuild( m_board );

    forEachGeometryItem( { PCB_TRACE_T, PCB_VIA_T, PCB_ARC_T },
                    LSET::AllCuMask(), evaluateLengthConstraints );

    std::map<DRC_RULE*, LENGTH_ENTRIES> matches;

    for( auto it : itemSets )
    {
        std::map<int, CITEMS> netMap;

        for( auto citem : it.second )
            netMap[ citem->GetNetCode() ].insert( citem );


        for( auto nitem : netMap )
        {
            LENGTH_ENTRY ent;
            ent.items = nitem.second;
            ent.netcode = nitem.first;
            ent.netname = m_board->GetNetInfo().GetNetItem( ent.netcode )->GetNetname();

            ent.viaCount = 0;
            ent.totalRoute = 0;
            ent.totalVia = 0;
            ent.totalPadToDie = 0;
            ent.fromItem = nullptr;
            ent.toItem = nullptr;

            for( auto citem : nitem.second )
            {
                if ( auto bi = dyn_cast<VIA*>( citem ) )
                {
                    ent.viaCount++;
                    ent.totalVia += computeViaThruLength( bi, nitem.second ); // fixme: via thru distance
                }
                else if ( auto bi = dyn_cast<TRACK*>(citem ))
                {
                    ent.totalRoute += bi->GetLength();
                }
                else if ( auto bi = dyn_cast<D_PAD*>( citem ))
                {
                    ent.totalPadToDie += bi->GetPadToDieLength();
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
                [] ( const LENGTH_ENTRY&a, const LENGTH_ENTRY&b ) -> int
                {
                    return a.netname < b.netname;
                }
            );

            reportAux( wxString::Format( _("Length-constrained traces for rule '%s':"), it.first->m_Name ) );

            for( auto& ent : matchedConnections )
            {
                reportAux(wxString::Format(
                    " - net: %s, from: %s, to: %s, %d matching items, total: %s (tracks: %s, vias: %s, pad-to-die: %s), vias: %d",
                    ent.netname,
                    ent.from,
                    ent.to,
                    (int) ent.items.size(),
                    MessageTextFromValue( userUnits(), ent.total, true ),
                    MessageTextFromValue( userUnits(), ent.totalRoute, true ),
                    MessageTextFromValue( userUnits(), ent.totalVia, true ),
                    MessageTextFromValue( userUnits(), ent.totalPadToDie, true ),
                    ent.viaCount
                    ) );
            }


            OPT<DRC_CONSTRAINT> lengthConstraint = rule->FindConstraint( DRC_CONSTRAINT_TYPE_LENGTH );
            if( lengthConstraint )
            {
                checkLengthViolations( *lengthConstraint, matchedConnections );
            }

            OPT<DRC_CONSTRAINT> skewConstraint = rule->FindConstraint( DRC_CONSTRAINT_TYPE_SKEW );
            if( skewConstraint )
            {
                checkSkewViolations( *skewConstraint, matchedConnections );
            }

            OPT<DRC_CONSTRAINT> viaCountConstraint = rule->FindConstraint( DRC_CONSTRAINT_TYPE_VIA_COUNT );
            if( viaCountConstraint )
            {
                checkViaCountViolations( *viaCountConstraint, matchedConnections );
            }
        }

        reportRuleStatistics();
    }

    return true;
}


std::set<DRC_CONSTRAINT_TYPE_T> test::DRC_TEST_PROVIDER_MATCHED_LENGTH::GetConstraintTypes() const
{
    return { DRC_CONSTRAINT_TYPE_LENGTH, DRC_CONSTRAINT_TYPE_SKEW, DRC_CONSTRAINT_TYPE_VIA_COUNT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_MATCHED_LENGTH> dummy;
}