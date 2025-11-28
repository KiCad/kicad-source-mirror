/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers.
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

#include <geometry/shape_segment.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_track.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider.h>

/*
    Drilled hole size test. scans vias/through-hole pads and checks for min drill sizes
    Errors generated:
    - DRCE_DRILL_OUT_OF_RANGE
    - DRCE_MICROVIA_DRILL_OUT_OF_RANGE
    - DRCE_PADSTACK
*/

class DRC_TEST_PROVIDER_HOLE_SIZE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_HOLE_SIZE()
    {}

    virtual ~DRC_TEST_PROVIDER_HOLE_SIZE() = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "hole_size" ); };

private:
    void checkViaHole( PCB_VIA* via, bool aExceedMicro, bool aExceedStd );
    void checkPadHole( PAD* aPad );
};


bool DRC_TEST_PROVIDER_HOLE_SIZE::Run()
{
    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
    {
        if( !reportPhase( _( "Checking pad holes..." ) ) )
            return false;   // DRC cancelled

        for( FOOTPRINT* footprint : m_drcEngine->GetBoard()->Footprints() )
        {
            for( PAD* pad : footprint->Pads() )
            {
                if( !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
                    checkPadHole( pad );
            }
        }
    }

    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_DRILL_OUT_OF_RANGE )
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
    {
        if( !m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE ) )
        {
            if( !reportPhase( _( "Checking via holes..." ) ) )
                return false;   // DRC cancelled
        }
        else
        {
            if( !reportPhase( _( "Checking micro-via holes..." ) ) )
                return false;   // DRC cancelled
        }

        for( PCB_TRACK* track : m_drcEngine->GetBoard()->Tracks() )
        {
            if( track->Type() == PCB_VIA_T )
            {
                bool exceedMicro = m_drcEngine->IsErrorLimitExceeded( DRCE_MICROVIA_DRILL_OUT_OF_RANGE );
                bool exceedStd = m_drcEngine->IsErrorLimitExceeded( DRCE_DRILL_OUT_OF_RANGE );

                if( exceedMicro && exceedStd )
                    break;

                checkViaHole( static_cast<PCB_VIA*>( track ), exceedMicro, exceedStd );
            }
        }
    }

    return !m_drcEngine->IsCancelled();
}


void DRC_TEST_PROVIDER_HOLE_SIZE::checkPadHole( PAD* aPad )
{
    int holeMinor = std::min( aPad->GetDrillSize().x, aPad->GetDrillSize().y );
    int holeMajor = std::max( aPad->GetDrillSize().x, aPad->GetDrillSize().y );

    if( holeMinor == 0 )
        return;

    auto constraint = m_drcEngine->EvalRules( HOLE_SIZE_CONSTRAINT, aPad, nullptr,
                                              UNDEFINED_LAYER /* holes are not layer-specific */ );
    bool     fail_min = false;
    bool     fail_max = false;
    int      constraintValue = 0;
    VECTOR2I ptA, ptB;

    if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
        return;

    if( constraint.Value().HasMax() && holeMajor > constraint.Value().Max() )
    {
        fail_max        = true;
        constraintValue = constraint.Value().Max();

        ptA = aPad->GetPosition();

        if( aPad->GetDrillSizeX() == aPad->GetDrillSizeY() )
            ptB = ptA - VECTOR2I( aPad->GetDrillSize() ).Resize( aPad->GetDrillSizeX() / 2 );
        else if( aPad->GetDrillSizeX() > aPad->GetDrillSizeY() )
            ptB = ptA - VECTOR2I( aPad->GetDrillSizeX() / 2, 0 );
        else
            ptB = ptA - VECTOR2I( 0, aPad->GetDrillSizeY() / 2 );
    }

    if( constraint.Value().HasMin() && holeMinor < constraint.Value().Min() )
    {
        fail_min        = true;
        constraintValue = constraint.Value().Min();

        ptA = aPad->GetPosition();

        if( aPad->GetDrillSizeX() == aPad->GetDrillSizeY() )
            ptB = ptA - VECTOR2I( aPad->GetDrillSize() ).Resize( aPad->GetDrillSizeX() / 2 );
        else if( aPad->GetDrillSizeX() < aPad->GetDrillSizeY() )
            ptB = ptA - VECTOR2I( aPad->GetDrillSizeX() / 2, 0 );
        else
            ptB = ptA - VECTOR2I( 0, aPad->GetDrillSizeY() / 2 );
    }

    if( fail_min || fail_max )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_DRILL_OUT_OF_RANGE );
        wxString constraintName = constraint.GetName();

        if( fail_min )
        {
            if( constraint.GetParentRule() && constraint.GetParentRule()->IsImplicit() )
                constraintName = _( "board setup constraints" );

            drcItem->SetErrorDetail( formatMsg( _( "(%s min hole %s; actual %s)" ),
                                                constraintName,
                                                constraintValue,
                                                holeMinor ) );
        }
        else
        {
            drcItem->SetErrorDetail( formatMsg( _( "(%s max hole %s; actual %s)" ),
                                                constraintName,
                                                constraintValue,
                                                holeMajor ) );
        }

        drcItem->SetItems( aPad );
        drcItem->SetViolatingRule( constraint.GetParentRule() );
        reportTwoPointGeometry( drcItem, ptA, ptA, ptB, UNDEFINED_LAYER );
    }
}


void DRC_TEST_PROVIDER_HOLE_SIZE::checkViaHole( PCB_VIA* via, bool aExceedMicro, bool aExceedStd )
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

    auto constraint = m_drcEngine->EvalRules( HOLE_SIZE_CONSTRAINT, via, nullptr,
                                              UNDEFINED_LAYER /* holes are not layer-specific */ );
    bool fail_min = false;
    bool fail_max = false;
    int  constraintValue = 0;
    int  drill = via->GetDrillValue();

    if( constraint.GetSeverity() == RPT_SEVERITY_IGNORE )
        return;

    if( constraint.Value().HasMin() && drill < constraint.Value().Min() )
    {
        fail_min        = true;
        constraintValue = constraint.Value().Min();
    }

    if( constraint.Value().HasMax() && drill > constraint.Value().Max() )
    {
        fail_max        = true;
        constraintValue = constraint.Value().Max();
    }

    if( fail_min || fail_max )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( errorCode );
        wxString constraintName = constraint.GetName();

        if( fail_min )
        {
            if( constraint.m_ImplicitMin )
                constraintName = _( "board setup constraints" );

            drcItem->SetErrorDetail( formatMsg( _( "(%s min hole %s; actual %s)" ),
                                                constraintName,
                                                constraintValue,
                                                drill ) );
        }
        else
        {
            drcItem->SetErrorDetail( formatMsg( _( "(%s max hole %s; actual %s)" ),
                                                constraintName,
                                                constraintValue,
                                                drill ) );
        }

        drcItem->SetItems( via );
        drcItem->SetViolatingRule( constraint.GetParentRule() );

        VECTOR2I ptA = via->GetPosition();

        VECTOR2I ptB = ptA - VECTOR2I( drill, drill ).Resize( drill / 2 );
        reportTwoPointGeometry( drcItem, ptA, ptA, ptB, UNDEFINED_LAYER );
    }
}


namespace detail
{
static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_HOLE_SIZE> dummy;
}
