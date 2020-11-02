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
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_test_provider_clearance_base.h>

/*
    Couartyard clearance. Tests for malformed component courtyards and overlapping footprints.
    Generated errors:
    - DRCE_OVERLAPPING_FOOTPRINTS
    - DRCE_MISSING_COURTYARD
    - DRCE_MALFORMED_COURTYARD

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
        return "Tests components' courtyard clearance";
    }

    virtual std::set<DRC_CONSTRAINT_TYPE_T> GetConstraintTypes() const override;

    int GetNumPhases() const override;

private:
    void testFootprintCourtyardDefinitions();

    void testOverlappingComponentCourtyards();
};


void DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testFootprintCourtyardDefinitions()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    // Detects missing (or malformed) footprint courtyards
    if( !reportPhase( _( "Checking footprint courtyard definitions..." ) ) )
        return;

    int ii = 0;

    for( MODULE* footprint : m_board->Modules() )
    {
        if( !reportProgress( ii++, m_board->Modules().size(), delta ) )
            return;

        if( ( footprint->GetFlags() & MALFORMED_COURTYARD ) != 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MALFORMED_COURTYARD) )
                continue;

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MALFORMED_COURTYARD );

            m_msg.Printf( drcItem->GetErrorText() + wxS( " " ) + _( "(not a closed shape)" ) );

            drcItem->SetErrorMessage( m_msg );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetPosition());
        }
        else if( footprint->GetPolyCourtyardFront().OutlineCount() == 0
                && footprint->GetPolyCourtyardBack().OutlineCount() == 0 )
        {
            if( m_drcEngine->IsErrorLimitExceeded( DRCE_MISSING_COURTYARD ) )
                continue;

            std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_MISSING_COURTYARD );
            drcItem->SetItems( footprint );
            reportViolation( drcItem, footprint->GetPosition());
        }
        else
        {
            footprint->GetPolyCourtyardFront().BuildBBoxCaches();
            footprint->GetPolyCourtyardBack().BuildBBoxCaches();
        }
    }
}


void DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::testOverlappingComponentCourtyards()
{
    const int delta = 100;  // This is the number of tests between 2 calls to the progress bar

    if( !reportPhase( _( "Checking footprints for overlapping courtyards..." ) ) )
        return;

    int ii = 0;

    for( auto it1 = m_board->Modules().begin(); it1 != m_board->Modules().end(); it1++ )
    {
        if( !reportProgress( ii++, m_board->Modules().size(), delta ) )
            break;

        if( m_drcEngine->IsErrorLimitExceeded( DRCE_OVERLAPPING_FOOTPRINTS) )
            break;

        MODULE*         footprint = *it1;
        SHAPE_POLY_SET& footprintFront = footprint->GetPolyCourtyardFront();
        SHAPE_POLY_SET& footprintBack = footprint->GetPolyCourtyardBack();

        if( footprintFront.OutlineCount() == 0 && footprintBack.OutlineCount() == 0 )
            continue; // No courtyards defined

        for( auto it2 = it1 + 1; it2 != m_board->Modules().end(); it2++ )
        {
            MODULE*         test = *it2;
            SHAPE_POLY_SET& testFront = test->GetPolyCourtyardFront();
            SHAPE_POLY_SET& testBack = test->GetPolyCourtyardBack();
            SHAPE_POLY_SET  intersection;
            bool            overlap = false;
            wxPoint         pos;

            if( footprintFront.OutlineCount() > 0 && testFront.OutlineCount() > 0
                && footprintFront.BBoxFromCaches().Intersects( testFront.BBoxFromCaches() ) )
            {
                intersection.RemoveAllContours();
                intersection.Append( footprintFront );

                // Build the common area between footprint and the test:
                intersection.BooleanIntersection( testFront, SHAPE_POLY_SET::PM_FAST );

                // If the intersection exists then they overlap
                if( intersection.OutlineCount() > 0 )
                {
                    overlap = true;
                    pos = (wxPoint) intersection.CVertex( 0, 0, -1 );
                }
            }

            if( footprintBack.OutlineCount() > 0 && testBack.OutlineCount() > 0
                && footprintBack.BBoxFromCaches().Intersects( testBack.BBoxFromCaches() ) )
            {
                intersection.RemoveAllContours();
                intersection.Append( footprintBack );

                intersection.BooleanIntersection( testBack, SHAPE_POLY_SET::PM_FAST );

                if( intersection.OutlineCount() > 0 )
                {
                    overlap = true;
                    pos = (wxPoint) intersection.CVertex( 0, 0, -1 );
                }
            }

            if( overlap )
            {
                std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_OVERLAPPING_FOOTPRINTS );
                drcItem->SetItems( footprint, test );
                reportViolation( drcItem, pos );
            }
        }
    }
}


bool DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::Run()
{
    m_board = m_drcEngine->GetBoard();

    // fixme: don't use polygon intersection but distance for clearance tests
    //m_largestClearance = 0;
    //ReportAux( "Worst courtyard clearance : %d nm", m_largestClearance );

    testFootprintCourtyardDefinitions();

    testOverlappingComponentCourtyards();

    return true;
}


int DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::GetNumPhases() const
{
    return 2;
}


std::set<DRC_CONSTRAINT_TYPE_T> DRC_TEST_PROVIDER_COURTYARD_CLEARANCE::GetConstraintTypes() const
{
    return { COURTYARD_CLEARANCE_CONSTRAINT };
}


namespace detail
{
    static DRC_REGISTER_TEST_PROVIDER<DRC_TEST_PROVIDER_COURTYARD_CLEARANCE> dummy;
}