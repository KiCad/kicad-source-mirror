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

#include <pad.h>
#include <track.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>


/*
    Drilled hole size test. scans vias/through-hole pads and checks for min drill sizes
    Errors generated:
    - DRCE_DRILL_OUT_OF_RANGE
    - DRCE_MICROVIA_DRILL_OUT_OF_RANGE
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

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    void checkVia( VIA* via, bool aExceedMicro, bool aExceedStd );
    void checkPad( PAD* aPad );

    BOARD* m_board;
};


bool DRC_TEST_PROVIDER_HOLE_SIZE::Run()
{
    if( !reportPhase( _( "Checking pad holes..." ) ) )
        return false;

    m_board = m_drcEngine->GetBoard();

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
            break;

        for( PAD* pad : footprint->Pads() )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
                break;

            checkPad( pad );
        }
    }

    if( !reportPhase( _( "Checking via holes..." ) ) )
        return false;

    std::vector<VIA*> vias;

    for( TRACK* track : m_board->Tracks() )
    {
        if( track->Type() == PCB_VIA_T )
            vias.push_back( static_cast<VIA*>( track ) );
    }


    for( VIA* via : vias )
    {
        bool exceedMicro = m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_DRILL_OUT_OF_RANGE );
        bool exceedStd = m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE );

        if( exceedMicro && exceedStd )
            break;

        checkVia( via, exceedMicro, exceedStd );
    }

    reportRuleStatistics();

    return true;
}


void DRC_TEST_PROVIDER_HOLE_SIZE::checkPad( PAD* aPad )
{
    int holeMinor = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );
    int holeMajor = std::max( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    if( holeMinor == 0 )
        return;

    auto constraint = m_drcEngine->EvalRulesForItems( HOLE_SIZE_CONSTRAINT, aPad );
    bool fail_min = false;
    bool fail_max = false;
    int  constraintValue;

    if( constraint.Value().HasMin() && holeMinor < constraint.Value().Min() )
    {
        fail_min        = true;
        constraintValue = constraint.Value().Min();
    }

    if( constraint.Value().HasMax() && holeMajor > constraint.Value().Max() )
    {
        fail_max        = true;
        constraintValue = constraint.Value().Max();
    }

    if( fail_min || fail_max )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DRILL_OUT_OF_RANGE );

        if( fail_min )
        {
            m_msg.Printf( _( "(%s min width %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), constraintValue ),
                          MessageTextFromValue( userUnits(), holeMinor ) );
        }
        else
        {
            m_msg.Printf( _( "(%s max width %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), constraintValue ),
                          MessageTextFromValue( userUnits(), holeMajor ) );
        }

        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
        drcItem->SetItems( aPad );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        reportViolation( drcItem, aPad->GetPosition() );
    }
}


void DRC_TEST_PROVIDER_HOLE_SIZE::checkVia( VIA* via, bool aExceedMicro, bool aExceedStd )
{
    int errorCode;

    if( via->GetViaType() == VIATYPE::MICROVIA )
    {
        if( aExceedMicro )
            return;

        errorCode = DRCE_MICROVIA_DRILL_OUT_OF_RANGE;
    }
    else
    {
        if( aExceedStd )
            return;

        errorCode = DRCE_DRILL_OUT_OF_RANGE;
    }

    auto constraint = m_drcEngine->EvalRulesForItems( HOLE_SIZE_CONSTRAINT, via );
    bool fail_min = false;
    bool fail_max = false;
    int  constraintValue;

    if( constraint.Value().HasMin() && via->GetDrillValue() < constraint.Value().Min() )
    {
        fail_min        = true;
        constraintValue = constraint.Value().Min();
    }

    if( constraint.Value().HasMax() && via->GetDrillValue() > constraint.Value().Max() )
    {
        fail_max        = true;
        constraintValue = constraint.Value().Max();
    }

    if( fail_min || fail_max )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );

        if( fail_min )
        {
            m_msg.Printf( _( "(%s min width %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), constraintValue ),
                          MessageTextFromValue( userUnits(), via->GetDrillValue() ) );
        }
        else
        {
            m_msg.Printf( _( "(%s max width %s; actual %s)" ),
                          constraint.GetName(),
                          MessageTextFromValue( userUnits(), constraintValue ),
                          MessageTextFromValue( userUnits(), via->GetDrillValue() ) );
        }

        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + m_msg );
        drcItem->SetItems( via );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        reportViolation( drcItem, via->GetPosition() );
    }
}


int DRC_TEST_PROVIDER_HOLE_SIZE::GetNumPhases() const
{
    return 2;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_HOLE_SIZE::GetConstraintTypes() const
{
    return { HOLE_SIZE_CONSTRAINT };
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_SIZE> dummy;
}
