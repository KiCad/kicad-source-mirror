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
 * @file test_drc_unconnected_save.cpp
 * @brief Test case to verify DRC unconnected item exclusions are saved properly
 */

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pcb_marker.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>
#include <project/project_file.h>

struct DRC_UNCONNECTED_SAVE_FIXTURE
{
    DRC_UNCONNECTED_SAVE_FIXTURE() :
            m_settingsManager( true /* headless */ )
    {
        m_board = std::make_unique<BOARD>();
        m_board->SetProject( &m_settingsManager.Prj() );
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCUnconnectedExclusionsSave, DRC_UNCONNECTED_SAVE_FIXTURE )
{
    // Create a mock unconnected items DRC marker
    std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
    drcItem->SetItems( KIID( "12345678-1234-1234-1234-123456789abc" ), KIID( "87654321-4321-4321-4321-cba987654321" ) );

    PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 1000, 1000 ) );
    m_board->Add( marker );

    // Exclude the marker
    marker->SetExcluded( true, "Test exclusion comment" );

    // Record the DRC exclusions (this is what happens during save)
    m_board->RecordDRCExclusions();

    // Verify the exclusion was recorded
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOOST_CHECK( !bds.m_DrcExclusions.empty() );

    // Get the serialized exclusion
    wxString serialized = marker->SerializeToString();
    BOOST_CHECK( bds.m_DrcExclusions.count( serialized ) == 1 );
    BOOST_CHECK( bds.m_DrcExclusionComments[serialized] == "Test exclusion comment" );

    // Simulate saving and reloading
    // Clear the marker but keep the exclusions in design settings
    m_board->DeleteMARKERs();

    // Create a new marker with the same properties
    std::shared_ptr<DRC_ITEM> newDrcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
    newDrcItem->SetItems( KIID( "12345678-1234-1234-1234-123456789abc" ),
                          KIID( "87654321-4321-4321-4321-cba987654321" ) );

    PCB_MARKER* newMarker = new PCB_MARKER( newDrcItem, VECTOR2I( 1000, 1000 ) );
    m_board->Add( newMarker );

    // Resolve exclusions (this is what happens when loading)
    m_board->ResolveDRCExclusions( false );

    // Verify the exclusion was restored
    BOOST_CHECK( newMarker->IsExcluded() );
    BOOST_CHECK( newMarker->GetComment() == "Test exclusion comment" );
}


BOOST_FIXTURE_TEST_CASE( DRCUnconnectedExclusionsMultipleSave, DRC_UNCONNECTED_SAVE_FIXTURE )
{
    // Create multiple unconnected items DRC markers
    for( int i = 0; i < 5; ++i )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
        wxString                  id1 = wxString::Format( "12345678-1234-1234-1234-12345678%04d", i );
        wxString                  id2 = wxString::Format( "87654321-4321-4321-4321-87654321%04d", i );
        drcItem->SetItems( KIID( id1 ), KIID( id2 ) );

        PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 1000 * i, 1000 * i ) );
        m_board->Add( marker );

        // Exclude odd-numbered markers
        if( i % 2 == 1 )
        {
            marker->SetExcluded( true, wxString::Format( "Exclusion %d", i ) );
        }
    }

    // Record the DRC exclusions
    m_board->RecordDRCExclusions();

    // Verify the correct number of exclusions
    BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();
    BOOST_CHECK_EQUAL( bds.m_DrcExclusions.size(), 2 ); // Only odd markers: 1 and 3

    // Clear all markers
    m_board->DeleteMARKERs();

    // Recreate all markers
    for( int i = 0; i < 5; ++i )
    {
        std::shared_ptr<DRC_ITEM> drcItem = DRC_ITEM::Create( DRCE_UNCONNECTED_ITEMS );
        wxString                  id1 = wxString::Format( "12345678-1234-1234-1234-12345678%04d", i );
        wxString                  id2 = wxString::Format( "87654321-4321-4321-4321-87654321%04d", i );
        drcItem->SetItems( KIID( id1 ), KIID( id2 ) );

        PCB_MARKER* marker = new PCB_MARKER( drcItem, VECTOR2I( 1000 * i, 1000 * i ) );
        m_board->Add( marker );
    }

    // Resolve exclusions
    m_board->ResolveDRCExclusions( false );

    // Verify only the correct markers are excluded
    int excludedCount = 0;
    for( PCB_MARKER* marker : m_board->Markers() )
    {
        if( marker->IsExcluded() )
        {
            excludedCount++;
            // Verify it was one of the odd-numbered markers
            BOOST_CHECK( marker->GetComment().Contains( "1" ) || marker->GetComment().Contains( "3" ) );
        }
    }
    BOOST_CHECK_EQUAL( excludedCount, 2 );
}
