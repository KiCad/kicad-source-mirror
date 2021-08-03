/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2021 KiCad Developers, see AUTHORS.txt for contributors.
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
#include <qa/pcbnew/board_test_utils.h>
#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <tracks_cleaner.h>
#include <cleanup_item.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>

struct TRACK_CLEANER_TEST_FIXTURE
{
    TRACK_CLEANER_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


struct TEST_DESCRIPTION
{
    wxString m_File;
    bool     m_Shorts;
    bool     m_RedundantVias;
    bool     m_RedundantTracks;
    bool     m_DanglingTracks;
    bool     m_TracksInPads;
    bool     m_DanglingVias;
    int      m_Expected;
};


BOOST_FIXTURE_TEST_CASE( FailedToCleanRegressionTests, TRACK_CLEANER_TEST_FIXTURE )
{
    /*
     * This one ensures that certain cleanup items are indeed found and marked for cleanup.
     */
    std::vector<TEST_DESCRIPTION> tests =
    {
                     //   short    redundant  redundant  dangling   tracks    dangling
                     //  circuits    vias      tracks     tracks    in pads     vias    expected
        { "issue2904",    false,     false,    false,     true,      false,    false,       6    },
        { "issue5093",    false,     false,    false,     false,     true,     false,     118    },
        { "issue7004",    false,     true,     false,     false,     false,    true,       25    },
        { "issue8883",    true,      true,     true,      true,      false,    true,       80    }
    };

    for( const TEST_DESCRIPTION& entry : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, entry.m_File, m_board );
        KI_TEST::FillZones( m_board.get(), 6 );
        m_board->GetConnectivity()->RecalculateRatsnest();

        TOOL_MANAGER toolMgr;
        toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );

        BOARD_COMMIT                                 commit( &toolMgr );
        TRACKS_CLEANER                               cleaner( m_board.get(), commit );
        std::vector< std::shared_ptr<CLEANUP_ITEM> > dryRunItems;
        std::vector< std::shared_ptr<CLEANUP_ITEM> > realRunItems;

        cleaner.CleanupBoard( true, &dryRunItems, entry.m_Shorts,
                                                  entry.m_RedundantVias,
                                                  entry.m_RedundantTracks,
                                                  entry.m_DanglingTracks,
                                                  entry.m_TracksInPads,
                                                  entry.m_DanglingVias );

        cleaner.CleanupBoard( true, &realRunItems, entry.m_Shorts,
                                                   entry.m_RedundantVias,
                                                   entry.m_RedundantTracks,
                                                   entry.m_DanglingTracks,
                                                   entry.m_TracksInPads,
                                                   entry.m_DanglingVias );

        if( dryRunItems.size() == entry.m_Expected && realRunItems.size() == entry.m_Expected )
        {
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "Track cleaner regression: %s, passed",
                                                  entry.m_File ) );
        }
        else
        {
            BOOST_CHECK_EQUAL( dryRunItems.size(), entry.m_Expected );
            BOOST_CHECK_EQUAL( realRunItems.size(), entry.m_Expected );

            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const std::shared_ptr<CLEANUP_ITEM>& item : realRunItems )
            {
                BOOST_TEST_MESSAGE( item->ShowReport( EDA_UNITS::INCHES, RPT_SEVERITY_ERROR,
                                                      itemMap ) );
            }

            BOOST_ERROR( wxString::Format( "Track cleaner regression: %s, failed",
                                           entry.m_File ) );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( TrackCleanerRegressionTests, TRACK_CLEANER_TEST_FIXTURE )
{
    /*
     * This one just makes sure that the dry-run counts agree with the "real" counts, and that
     * the cleaning doesn't produce any connectivity changes.
     */
    std::vector<wxString> tests = { "issue832",
                                    "issue4257",
                                    "issue8909" };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        KI_TEST::FillZones( m_board.get(), 6 );
        m_board->GetConnectivity()->RecalculateRatsnest();

        TOOL_MANAGER toolMgr;
        toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );

        BOARD_COMMIT                                 commit( &toolMgr );
        TRACKS_CLEANER                               cleaner( m_board.get(), commit );
        std::vector< std::shared_ptr<CLEANUP_ITEM> > dryRunItems;
        std::vector< std::shared_ptr<CLEANUP_ITEM> > realRunItems;

        cleaner.CleanupBoard( true, &dryRunItems, true,   // short circuits
                                                  true,   // redundant vias
                                                  true,   // redundant tracks
                                                  true,   // dangling tracks
                                                  true,   // tracks in pads
                                                  true ); // dangling vias

        cleaner.CleanupBoard( true, &realRunItems, true,   // short circuits
                                                   true,   // redundant vias
                                                   true,   // redundant tracks
                                                   true,   // dangling tracks
                                                   true,   // tracks in pads
                                                   true ); // dangling vias

        BOOST_CHECK_EQUAL( dryRunItems.size(), realRunItems.size() );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                {
                    if( aItem->GetErrorCode() == DRCE_UNCONNECTED_ITEMS )
                        violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

        if( violations.empty() )
        {
            BOOST_TEST_MESSAGE( wxString::Format( "Track cleaner regression: %s, passed",
                                                  relPath ) );
        }
        else
        {
            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
            {
                BOOST_TEST_MESSAGE( item.ShowReport( EDA_UNITS::INCHES, RPT_SEVERITY_ERROR,
                                                     itemMap ) );
            }

            BOOST_ERROR( wxString::Format( "Track cleaner regression: %s, failed",
                                           relPath ) );
        }
    }
}

