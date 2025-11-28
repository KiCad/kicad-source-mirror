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

#include <geometry/shape_poly_set.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pad.h>
#include <geometry/shape_segment.h>
#include <drc/drc_test_provider.h>
#include <footprint.h>

/*
    Couartyard clearance. Tests for malformed component courtyards and overlapping footprints.
    Generated errors:
    - DRCE_OVERLAPPING_FOOTPRINTS
    - DRCE_MISSING_COURTYARD
    - DRCE_MALFORMED_COURTYARD
    - DRCE_PTH_IN_COURTYARD,
    - DRCE_NPTH_IN_COURTYARD,
*/

class DRC_TEST_PROVIDER_COURTYARD_CLEARANCE : public DRC_TEST_PROVIDER
{
public:
    DRC_TEST_PROVIDER_COURTYARD_CLEARANCE () :
            DRC_TEST_PROVIDER(),
            m_largestCourtyardClearance( 0 )
    {
        m_isRuleDriven = false;
    }

    virtual ~DRC_TEST_PROVIDER_COURTYARD_CLEARANCE () = default;

    virtual bool Run() override;

    virtual const wxString GetName() const override { return wxT( "courtyard_clearance" ); }

private:
    bool testFootprintCourtyardDefinitions();

    bool testCourtyardClearances();

private:
    int  m_largestCourtyardClearance;
};


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testFootprintCourtyardDefinitions()
{
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
        REPORT_AUX( wxT( "All courtyard violations ignored. Tests not run." ) );
        return true;        // continue with other tests
    }

    const int progressDelta = 500;
    int       ii = 0;

    for( FOOTPRINT* footprint : m_board->Footprints() )
    {
        if( !reportProgress( ii++, m_board->Footprints().size(), progressDelta ) )
            return false;   // DRC cancelled

        if( ( footprint->GetFlags() & MALFORMED_COURTYARDS ) != 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_COURTYARD) )
                continue;

            OUTLINE_ERROR_HANDLER errorHandler =
                    [&]( const wxString& msg, BOARD_ITEM*, BOARD_ITEM*, const VECTOR2I& pt )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD );
                        drcItem->SetErrorDetail( msg );
                        drcItem->SetItems( footprint );
                        reportViolation( drcItem, pt, UNDEFINED_LAYER );
                    };

            // Re-run courtyard tests to generate DRC_ITEMs
            footprint->BuildCourtyardCaches( &errorHandler );
        }
        else if( footprint->GetCourtyard( F_CrtYd ).OutlineCount() == 0
                && footprint->GetCourtyard( B_CrtYd ).OutlineCount() == 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_COURTYARD ) )
                continue;

            if( footprint->AllowMissingCourtyard() )
                continue;

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_COURTYARD );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetPosition(), UNDEFINED_LAYER );
        }
        else
        {
            footprint->GetCourtyard( F_CrtYd ).BuildBBoxCaches();
            footprint->GetCourtyard( B_CrtYd ).BuildBBoxCaches();
        }
    }

    return !m_drcEngine->IsCancelled();
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testCourtyardClearances()
{
    if( !reportPhase( _( "Checking footprints for overlapping courtyards..." ) ) )
        return false;   // DRC cancelled

    const int progressDelta = 100;
    int       ii = 0;

    // Stable sorting gives stable violation generation (and stable comparisons to previously-
    // generated violations for exclusion checking).
    std::vector<FOOTPRINT*> footprints;

    footprints.insert( footprints.begin(), m_board->Footprints().begin(),
                       m_board->Footprints().end() );

    std::sort( footprints.begin(), footprints.end(),
               []( const FOOTPRINT* a, const FOOTPRINT* b )
               {
                   return a->m_Uuid < b->m_Uuid;
               } );

    for( auto itA = footprints.begin(); itA != footprints.end(); itA++ )
    {
        if( !reportProgress( ii++, footprints.size(), progressDelta ) )
            return false;   // DRC cancelled

        // Ensure tests realted to courtyard constraints are not fully disabled:
        if( m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS)
            && m_drcEngine->IsErrorLimitExceeded( DRCE_PTH_IN_COURTYARD )
            && m_drcEngine->IsErrorLimitExceeded( DRCE_NPTH_IN_COURTYARD ) )
        {
            return true;   // continue with other tests
        }

        FOOTPRINT*            fpA = *itA;
        const SHAPE_POLY_SET& frontA = fpA->GetCourtyard( F_CrtYd );
        const SHAPE_POLY_SET& backA = fpA->GetCourtyard( B_CrtYd );

        if( frontA.OutlineCount() == 0 && backA.OutlineCount() == 0
             && m_drcEngine->IsErrorLimitExceeded( DRCE_PTH_IN_COURTYARD )
             && m_drcEngine->IsErrorLimitExceeded( DRCE_NPTH_IN_COURTYARD ) )
        {
            // No courtyards defined and no hole testing against other footprint's courtyards
            continue;
        }

        BOX2I frontA_worstCaseBBox = frontA.BBoxFromCaches();
        BOX2I backA_worstCaseBBox = backA.BBoxFromCaches();

        frontA_worstCaseBBox.Inflate( m_largestCourtyardClearance );
        backA_worstCaseBBox.Inflate( m_largestCourtyardClearance );

        BOX2I fpA_bbox = fpA->GetBoundingBox();

        for( auto itB = itA + 1; itB != footprints.end(); itB++ )
        {
            FOOTPRINT*            fpB = *itB;
            const SHAPE_POLY_SET& frontB = fpB->GetCourtyard( F_CrtYd );
            const SHAPE_POLY_SET& backB = fpB->GetCourtyard( B_CrtYd );

            if( frontB.OutlineCount() == 0 && backB.OutlineCount() == 0
                 && m_drcEngine->IsErrorLimitExceeded( DRCE_PTH_IN_COURTYARD )
                 && m_drcEngine->IsErrorLimitExceeded( DRCE_NPTH_IN_COURTYARD ) )
            {
                // No courtyards defined and no hole testing against other footprint's courtyards
                continue;
            }

            BOX2I frontB_worstCaseBBox = frontB.BBoxFromCaches();
            BOX2I backB_worstCaseBBox = backB.BBoxFromCaches();

            frontB_worstCaseBBox.Inflate( m_largestCourtyardClearance );
            backB_worstCaseBBox.Inflate( m_largestCourtyardClearance );

            BOX2I          fpB_bbox = fpB->GetBoundingBox();
            DRC_CONSTRAINT constraint;
            int            clearance;
            int            actual;
            VECTOR2I       pos;

            // Check courtyard-to-courtyard collisions on front of board,
            // if DRCE_OVERLAPPING_FOOTPRINTS is not diasbled
            if( frontA.OutlineCount() > 0 && frontB.OutlineCount() > 0
                    && frontA_worstCaseBBox.Intersects( frontB.BBoxFromCaches() )
                    && !m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS ) )
            {
                constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, F_Cu );
                clearance = constraint.GetValue().Min();

                if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                {
                    if( frontA.Collide( &frontB, clearance, &actual, &pos ) )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );

                        if( clearance > 0 )
                        {
                            drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                                constraint.GetName(),
                                                                clearance,
                                                                actual ) );
                        }

                        drcItem->SetViolatingRule( constraint.GetParentRule() );
                        drcItem->SetItems( fpA, fpB );
                        reportTwoShapeGeometry( drcItem, pos, &frontA, &frontB, F_CrtYd, actual );
                    }
                }
            }

            // Check courtyard-to-courtyard collisions on back of board,
            // if DRCE_OVERLAPPING_FOOTPRINTS is not disabled
            if( backA.OutlineCount() > 0 && backB.OutlineCount() > 0
                    && backA_worstCaseBBox.Intersects( backB.BBoxFromCaches() )
                    && !m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS ) )
            {
                constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
                clearance = constraint.GetValue().Min();

                if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                {
                    if( backA.Collide( &backB, clearance, &actual, &pos ) )
                    {
                        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );

                        if( clearance > 0 )
                        {
                            drcItem->SetErrorDetail( formatMsg( _( "(%s clearance %s; actual %s)" ),
                                                                constraint.GetName(),
                                                                clearance,
                                                                actual ) );
                        }

                        drcItem->SetViolatingRule( constraint.GetParentRule() );
                        drcItem->SetItems( fpA, fpB );
                        reportTwoShapeGeometry( drcItem, pos, &backA, &backB, B_CrtYd, actual );
                    }
                }
            }

            //
            // Check pad-hole-to-courtyard collisions on front and back of board.
            //
            // NB: via holes are not checked.  There is a presumption that a physical object goes
            // through a pad hole, which is not the case for via holes.
            //
            bool checkFront = false;
            bool checkBack = false;

            constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, F_Cu );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                checkFront = true;

            constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpB, fpA, F_Cu );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                checkFront = true;

            constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpA, fpB, B_Cu );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                checkBack = true;

            constraint = m_drcEngine->EvalRules( COURTYARD_CLEARANCE_CONSTRAINT, fpB, fpA, B_Cu );
            clearance = constraint.GetValue().Min();

            if( constraint.GetSeverity() != RPT_SEVERITY_IGNORE && clearance >= 0 )
                checkBack = true;

            auto testPadAgainstCourtyards =
                    [&]( const PAD* pad, const FOOTPRINT* fp )
                    {
                        int errorCode = 0;

                        if( pad->GetProperty() == PAD_PROP::HEATSINK )
                            return;
                        else if( pad->GetAttribute() == PAD_ATTRIB::PTH )
                            errorCode = DRCE_PTH_IN_COURTYARD;
                        else if( pad->GetAttribute() == PAD_ATTRIB::NPTH )
                            errorCode = DRCE_NPTH_IN_COURTYARD;
                        else
                            return;

                        if( m_drcEngine->IsErrorLimitExceeded( errorCode ) )
                            return;

                        if( pad->HasHole() )
                        {
                            std::shared_ptr<SHAPE_SEGMENT> hole = pad->GetEffectiveHoleShape();
                            const SHAPE_POLY_SET&          front = fp->GetCourtyard( F_CrtYd );
                            const SHAPE_POLY_SET&          back = fp->GetCourtyard( B_CrtYd );

                            if( checkFront && front.OutlineCount() > 0 && front.Collide( hole.get(), 0 ) )
                            {
                                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( errorCode );
                                drce->SetItems( pad, fp );
                                reportViolation( drce, pad->GetPosition(), F_CrtYd );
                            }
                            else if( checkBack && back.OutlineCount() > 0 && back.Collide( hole.get(), 0 ) )
                            {
                                std::shared_ptr<DRC_ITEM> drce = DRC_ITEM::Create( errorCode );
                                drce->SetItems( pad, fp );
                                reportViolation( drce, pad->GetPosition(), B_CrtYd );
                            }
                        }
                    };

            if( ( frontA.OutlineCount() > 0 && frontA_worstCaseBBox.Intersects( fpB_bbox ) )
                || ( backA.OutlineCount() > 0 && backA_worstCaseBBox.Intersects( fpB_bbox ) ) )
            {
                for( const PAD* padB : fpB->Pads() )
                    testPadAgainstCourtyards( padB, fpA );
            }

            if( ( frontB.OutlineCount() > 0 && frontB.BBoxFromCaches().Intersects( fpA_bbox ) )
                || ( backB.OutlineCount() > 0 && backB.BBoxFromCaches().Intersects( fpA_bbox ) ) )
            {
                for( const PAD* padA : fpA->Pads() )
                    testPadAgainstCourtyards( padA, fpB );
            }

            if( m_drcEngine->IsCancelled() )
                return false;
        }
    }

    return !m_drcEngine->IsCancelled();
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();
    DRC_CONSTRAINT constraint;

    if( m_drcEngine->QueryWorstConstraint( COURTYARD_CLEARANCE_CONSTRAINT, constraint ) )
        m_largestCourtyardClearance = constraint.GetValue().Min();

    if( !testFootprintCourtyardDefinitions() )
        return false;

   if( !testCourtyardClearances() )
       return false;

    return true;
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COURTYARD_CLEARANCE> dummy;
}
