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

#include <pcb_track.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

/*
    Via diameter test.

    Errors generated:
    - DRCE_VIA_DIAMETER
*/

class DRC_TEST_PROVIDER_VIA_DIAMETER : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_VIA_DIAMETER()
    {
    }

    virtual ~DRC_TEST_PROVIDER_VIA_DIAMETER()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "diameter";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests via diameters";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;
};


bool DRC_TEST_PROVIDER_VIA_DIAMETER::Run()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    if( m_drcEngine->IsErrorLimitExceeded( DRCE_VIA_DIAMETER ) )
    {
        reportAux( "Via diameter violations ignored. Tests not run." );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( VIA_DIAMETER_CONSTRAINT ) )
    {
        reportAux( "No via diameter constraints found. Tests not run." );
        return true;        // continue with other tests
    }

    if( !reportPhase( _( "Checking via diameters..." ) ) )
        return false;       // DRC cancelled

    auto checkViaDiameter =
            [&]( BOARD_ITEM* item ) -> bool
            {
                if( m_drcEngine->IsErrorLimitExceeded( DRCE_VIA_DIAMETER ) )
                    return false;

                PCB_VIA* via = dyn_cast<PCB_VIA*>( item );

                // fixme: move to pad stack check?
                if( !via )
                    return true;

                // TODO: once we have padstacks this will need to run per-layer...
                auto constraint = m_drcEngine->EvalRules( VIA_DIAMETER_CONSTRAINT, item, nullptr,
                                                          UNDEFINED_LAYER );
                bool fail_min = false;
                bool fail_max = false;
                int  constraintDiameter = 0;
                int  actual = via->GetWidth();

                if( constraint.Value().HasMin() && actual < constraint.Value().Min() )
                {
                    fail_min = true;
                    constraintDiameter = constraint.Value().Min();
                }

                if( constraint.Value().HasMax() && actual > constraint.Value().Max() )
                {
                    fail_max = true;
                    constraintDiameter = constraint.Value().Max();
                }

                if( fail_min )
                {
                    m_msg.Printf( _( "(%s min diameter %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), constraintDiameter ),
                                  MessageTextFromValue( userUnits(), actual ) );
                }
                else if( fail_max )
                {
                    m_msg.Printf( _( "(%s max diameter %s; actual %s)" ),
                                  constraint.GetName(),
                                  MessageTextFromValue( userUnits(), constraintDiameter ),
                                  MessageTextFromValue( userUnits(), actual ) );
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_DIAMETER );

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, via->GetPosition() );
                }

                return true;
            };

    int ii = 0;

    for( PCB_TRACK* item : m_drcEngine->GetBoard()->Tracks() )
    {
        if( !reportProgress( ii++, m_drcEngine->GetBoard()->Tracks().size(), delta ) )
            break;

        if( !checkViaDiameter( item ) )
            break;
    }

    reportRuleStatistics();

    return true;
}


int DRC_TEST_PROVIDER_VIA_DIAMETER::GetNumPhases() const
{
    return 1;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_VIA_DIAMETER::GetConstraintTypes() const
{
    return { VIA_DIAMETER_CONSTRAINT };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_VIA_DIAMETER> dummy;
}
