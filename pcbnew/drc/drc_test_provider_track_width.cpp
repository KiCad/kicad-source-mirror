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
#include <track.h>
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

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_TRACK_WIDTH::Run()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_TRACK_WIDTH ) )
    {
        reportAux( "Track width violations ignored. Tests not run." );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( TRACK_WIDTH_CONSTRAINT ) )
    {
        reportAux( "No track width constraints found. Tests not run." );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking track widths..." ) ) )
        return false;       // DRC cancelled

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
                else
                {
                    return true;
                }

                auto constraint = m_drcEngine->EvalRules( TRACK_WIDTH_CONSTRAINT, item, nullptr,
                                                          item->GetLayer() );
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

                    if( fail_min )
                    {
                        m_msg.Printf( _( "(%s min width %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), constraintWidth ),
                                      MessageTextFromValue( userUnits(), actual ) );
                    }
                    else
                    {
                        m_msg.Printf( _( "(%s max width %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), constraintWidth ),
                                      MessageTextFromValue( userUnits(), actual ) );
                    }

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, p0 );
                }

                return true;
            };

    int ii = 0;

    for( TRACK* item : m_drcEngine->GetBoard()->Tracks() )
    {
        if( !reportProgress( ii++, m_drcEngine->GetBoard()->Tracks().size(), delta ) )
            break;

        if( !checkTrackWidth( item ) )
            break;
    }

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_TRACK_WIDTH::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_TRACK_WIDTH::GetConstraintTypes() const
{
    return { TRACK_WIDTH_CONSTRAINT };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_TRACK_WIDTH> dummy;
}
