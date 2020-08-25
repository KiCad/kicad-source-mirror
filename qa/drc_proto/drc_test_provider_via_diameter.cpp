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

#include <class_board.h>
#include <class_track.h>
#include <common.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider.h>


/*
    Via diameter test.

    Errors generated:
    - DRCE_TOO_SMALL_VIA
    - DRCE_TOO_SMALL_MICROVIA
*/

namespace test
{

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

    virtual std::set<test::DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;
};

}; // namespace test


bool test::DRC_TEST_PROVIDER_VIA_DIAMETER::Run()
{
    if( !m_drcEngine->HasCorrectRulesForId(
                test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_VIA_DIAMETER ) )
    {
        ReportAux( "No diameter constraints found. Skipping check." );
        return false;
    }

    ReportStage( ( "Testing via diameters" ), 0, 2 );

    auto checkViaDiameter = [&]( BOARD_ITEM* item ) -> bool {
        auto via = dyn_cast<VIA*>( item );

        // fixme: move to pad stack check?
        if( !via )
            return true;

        test::DRC_CONSTRAINT constraint = m_drcEngine->EvalRulesForItems(
                test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_VIA_DIAMETER, item );

        bool fail_min = false, fail_max = false;
        int constraintDiameter;
        int diameter = via->GetWidth();

        if( constraint.Value().HasMin() && diameter < constraint.Value().Min() )
        {
            fail_min        = true;
           constraintDiameter = constraint.Value().Min();
        }

        if( constraint.Value().HasMax() && diameter > constraint.Value().Max() )
        {
            fail_max        = true;
           constraintDiameter = constraint.Value().Max();
        }

        if( fail_min || fail_max )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_VIA_DIAMETER );
            wxString                  msg;

            msg.Printf( drcItem->GetErrorText() + _( " (%s; diameter %s, constraint %s %s)" ),
                    constraint.GetParentRule()->GetName(),
                    MessageTextFromValue( userUnits(), diameter, true ),
                    fail_min ? _( "minimum" ) : _( "maximum" ),
                    MessageTextFromValue( userUnits(), constraintDiameter, true ) );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( item );
            drcItem->SetViolatingRule( constraint.GetParentRule() );

            ReportWithMarker( drcItem, via->GetPosition() );

            if( isErrorLimitExceeded( DRCE_VIA_DIAMETER ) )
                return false;

        }

        return true;
    };

    forEachGeometryItem( { PCB_VIA_T }, LSET::AllCuMask(), checkViaDiameter );

    reportRuleStatistics();

    return true;
}


std::set<test::DRC_CONSTRAINT_TYPE_T>
test::DRC_TEST_PROVIDER_VIA_DIAMETER::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_VIA_DIAMETER };
}


namespace detail
{
static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_VIA_DIAMETER> dummy;
}