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

#include <class_pad.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc.h>
#include <drc/drc_test_provider_clearance_base.h>


/*
    Drilled hole size test. scans vias/through-hole pads and checks for min drill sizes
    Errors generated:
    - DRCE_TOO_SMALL_DRILL
    - DRCE_TOO_SMALL_MICROVIA_DRILL

    TODO: max drill size check
*/

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

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetMatchingConstraintIds() const override;

private:
    bool checkVia( VIA* via );
    bool checkPad( D_PAD* aPad );

    BOARD* m_board;
};


bool DRC_TEST_PROVIDER_HOLE_SIZE::Run()
{
    ReportStage( _( "Testing pad holes" ), 0, 2 );

    m_board = m_drcEngine->GetBoard();

    for( MODULE* module : m_board->Modules() )
    {
        for( D_PAD* pad : module->Pads() )
        {
            if( checkPad( pad ) )
                break;
        }
    }

    ReportStage( _( "Testing via/microvia holes" ), 0, 2 );

    std::vector<VIA*> vias;

    for( TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            vias.push_back( static_cast<VIA*>( track ) );
    }


    for( VIA* via : vias )
    {
        if( checkVia( via ) )
            break;
    }

    reportRuleStatistics();

    return true;
}


bool DRC_TEST_PROVIDER_HOLE_SIZE::checkPad( D_PAD* aPad )
{
    int holeSize = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    if( holeSize == 0 )
        return true;

    auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_HOLE_SIZE, aPad );
    int  minHole = constraint.GetValue().Min();

    accountCheck( constraint );

    if( holeSize < minHole )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_TOO_SMALL_DRILL );

        m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                      constraint.GetName(),
                      MessageTextFromValue( userUnits(), minHole, true ),
                      MessageTextFromValue( userUnits(), holeSize, true ) );

        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( aPad );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        ReportWithMarker( drcItem, aPad->GetPosition() );

        return isErrorLimitExceeded( DRCE_TOO_SMALL_DRILL );
    }

    return false;
}


bool DRC_TEST_PROVIDER_HOLE_SIZE::checkVia( VIA* via )
{
    auto constraint = m_drcEngine->EvalRulesForItems( DRC_CONSTRAINT_TYPE_HOLE_SIZE, via );
    int  minHole = constraint.GetValue().Min();

    accountCheck( constraint );

    if( via->GetDrillValue() < minHole )
    {
        int errorCode = via->GetViaType() == VIATYPE::MICROVIA ? DRCE_TOO_SMALL_MICROVIA_DRILL :
                                                                 DRCE_TOO_SMALL_DRILL;

        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );

        m_msg.Printf( drcItem->GetErrorText() + _( " (%s %s; actual %s)" ),
                      constraint.GetName(),
                      MessageTextFromValue( userUnits(), minHole, true ),
                      MessageTextFromValue( userUnits(), via->GetDrillValue(), true ) );

        drcItem->SetErrorMessage( m_msg );
        drcItem->SetItems( via );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        ReportWithMarker( drcItem, via->GetPosition() );

        return isErrorLimitExceeded( errorCode );
    }

    return false;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_HOLE_SIZE::GetMatchingConstraintIds() const
{
    return { DRC_CONSTRAINT_TYPE_T::DRC_CONSTRAINT_TYPE_HOLE_SIZE };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_SIZE> dummy;
}
