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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <drc/drc_engine.h>
#include <drc/drc_item.h>
#include <footprint.h>
#include <pad.h>
#include <pcb_marker.h>
#include <settings/settings_manager.h>
#include <widgets/report_severity.h>


/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/issues/23469
 *
 * Custom DRC rules that key on A.Reference or B.Reference were silently skipped
 * whenever the matched item was a pad, graphic or other sub-item of a footprint,
 * because the Reference property was only registered on FOOTPRINT.  The custom
 * rule therefore never overrode the implicit board-wide edge clearance and
 * edge-clearance violations were reported against pads that the rule was meant
 * to exempt.
 */


struct DRC_ISSUE23469_FIXTURE
{
    DRC_ISSUE23469_FIXTURE() {}

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCIssue23469_ReferenceReducesEdgeClearance, DRC_ISSUE23469_FIXTURE )
{
    KI_TEST::LoadBoard( m_settingsManager, "issue23469/issue23469", m_board );

    std::vector<DRC_ITEM>  edgeViolations;
    std::vector<DRC_ITEM>  holeViolations;
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

    bds.m_DRCSeverities[DRCE_INVALID_OUTLINE] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_UNCONNECTED_ITEMS] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_COPPER_SLIVER] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_STARVED_THERMAL] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_ISSUES] = SEVERITY::RPT_SEVERITY_IGNORE;
    bds.m_DRCSeverities[DRCE_LIB_FOOTPRINT_MISMATCH] = SEVERITY::RPT_SEVERITY_IGNORE;

    bds.m_DRCEngine->SetViolationHandler(
            [&]( const std::shared_ptr<DRC_ITEM>& aItem, const VECTOR2I&, int,
                 const std::function<void( PCB_MARKER* )>& )
            {
                if( aItem->GetErrorCode() == DRCE_EDGE_CLEARANCE )
                    edgeViolations.push_back( *aItem );
                else if( aItem->GetErrorCode() == DRCE_HOLE_CLEARANCE )
                    holeViolations.push_back( *aItem );
            } );

    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    std::map<KIID, EDA_ITEM*> itemMap;
    m_board->FillItemMap( itemMap );

    // Count violations where at least one (or both) of the affected items belong to a
    // footprint whose reference designator matches aRef.  aRequireBothItems is used to
    // verify rules that key on both A.Reference and B.Reference.
    auto countViolationsForRef =
            [&]( const std::vector<DRC_ITEM>& aViolations, const wxString& aRef, bool aRequireBothItems )
            {
                int count = 0;

                for( const DRC_ITEM& item : aViolations )
                {
                    int hits = 0;

                    for( KIID uuid : { item.GetMainItemID(), item.GetAuxItemID() } )
                    {
                        if( uuid == niluuid )
                            continue;

                        auto it = itemMap.find( uuid );

                        if( it == itemMap.end() )
                            continue;

                        BOARD_ITEM* boardItem = dynamic_cast<BOARD_ITEM*>( it->second );

                        if( !boardItem )
                            continue;

                        FOOTPRINT* fp = boardItem->GetParentFootprint();

                        if( fp && fp->GetReference() == aRef )
                            ++hits;
                    }

                    if( aRequireBothItems ? hits >= 2 : hits > 0 )
                        ++count;
                }

                return count;
            };

    // Primary assertion: the custom rule "(condition "A.Reference == 'J1'")" must reduce
    // the default edge clearance so that J1 pads no longer violate.
    BOOST_CHECK_EQUAL( countViolationsForRef( edgeViolations, wxString( "J1" ), false ), 0 );

    // Secondary assertion: the reproduction rule set also contains a hole_clearance rule
    // that keys on both A.Reference and B.Reference.  Verify the B.Reference side of the
    // fix as well (PAD-PAD hole clearance within J1 would currently never match).
    BOOST_CHECK_EQUAL( countViolationsForRef( holeViolations, wxString( "J1" ), true ), 0 );

    if( countViolationsForRef( edgeViolations, wxString( "J1" ), false ) != 0
        || countViolationsForRef( holeViolations, wxString( "J1" ), true ) != 0 )
    {
        UNITS_PROVIDER unitsProvider( pcbIUScale, EDA_UNITS::MM );

        for( const DRC_ITEM& item : edgeViolations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );

        for( const DRC_ITEM& item : holeViolations )
            BOOST_TEST_MESSAGE( item.ShowReport( &unitsProvider, RPT_SEVERITY_ERROR, itemMap ) );
    }

    // Without the custom rules J1 violates the board defaults, so the clean
    // results above came from the rules matching and not from DRC being silent.
    edgeViolations.clear();
    holeViolations.clear();

    bds.m_DRCEngine->InitEngine( wxFileName() );
    bds.m_DRCEngine->RunTests( EDA_UNITS::MM, true, false );

    BOOST_REQUIRE_GT( countViolationsForRef( edgeViolations, wxString( "J1" ), false ), 0 );
    BOOST_REQUIRE_GT( countViolationsForRef( holeViolations, wxString( "J1" ), true ), 0 );
}
