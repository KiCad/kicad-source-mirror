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
#include <common.h>

#include <geometry/shape.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider.h>

/*
    "Disallow" test. Goes through all items, matching types/conditions drop errors.
    Errors generated:
    - DRCE_ALLOWED_ITEMS
*/

namespace test
{

class DRC_TEST_PROVIDER_DISALLOW : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual ~DRC_TEST_PROVIDER_DISALLOW()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "disallow";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests for disallowed items (e.g. keepouts)";
    }

    virtual std::set<test::DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:
};

}; // namespace test


bool test::DRC_TEST_PROVIDER_DISALLOW::Run()
{
    if( !m_drcEngine->HasCorrectRulesForId( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_DISALLOW ) )
    {
        ReportAux( "No disallow constraints found. Skipping check." );
        return false;
    }

    ReportStage( ("Testing for disallow constraints"), 0, 2 );

    auto checkItem = [&] ( BOARD_ITEM *item ) -> bool
    {
        test::DRC_CONSTRAINT constraint = m_drcEngine->EvalRulesForItems(
                test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_DISALLOW, item );

        if( constraint.Allowed() )
            return true;

        if( ( constraint.GetAllowedLayers() & item->GetLayerSet() ).any() )
        {
            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_ALLOWED_ITEMS );
            wxString                  msg;

            msg.Printf(
                    drcItem->GetErrorText() + _( " (%s)" ), constraint.GetParentRule()->GetName() );

            drcItem->SetErrorMessage( msg );
            drcItem->SetItems( item );
            drcItem->SetViolatingRule( constraint.GetParentRule() );

            ReportWithMarker( drcItem, item->GetPosition() );

            if( isErrorLimitExceeded( DRCE_ALLOWED_ITEMS ) )
                return false;
        }
        return true;
    };

    forEachGeometryItem( {}, LSET::AllLayersMask(), checkItem );

    reportRuleStatistics();

    return true;
}


std::set<test::DRC_CONSTRAINT_TYPE_T>
test::DRC_TEST_PROVIDER_DISALLOW::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_DISALLOW };
}


namespace detail
{
static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_DISALLOW> dummy;
}
