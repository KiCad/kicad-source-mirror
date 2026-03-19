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
 * @file test_drc_creepage_issue23389.cpp
 *
 * Regression test for issue #23389: creepage DRC must treat NPTH slots as board edges.
 *
 * The board has multiple 'Sitove' netclass pads around a 5mm x 1mm NPTH oval slot with
 * a 5mm creepage rule. Correct slot modeling (arcs + segments) produces 4 violations,
 * two with actual distances above 4mm. With the old oversized-circle representation
 * (radius 2.5mm instead of 0.5mm), those paths exceeded 5mm and were not reported.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_CREEPAGE_NPTH_SLOT_FIXTURE
{
    DRC_CREEPAGE_NPTH_SLOT_FIXTURE() = default;

    ~DRC_CREEPAGE_NPTH_SLOT_FIXTURE()
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


BOOST_FIXTURE_TEST_CASE( CreepageNPTHSlotIssue23389, DRC_CREEPAGE_NPTH_SLOT_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23389/issue23389", m_board );

    BOOST_REQUIRE_MESSAGE( m_board, "Failed to load board issue23389" );

    std::vector<std::shared_ptr<DRC_ITEM>> violations;
    BOARD_DESIGN_SETTINGS&                 bds = m_board->GetDesignSettings();

    BOOST_REQUIRE_MESSAGE( bds.m_DRCEngine, "DRC engine not initialized" );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_CREEPAGE] = SEVERITY::RPT_SEVERITY_ERROR;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                    violations.push_back( aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_TEST_MESSAGE( wxString::Format( "Found %d creepage violations", (int) violations.size() ) );

    double maxActual = 0.0;

    for( const auto& v : violations )
    {
        wxString msg = v->GetErrorMessage( false );
        BOOST_TEST_MESSAGE( wxString::Format( "  Violation: %s", msg ) );

        // Extract the reported actual distance from "actual X.XXXX mm"
        int pos = msg.Find( "actual " );

        if( pos != wxNOT_FOUND )
        {
            double   val = 0.0;
            wxString tail = msg.Mid( pos + 7 );
            int      spacePos = tail.Find( ' ' );

            if( spacePos != wxNOT_FOUND )
                tail = tail.Left( spacePos );

            if( tail.ToDouble( &val ) )
                maxActual = std::max( maxActual, val );
        }
    }

    // The board has multiple 'Sitove' netclass pad pairs around a 5mm x 1mm NPTH oval slot.
    // With correct slot modeling (arcs + segments), creepage paths must go around the actual
    // slot boundary. This produces 4 violations, two of which have actual distances above 4mm.
    // If the slot were modeled as an oversized circle (old bug, radius=2.5mm instead of 0.5mm),
    // those longer paths would exceed 5mm and not be reported, dropping the count to 2.
    BOOST_CHECK_EQUAL( violations.size(), 4u );

    // At least one violation must have a measured distance above 4mm, proving the path
    // correctly traverses the slot boundary rather than cutting through the slot interior
    // or detouring around an oversized circle.
    BOOST_CHECK_GT( maxActual, 4.0 );
}
