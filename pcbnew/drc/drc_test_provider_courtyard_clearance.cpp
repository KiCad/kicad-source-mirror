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

#include <geometry/shape_poly_set.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <pad.h>
#include <geometry/shape_segment.h>
#include <drc/drc_test_provider_clearance_base.h>
#include <footprint.h>

/*
    Couartyard clearance. Tests for malformed component courtyards and overlapping footprints.
    Generated errors:
    - DRCE_OVERLAPPING_FOOTPRINTS
    - DRCE_MISSING_COURTYARD
    - DRCE_MALFORMED_COURTYARD
    - DRCE_PTH_IN_COURTYARD,
    - DRCE_NPTH_IN_COURTYARD,

    TODO: do an actual clearance check instead of polygon intersection. Treat closed outlines
    as filled and allow open curves in the courtyard.
*/

class DRC_TEST_PROVIDER_COURTYARD_CLEARANCE : public DRC_TEST_PROVIDER_CLEARANCE_BASE
{
public:
    DRC_TEST_PROVIDER_COURTYARD_CLEARANCE ()
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_COURTYARD_CLEARANCE ()
    {
    }

    virtual bool Run() override;

    virtual const wxString GetName() const override
    {
        return "courtyard_clearance";
    }

    virtual const wxString GetDescription() const override
    {
        return "Tests footprints' courtyard clearance";
    }

    virtual std::set<DRC_CONSTRAINT_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    bool testFootprintCourtyardDefinitions();

    bool testCourtyardClearances();
};


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testFootprintCourtyardDefinitions()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    // Detects missing (or malformed) footprint courtyards
    if( !m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_COURTYARD)
            || !m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_COURTYARD) )
    {
        if( !reportPhase( _( "Checking footprint courtyard definitions..." ) ) )
            return false;   // DRC cancelled
    }
    else if( !m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS) )
    {
        if( !reportPhase( _( "Gathering footprint courtyards..." ) ) )
            return false;   // DRC cancelled
    }
    else
    {
        reportAux( "All courtyard violations ignored. Tests not run." );
        return true;        // continue with other tests
    }

    int ii = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( !reportProgress( ii++, m_board->Footprints().size(), delta ) )
            return false;   // DRC cancelled

        if( ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_COURTYARD) )
                continue;

            OUTLINE_ERROR_HANDLER errorHandler =
                    [&]( const wxString& msg, BOARD_ITEM* , BOARD_ITEM* , const wxPoint& pt )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD );
                        drcItem->SetErrorMessage( drcItem->GetErrorText() + wxS( " " ) + msg );
                        drcItem->SetItems( footprint );
                        reportViolation( drcItem, pt );
                    };

            // Re-run courtyard tests to generate DRC_ITEMs
            footprint->BuildPolyCourtyards( &errorHandler );
        }
        else if( footprint->GetPolyCourtyard( F_CrtYd ).OutlineCount() == 0
                && footprint->GetPolyCourtyard( B_CrtYd ).OutlineCount() == 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_COURTYARD ) )
                continue;

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_COURTYARD );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetPosition() );
        }
        else
        {
            footprint->GetPolyCourtyard( F_CrtYd ).BuildBBoxCaches();
            footprint->GetPolyCourtyard( B_CrtYd ).BuildBBoxCaches();
        }
    }

    return true;
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testCourtyardClearances()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    if( !reportPhase( _( "Checking footprints for overlapping courtyards..." ) ) )
        return false;   // DRC cancelled

    int ii = 0;

    for( auto itA = m_board->Footprints().begin(); itA != m_board->Footprints().end(); itA++ )
    {
        if( !reportProgress( ii++, m_board->Footprints().size(), delta ) )
            return false;   // DRC cancelled

        if( m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS)
            && m_drcEngine->IsErrorLimitExceeded( DRCE_PTH_IN_COURTYARD )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_NPTH_IN_COURTYARD ) )
        {
            return true;   // continue with other tests
        }

        FOOTPRINT*            fpA = *itA;
        const SHAPE_POLY_SET& frontA = fpA->GetPolyCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backA = fpA->GetPolyCourtyard( B_CrtYd );

        if( frontA.OutlineCount() == 0 && backA.OutlineCount() == 0
             && m_drcEngine->IsErrorLimitExceeded( DRCE_PTH_IN_COURTYARD )
             && m_drcEngine->IsErrorLimitExceeded( DRCE_NPTH_IN_COURTYARD ) )
        {
            // No courtyards defined and no hole testing against other footprint's courtyards
            continue;
        }

        BOX2I frontBBox = frontA.BBoxFromCaches();
        BOX2I backBBox = backA.BBoxFromCaches();

        frontBBox.Inflate( m_largestClearance );
        backBBox.Inflate( m_largestClearance );

        EDA_RECT fpABBox = fpA->GetBoundingBox();

        for( auto itB = itA + 1; itB != m_board->Footprints().end(); itB++ )
        {
            FOOTPRINT*            fpB = *itB;
            EDA_RECT              fpBBBox = fpB->GetBoundingBox();
            const SHAPE_POLY_SET& frontB = fpB->GetPolyCourtyard( F_CrtYd );
            const SHAPE_POLY_SET& backB = fpB->GetPolyCourtyard( B_CrtYd );
            DRC_CONSTRAINT        constraint;
            int                   clearance;
            int                   actual;
            VECTOR2I              pos;

            if( frontA.OutlineCount() > 0 && frontB.OutlineCount() > 0
                    && frontBBox.Intersects( frontB.BBoxFromCaches() ) )
            {
                constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB,
                                                     F_Cu );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 && frontA.Collide( &frontB, clearance, &actual, &pos ) )
                {
                    auto drce = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );

                    if( clearance > 0 )
                    {
                        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), clearance ),
                                      MessageTextFromValue( userUnits(), actual ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                        drce->SetViolatingRule( constraint.GetParentRule() );
                    }

                    drce->SetItems( fpA, fpB );
                    reportViolation( drce, (wxPoint) pos );
                }
            }

            if( backA.OutlineCount() > 0 && backB.OutlineCount() > 0
                    && backBBox.Intersects( backB.BBoxFromCaches() ) )
            {
                constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB,
                                                     B_Cu );
                clearance = constraint.GetValue().Min();

                if( clearance >= 0 && backA.Collide( &backB, clearance, &actual, &pos ) )
                {
                    auto drce = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );

                    if( clearance > 0 )
                    {
                        m_msg.Printf( _( "(%s clearance %s; actual %s)" ),
                                      constraint.GetName(),
                                      MessageTextFromValue( userUnits(), clearance ),
                                      MessageTextFromValue( userUnits(), actual ) );

                        drce->SetErrorMessage( drce->GetErrorText() + wxS( " " ) + m_msg );
                        drce->SetViolatingRule( constraint.GetParentRule() );
                    }

                    drce->SetItems( fpA, fpB );
                    reportViolation( drce, (wxPoint) pos );
                }
            }

            auto testPadAgainstCourtyards =
                    [&]( const PAD* pad, const FOOTPRINT* footprint )
                    {
                        int errorCode = 0;

                        if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                            errorCode = DRCE_PTH_IN_COURTYARD;
                        else if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                            errorCode = DRCE_NPTH_IN_COURTYARD;
                        else
                            return;

                        if( m_drcEngine->IsErrorLimitExceeded( errorCode ) )
                            return;

                        const SHAPE_SEGMENT*  hole = pad->GetEffectiveHoleShape();
                        const SHAPE_POLY_SET& front = footprint->GetPolyCourtyard( F_CrtYd );
                        const SHAPE_POLY_SET& back = footprint->GetPolyCourtyard( B_CrtYd );

                        if( ( front.OutlineCount() > 0 && front.Collide( hole, 0 ) )
                            || ( back.OutlineCount() > 0 && back.Collide( hole, 0 ) ) )
                        {
                            std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( errorCode );
                            drce->SetItems( pad, footprint );
                            reportViolation( drce, pad->GetPosition() );
                        }
                    };

            if( ( frontA.OutlineCount() > 0 && frontA.BBoxFromCaches().Intersects( fpBBBox ) )
                || ( backA.OutlineCount() > 0 && backA.BBoxFromCaches().Intersects( fpBBBox ) ) )
            {
                for( const PAD* padB : fpB->Pads() )
                    testPadAgainstCourtyards( padB, fpA );
            }

            if( ( frontB.OutlineCount() > 0 && frontB.BBoxFromCaches().Intersects( fpABBox ) )
                || ( backB.OutlineCount() > 0 && backB.BBoxFromCaches().Intersects( fpABBox ) ) )
            {
                for( const PAD* padA : fpA->Pads() )
                    testPadAgainstCourtyards( padA, fpB );
            }
        }
    }

    return true;
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    DRC_CONSTRAINT constraint;

    if( m_drcEngine->QueryWorstConstraint( COURTYARD_CLEARANCE_CONSTRAINT, constraint ) )
        m_largestClearance = constraint.GetValue().Min();

    reportAux( "Worst courtyard clearance : %d nm", m_largestClearance );

    if( !testFootprintCourtyardDefinitions() )
        return false;

   if( !testCourtyardClearances() )
       return false;

    return true;
}


int DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::GetNumPhases() const
{
    return 2;
}


std::set<DRC_CONSTRAINT_T> DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::GetConstraintTypes() const
{
    return { COURTYARD_CLEARANCE_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COURTYARD_CLEARANCE> dummy;
}
