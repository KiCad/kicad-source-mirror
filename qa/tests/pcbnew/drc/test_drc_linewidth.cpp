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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <drc/drc_rule.h>
#include <drc/drc_rule_parser.h>
#include <pcb_marker.h>
#include <pcb_shape.h>
#include <settings/settings_manager.h>


namespace
{
struct DRC_LINE_WIDTH_FIXTURE
{
    ~DRC_LINE_WIDTH_FIXTURE()
    {
        if( m_board && m_board->GetDesignSettings().m_DRCEngine )
            m_board->GetDesignSettings().m_DRCEngine->ClearViolationHandler();

        if( m_board )
            m_board->SetProject( nullptr );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};
} // namespace


BOOST_FIXTURE_TEST_CASE( DRCLineWidth, DRC_LINE_WIDTH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "test_drc_linewidth", m_board );
    BOOST_REQUIRE( m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // The project is at schema 2, so min_text_thickness must survive the rename to line width
    BOOST_CHECK_EQUAL( bds.m_MinSilkLineWidth, pcbIUScale.mmToIU( 0.2 ) );

    for( int ii = DRCE_FIRST; ii <= DRCE_LAST; ++ii )
        bds.m_DRCSeverities[ii] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCSeverities[DRCE_LINE_WIDTH] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<int> codes;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I& aPos, int aLayer,
                 const std::function<void( PCB_MARKER* )>& aPathGenerator )
            {
                codes.push_back( aItem->GetErrorCode() );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    // One copper circle over the 1mm max, four unfilled silk shapes and one silk text under 0.2mm
    BOOST_CHECK_EQUAL( codes.size(), 6 );

    for( int code : codes )
        BOOST_CHECK_EQUAL( code, DRCE_LINE_WIDTH );
}


BOOST_AUTO_TEST_CASE( DRCLegacyTextThickness )
{
    std::vector<std::shared_ptr<DRC_RULE>> rules;
    DRC_RULES_PARSER parser( wxT( "(version 1)(rule old (constraint text_thickness (min 0.2mm)))" ),
                             wxT( "legacy rules" ) );

    parser.Parse( rules, nullptr );

    BOOST_REQUIRE_EQUAL( rules.size(), 1 );
    BOOST_REQUIRE_EQUAL( rules[0]->m_Constraints.size(), 1 );
    BOOST_CHECK_EQUAL( rules[0]->m_Constraints[0].m_Type, LINE_WIDTH_CONSTRAINT );

    // Exclusions saved before the rename name the old settings key
    std::shared_ptr<DRC_ITEM> item = DRC_ITEM::Create( wxT( "text_thickness" ) );

    BOOST_REQUIRE( item );
    BOOST_CHECK_EQUAL( item->GetErrorCode(), DRCE_LINE_WIDTH );
}


BOOST_FIXTURE_TEST_CASE( DRCLineWidthShowMatches, DRC_LINE_WIDTH_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "test_drc_linewidth", m_board );
    BOOST_REQUIRE( m_board );

    auto rule = std::make_shared<DRC_RULE>( wxT( "widths" ) );
    DRC_CONSTRAINT constraint( LINE_WIDTH_CONSTRAINT );
    rule->AddConstraint( constraint );

    int shapes = 0;

    for( BOARD_ITEM* item : m_board->GetDesignSettings().m_DRCEngine->GetItemsMatchingRule( rule, nullptr ) )
    {
        if( item->Type() == PCB_SHAPE_T )
        {
            BOOST_CHECK( !static_cast<PCB_SHAPE*>( item )->IsSolidFill() );
            shapes++;
        }
    }

    // Fifteen graphic shapes, two of them solid filled
    BOOST_CHECK_EQUAL( shapes, 13 );
}
