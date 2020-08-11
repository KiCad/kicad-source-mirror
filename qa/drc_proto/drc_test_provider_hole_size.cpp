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
#include <class_drawsegment.h>
#include <class_pad.h>
#include <common.h>

#include <convert_basic_shapes_to_polygon.h>
#include <geometry/polygon_test_point_inside.h>

#include <geometry/seg.h>
#include <geometry/shape_poly_set.h>
#include <geometry/shape_rect.h>
#include <geometry/shape_segment.h>

#include <drc_proto/drc_engine.h>
#include <drc_proto/drc_item.h>
#include <drc_proto/drc_rule.h>
#include <drc_proto/drc_test_provider_clearance_base.h>


/*
    Drilled hole size test. scans vias/through-hole pads and checks for min drill sizes
    Errors generated:
    - DRCE_TOO_SMALL_DRILL
    - DRCE_TOO_SMALL_MICROVIA_DRILL

    TODO: max drill size check
*/

namespace test
{

class DRC_TEST_PROVIDER_HOLE_SIZE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_HOLE_SIZE() :
        m_board( nullptr )
    {
    }

    virtual ~DRC_TEST_PROVIDER_HOLE_SIZE()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "hole_size";
    };

    virtual const wxString GetDescription() const override
    {
        return "Tests sizes of drilled holes (via/pad drills)";
    }

    virtual std::set<test::DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:
    bool checkVia( VIA* via );
    bool checkPad( D_PAD* aPad );

    BOARD* m_board;
};

}; // namespace test


bool test::DRC_TEST_PROVIDER_HOLE_SIZE::Run()
{
    ReportStage( ( "Testing pad holes" ), 0, 2 );

    for( auto module : m_board->Modules() )
    {
        for( auto pad : module->Pads() )
        {
            if( checkPad( pad ) )
                break;
        }
    }

    ReportStage( ( "Testing via/microvia holes" ), 0, 2 );

    std::vector<VIA*> vias;

    for( auto track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
        {
            vias.push_back( static_cast<VIA*>( track ) );
        }
    }


    for( auto via : vias )
    {
        if( checkVia( via ) )
            break;
    }

    reportRuleStatistics();

    return true;
}


bool test::DRC_TEST_PROVIDER_HOLE_SIZE::checkPad( D_PAD* aPad )
{
    int holeSize = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    if( holeSize == 0 )
        return true;

    auto constraint = m_drcEngine->EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE, aPad );
    auto minHole = constraint.GetValue().Min();

    accountCheck( constraint );

    if( holeSize < minHole )
    {
        wxString  msg;
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TOO_SMALL_DRILL );

        msg.Printf( drcItem->GetErrorText() + _( " (%s; actual %s)" ),
                MessageTextFromValue( userUnits(), minHole, true ),
                MessageTextFromValue( userUnits(), holeSize, true ) );

        drcItem->SetViolatingRule( constraint.GetParentRule() );
        drcItem->SetErrorMessage( msg );
        drcItem->SetItems( aPad );

        ReportWithMarker( drcItem, aPad->GetPosition() );

        return isErrorLimitExceeded( DRCE_TOO_SMALL_DRILL );
    }

    return false;
}


bool test::DRC_TEST_PROVIDER_HOLE_SIZE::checkVia( VIA* via )
{
    auto constraint = m_drcEngine->EvalRulesForItems( test::DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE, via );
    auto minHole = constraint.GetValue().Min();

    accountCheck( constraint );

    if( via->GetDrillValue() < minHole )
    {
        wxString msg;
        int errorCode = via->GetViaType() == VIATYPE::MICROVIA ? DRCE_TOO_SMALL_MICROVIA_DRILL :
                                                                 DRCE_TOO_SMALL_DRILL;

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );

        msg.Printf( drcItem->GetErrorText() + _( " (%s; actual %s)" ),
                MessageTextFromValue( userUnits(), minHole, true ),
                MessageTextFromValue( userUnits(), via->GetDrillValue(), true ) );

        drcItem->SetViolatingRule( constraint.GetParentRule() );
        drcItem->SetErrorMessage( msg );
        drcItem->SetItems( via );

        ReportWithMarker( drcItem, via->GetPosition() );

        return isErrorLimitExceeded( errorCode );
    }

    return false;
}


std::set<test::DRC_CONSTRAINT_TYPE_T> test::DRC_TEST_PROVIDER_HOLE_SIZE::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE };
}


namespace detail
{
static test::DRC_REGISTER_TEST_PROVIDER<test::DRC_TEST_PROVIDER_HOLE_SIZE> dummy;
}
