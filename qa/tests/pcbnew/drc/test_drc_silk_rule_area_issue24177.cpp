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
 * The fix skips rule areas in both providers' rtree population.  This test
 * builds a synthetic board with one rule area placed near a silk segment
 * and verifies that DRC reports zero silk-clearance, silk-mask-clearance
 * and solder-mask-bridge violations.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <board_design_settings.h>
#include <pcb_shape.h>
#include <pcb_track.h>
#include <zone.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


struct DRC_SILK_RULE_AREA_FIXTURE
{
    DRC_SILK_RULE_AREA_FIXTURE() = default;

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( RuleAreaDoesNotCauseSilkOrMaskViolationsIssue24177,
                         DRC_SILK_RULE_AREA_FIXTURE )
{
    m_board = std::make_unique<BOARD>();
    m_board->SetCopperLayerCount( 2 );

    // Logical rule area on F.Cu.  Its layer set covers the mask and silk targets
    // that the silk-clearance and solder-mask providers walk; without the fix
    // the providers would insert this zone into their rtrees and report bogus
    // clearance violations against the silk segment below.
    ZONE* ruleArea = new ZONE( m_board.get() );
    ruleArea->SetIsRuleArea( true );
    ruleArea->SetLayerSet( LSET( { F_Cu, F_Mask, F_SilkS } ) );
    ruleArea->AppendCorner( VECTOR2I( 0, 0 ), -1 );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 20 ), 0 ), -1 );
    ruleArea->AppendCorner( VECTOR2I( pcbIUScale.mmToIU( 20 ), pcbIUScale.mmToIU( 20 ) ), -1 );
    ruleArea->AppendCorner( VECTOR2I( 0, pcbIUScale.mmToIU( 20 ) ), -1 );
    m_board->Add( ruleArea );

    // Silk segment placed across the rule-area boundary so any collision-based
    // check using the rule area's effective shape would flag it.
    PCB_SHAPE* silk = new PCB_SHAPE( m_board.get(), SHAPE_T::SEGMENT );
    silk->SetLayer( F_SilkS );
    silk->SetStart( VECTOR2I( pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 10 ) ) );
    silk->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 25 ), pcbIUScale.mmToIU( 10 ) ) );
    silk->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.15 ), LINE_STYLE::SOLID ) );
    m_board->Add( silk );

    // Unrelated copper trace to keep the connectivity graph non-trivial.
    PCB_TRACK* trace = new PCB_TRACK( m_board.get() );
    trace->SetLayer( F_Cu );
    trace->SetStart( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 0 ) ) );
    trace->SetEnd( VECTOR2I( pcbIUScale.mmToIU( 30 ), pcbIUScale.mmToIU( 20 ) ) );
    trace->SetWidth( pcbIUScale.mmToIU( 0.2 ) );
    m_board->Add( trace );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    if( !bds.m_DRCEngine )
        bds.m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &bds );

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
