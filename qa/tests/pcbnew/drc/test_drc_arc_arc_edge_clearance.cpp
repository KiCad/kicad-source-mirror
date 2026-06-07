/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software, you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 2 of the License, or (at your
 * option) any later version.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


/*
 * Regression test for arc vs arc edge clearance.
 *
 * SHAPE_ARC::NearestPoints handles the case where two arc circles intersect or
 * sit externally to each other, but skips the case where one arc's full circle
 * sits inside the other's. In that internal containment case the closest pair
 * of points on the two circles lies along the same ray from C1 through C2,
 * outward past C2, not on the segment between centers. CIRCLE::NearestPoint
 * returns the wrong side and the function falls back to endpoint vs arc
 * distances, which can easily exceed the configured clearance even when the
 * arcs are essentially touching.
 *
 * The fixture has a curved track on F.Cu whose full circle contains an
 * Edge.Cuts arc cutout. The track conductor edge is essentially touching the
 * cutout boundary. Edge clearance is set to 0.2 mm so the buggy reported
 * distance (~0.35 mm) is above the threshold and DRC stays silent. Once
 * NearestPoints handles the internal containment case, the actual gap
 * (~0 mm) drops the reported distance below 0.2 mm and DRC flags the
 * violation.
 *
 * Do not raise the edge clearance to the default 0.5 mm. At 0.5 mm the
 * buggy reported distance falls under the threshold by accident and DRC
 * appears to work, hiding the bug.
 */


struct DRC_ARC_ARC_EDGE_CLEARANCE_FIXTURE
{
    DRC_ARC_ARC_EDGE_CLEARANCE_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCArcArcEdgeClearance_InternalContainment, DRC_ARC_ARC_EDGE_CLEARANCE_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "drc_arc_arc_edge_clearance/drc_arc_arc_edge_clearance", m_board );

    std::vector<DRC_ITEM>  edgeViolations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    for( int code = DRCE_FIRST; code <= DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_EDGE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_EDGE_CLEARANCE )
                    edgeViolations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_CHECK_GE( edgeViolations.size(), 1u );

    if( edgeViolations.empty() )
        return;

    const KIID trackUuid( "e918352c-937f-4a75-aac0-7856db5d052e" );
    const KIID cutoutLeftArcUuid( "b9bfe430-49e3-430b-b86f-c88b7c1be60e" );

    bool trackHit = false;
    bool cutoutHit = false;

    for( const DRC_ITEM& item : edgeViolations )
    {
        for( const KIID& uuid : { item.GetMainItemID(), item.GetAuxItemID() } )
        {
            if( uuid == trackUuid )
                trackHit = true;
            else if( uuid == cutoutLeftArcUuid )
                cutoutHit = true;
        }
    }

    BOOST_CHECK_MESSAGE( trackHit && cutoutHit, "Expected at least one DRCE_EDGE_CLEARANCE violation between the "
                                                "F.Cu track arc and the Edge.Cuts left cutout arc" );
}
