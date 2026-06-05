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
#include <board_item.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/24525
 *
 * DRC reported "Text or graphic on Edge.Cuts layer" (DRCE_TEXT_ON_EDGECUTS) for text and
 * dimensions on Edge.Cuts, but silently ignored tables and barcodes even though those also
 * plot geometry onto the board outline.  Reference images are deliberately exempt because
 * they are never plotted to any output.
 *
 * The fixture places one text, one dimension, two tables (one rotated), one barcode and one
 * reference image on Edge.Cuts.  Expected result: 5 violations (text, dimension, 2 tables,
 * barcode) and none from the reference image.  On the unfixed code only 2 fire (text and
 * dimension), so this test fails without the fix.
 */


struct DRC_ISSUE24525_FIXTURE
{
    DRC_ISSUE24525_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCIssue24525_TableAndBarcodeOnEdgeCuts, DRC_ISSUE24525_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue24525/issue24525", m_board );

    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    // Make sure the rule under test is active regardless of what the fixture stored.
    bds.m_DRCSeverities[DRCE_TEXT_ON_EDGECUTS] = SEVERITY::RPT_SEVERITY_ERROR;

    std::vector<DRC_ITEM> violations;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_TEXT_ON_EDGECUTS )
                    violations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    std::map<KIID, EDA_ITEM*> itemMap;
    m_board->FillItemMap( itemMap );

    // Tally the violations by the type of the offending item so the test pins down exactly
    // which item types are caught, not just the total.
    std::map<KICAD_T, int> byType;

    for( const DRC_ITEM& item : violations )
    {
        auto it = itemMap.find( item.GetMainItemID() );

        BOOST_REQUIRE( it != itemMap.end() );

        KICAD_T type = it->second->Type();

        if( BaseType( type ) == PCB_DIMENSION_T )
            type = PCB_DIMENSION_T;

        byType[type]++;
    }

    BOOST_CHECK_EQUAL( byType[PCB_TEXT_T], 1 );
    BOOST_CHECK_EQUAL( byType[PCB_DIMENSION_T], 1 );
    BOOST_CHECK_EQUAL( byType[PCB_TABLE_T], 2 );
    BOOST_CHECK_EQUAL( byType[PCB_BARCODE_T], 1 );

    // The reference image on Edge.Cuts must not be flagged.  It is never plotted, so it
    // cannot corrupt the outline.
    BOOST_CHECK_EQUAL( byType[PCB_REFERENCE_IMAGE_T], 0 );

    BOOST_CHECK_EQUAL( violations.size(), 5 );
}
