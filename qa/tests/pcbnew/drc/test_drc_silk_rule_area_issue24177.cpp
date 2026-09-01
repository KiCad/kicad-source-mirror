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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_drc_silk_rule_area_issue24177.cpp
 *
 * Regression test for issue #24177: PcbNew 10.99 erroneously reported
 * silk-to-rule-area clearance violations and solder-mask bridges against
 * ZONE objects that are rule areas (logical keepouts/disallow regions).
 *
 * A rule area has GetIsRuleArea() == true and represents a constraint, not
 * physical copper, mask or silk material.  When the silk-clearance and
 * solder-mask DRC providers inserted rule-area zones into their rtrees the
 * resulting collisions produced bogus violations between the rule area and
 * any nearby silk graphic.
 *
 * The fix skips rule areas in both providers' rtree population.
 */

#include <algorithm>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>
#include <zone.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <widgets/report_severity.h>


BOOST_AUTO_TEST_CASE( RuleAreaDoesNotCauseSilkOrMaskViolationsIssue24177 )
{
    const std::string path = KI_TEST::GetPcbnewTestDataDir()
                             + "issue24177/silk_rule_area.kicad_pcb";

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( path );

    BOOST_REQUIRE( board );

    // Zero violations is also what an empty board reports, so pin down the
    // geometry that has to be present for the counts below to mean anything.
    BOOST_REQUIRE_EQUAL( board->Zones().size(), 1u );
    BOOST_REQUIRE( board->Zones().front()->GetIsRuleArea() );

    const auto isSilk =
            []( const BOARD_ITEM* aItem )
            {
                return aItem->Type() == PCB_SHAPE_T && aItem->GetLayer() == F_SilkS;
            };

    BOOST_REQUIRE( std::any_of( board->Drawings().begin(), board->Drawings().end(), isSilk ) );

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    // The QA loader does not wire up the DRC engine the way board_loader.cpp does.
    if( !bds.m_DRCEngine )
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( board.get(), &bds );

    bds.m_DRCEngine->InitEngine( wxFileName() );

    // Focus the run on the providers we changed.
    for( int code = 0; code < DRCE_LAST; ++code )
        bds.m_DRCSeverities[code] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_SILK_CLEARANCE]      = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_SILK_MASK_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_SILK_EDGE_CLEARANCE] = SEVERITY::RPT_SEVERITY_ERROR;
    bds.m_DRCSeverities[DRCE_SOLDERMASK_BRIDGE]   = SEVERITY::RPT_SEVERITY_ERROR;

    int silkClearance     = 0;
    int silkMaskClearance = 0;
    int silkEdgeClearance = 0;
    int solderMaskBridge  = 0;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& /*aPos*/, int /*aLayer*/,
                 const std::function<void( PCB_MARKER* )>& /*aPathGenerator*/ )
            {
                switch( aItem->GetErrorCode() )
                {
                case DRCE_SILK_CLEARANCE:      ++silkClearance;     break;
                case DRCE_SILK_MASK_CLEARANCE: ++silkMaskClearance; break;
                case DRCE_SILK_EDGE_CLEARANCE: ++silkEdgeClearance; break;
                case DRCE_SOLDERMASK_BRIDGE:   ++solderMaskBridge;  break;
                default:                                            break;
                }
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    bds.m_DRCEngine->ClearViolationHandler();

    BOOST_CHECK_EQUAL( silkClearance, 0 );
    BOOST_CHECK_EQUAL( silkMaskClearance, 0 );
    BOOST_CHECK_EQUAL( silkEdgeClearance, 0 );
    BOOST_CHECK_EQUAL( solderMaskBridge, 0 );
}
