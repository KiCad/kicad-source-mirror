/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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

/**
 * @file test_drc_invalid_outline_issue24241.cpp
 *
 * Regression test for issue #24241: DRC reports "arc has null or very small
 * size" on Edge.Cuts, but the resulting marker is placed at an arbitrary
 * pair of nearby (and valid) shapes, making the bad arc impossible to locate
 * in the GUI.  The reproduction board contains a single degenerate arc whose
 * start, mid and end points are all identical; the rest of the board outline
 * is valid (rounded rectangle with 25-mil corner radii).
 *
 * Before the fix, the violation was rendered at the centre of an unrelated
 * pair of edges chosen by findClosestOutlineGap, and the linked items did
 * NOT reference the degenerate arc.  After the fix, the marker location
 * matches the degenerate arc's start point, and item A of the violation is
 * the degenerate arc itself.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_shape.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_INVALID_OUTLINE_DEGENERATE_ARC_FIXTURE
{
    DRC_INVALID_OUTLINE_DEGENERATE_ARC_FIXTURE() = default;

    ~DRC_INVALID_OUTLINE_DEGENERATE_ARC_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DegenerateArcOutlineMarkerIssue24241,
                         DRC_INVALID_OUTLINE_DEGENERATE_ARC_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24241/issue24241", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue24241" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    std::vector<VECTOR2I>                  markerPositions;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGenerator*/ )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                {
                    violations.push_back( aItem );
                    markerPositions.push_back( aPos );
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d invalid-outline violations",
                                          (int) violations.size() ) );

    BOOST_REQUIRE_GE( violations.size(), 1u );

    // Locate the degenerate arc directly from the board.  Its three control
    // points should all coincide.
    PCB_SHAPE* degenerateArc = nullptr;

    for( BOARD_ITEM* item : m_board->Drawings() )
    {
        if( item->Type() != PCB_SHAPE_T )
            continue;

        PCB_SHAPE* shape = static_cast<PCB_SHAPE*>( item );

        if( shape->GetShape() != SHAPE_T::ARC || shape->GetLayer() != Edge_Cuts )
            continue;

        if( shape->GetStart() == shape->GetEnd() && shape->GetStart() == shape->GetArcMid() )
        {
            degenerateArc = shape;
            break;
        }
    }

    BOOST_REQUIRE_MESSAGE( degenerateArc,
                           "Reproduction board is expected to contain a degenerate arc" );

    // At least one violation must reference the degenerate arc (either as
    // its primary or auxiliary item) and place the marker at the arc's
    // coordinates so the user can navigate to it.
    bool foundLinkedViolation = false;

    for( size_t ii = 0; ii < violations.size(); ++ii )
    {
        const std::shared_ptr<DRC_ITEM>& v = violations[ii];
        const VECTOR2I&                  pos = markerPositions[ii];

        const bool itemMatches = v->GetMainItemID() == degenerateArc->m_Uuid
                                 || v->GetAuxItemID() == degenerateArc->m_Uuid;

        if( itemMatches && pos == degenerateArc->GetStart() )
        {
            foundLinkedViolation = true;
            break;
        }
    }

    BOOST_CHECK_MESSAGE( foundLinkedViolation,
                         "Expected a violation that references the degenerate arc and "
                         "places the marker at its coordinates so the user can locate it" );
}
