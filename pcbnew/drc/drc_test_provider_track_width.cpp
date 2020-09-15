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

//#include <common.h>
#include <class_track.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>


/*
    Track width test. As the name says, checks width of the tracks (including segments and arcs)
    Errors generated:
    - DRCE_TRACK_WIDTH
*/

class DRC_TEST_PROVIDER_TRACK_WIDTH : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_TRACK_WIDTH()
    {
    }

    virtual ~DRC_TEST_PROVIDER_TRACK_WIDTH()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "width";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests track widths";
    }

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_TRACK_WIDTH::Run()
{
    if( !m_drcEngine->HasRulesForConstraintType( DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_TRACK_WIDTH ) )
    {
        reportAux( "No track width constraints found. Skipping check." );
        return false;
    }

    reportPhase( _( "Track widths..." ));

    auto checkTrackWidth =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_WIDTH ) )
                    return false;

                int     actual;
                wxPoint p0;

                if( ARC* arc = dyn_cast<ARC*>( item ) )
                {
                    actual = arc->GetWidth();
                    p0 = arc->GetStart();
                }
                else if( TRACK* trk = dyn_cast<TRACK*>( item ) )
                {
                    actual = trk->GetWidth();
                    p0 = ( trk->GetStart() + trk->GetEnd() ) / 2;
                }

                auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_TRACK_WIDTH,
                                                                  item );
                bool fail_min = false;
                bool fail_max = false;
                int  constraintWidth;

                if( constraint.Value().HasMin() && actual < constraint.Value().Min() )
                {
                    fail_min        = true;
                    constraintWidth = constraint.Value().Min();
                }

                if( constraint.Value().HasMax() && actual > constraint.Value().Max() )
                {
                    fail_max        = true;
                    constraintWidth = constraint.Value().Max();
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TRACK_WIDTH );

                    m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s width %s; actual %s)" ),
                                  constraint.GetName(),
                                  fail_min ? _( "min" ) : _( "max" ),
                                  MessageTextFromValue( userUnits(), constraintWidth, true ),
                                  MessageTextFromValue( userUnits(), actual, true ) );

                    drcItem->SetErrorMessage( m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, p0 );
                }

                return true;
            };

    forEachGeometryItem( { PCB_TRACE_T, PCB_ARC_T }, LSET::AllCuMask(), checkTrackWidth );

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_TRACK_WIDTH::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_TRACK_WIDTH::GetConstraintTypes() const
{
    return { DRC_CONSTRAINT_TYPE_TRACK_WIDTH };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TRACK_WIDTH> dummy;
}