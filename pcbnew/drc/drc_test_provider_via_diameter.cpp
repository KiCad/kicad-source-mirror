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
        return wxT( "diameter" );
    };

    virtual const wxString GetDescription() const override
    {
        return wxT( "Tests via diameters" );
    }
};


bool DRC_TEST_PROVIDER_VIA_DIAMETER::Run()
{
    if( m_drcEngine->IsErrorLimitExceeded( DRCE_VIA_DIAMETER ) )
    {
        reportAux( wxT( "Via diameter violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    if( !m_drcEngine->HasRulesForConstraintType( VIA_DIAMETER_CONSTRAINT ) )
    {
        reportAux( wxT( "No via diameter constraints found. Tests not run." ) );
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

                if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE )
                {
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
                }

                if( fail_min || fail_max )
                {
                    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_DIAMETER );
                    wxString msg;

                    if( fail_min )
                    {
                        msg = formatMsg( _( "(%s min diameter %s; actual %s)" ),
                                         constraint.GetName(),
                                         constraintDiameter,
                                         actual );
                    }
                    else if( fail_max )
                    {
                        msg = formatMsg( _( "(%s max diameter %s; actual %s)" ),
                                         constraint.GetName(),
                                         constraintDiameter,
                                         actual );
                    }

                    drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
                    drcItem->SetItems( item );
                    drcItem->SetViolatingRule( constraint.GetParentRule() );

                    reportViolation( drcItem, via->GetPosition(), via->GetLayer() );
                }

                return true;
            };

    const int progressDelta = 500;
    int       ii = 0;

    for( PCB_TRACK* item : m_drcEngine->GetBoard()->Tracks() )
    {
        if( !reportProgress( ii++, m_drcEngine->GetBoard()->Tracks().size(), progressDelta ) )
            break;

        if( !checkViaDiameter( item ) )
            break;
    }

    reportRuleStatistics();

    return !m_drcEngine->IsCancelled();
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_VIA_DIAMETER> dummy;
}
