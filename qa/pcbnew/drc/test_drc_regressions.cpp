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
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <drc/drc_item.h>
#include <settings/settings_manager.h>


struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    { }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
};


BOOST_FIXTURE_TEST_CASE( DRCFalsePositiveRegressions, DRC_REGRESSION_TEST_FIXTURE )
{
    // These documents at one time flagged DRC errors that they shouldn't have.

    std::vector<wxString> tests = { "issue4139",
                                    "issue4774",
                                    "issue5978",
                                    "issue5990",
                                    "issue6443",
                                    "issue7567",
                                    "issue7975",
                                    "issue8407" };

    for( const wxString& relPath : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, relPath, m_board );
        KI_TEST::FillZones( m_board.get(), 6 );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCSeverities[ DRCE_INVALID_OUTLINE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_UNCONNECTED_ITEMS ] = SEVERITY::RPT_SEVERITY_IGNORE;

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

        if( violations.empty() )
        {
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC regression: %s, passed", relPath ) );
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

            BOOST_ERROR( wxString::Format( "DRC regression: %s, failed", relPath ) );
        }
    }
}


BOOST_FIXTURE_TEST_CASE( DRCFalseNegativeRegressions, DRC_REGRESSION_TEST_FIXTURE )
{
    // These documents at one time failed to catch DRC errors that they should have.

    std::vector< std::pair<wxString, int> > tests = { { "issue1358", 2 },
                                                      { "issue2528", 1 },
                                                      { "issue5750", 4 },
                                                      { "issue5854", 3 },
                                                      { "issue6879", 6 },
                                                      { "issue6945", 2 },
                                                      { "issue7241", 1 },
                                                      { "issue7267", 4 },
                                                      { "issue7325", 2 },
                                                      { "issue8003", 2 },
                                                      { "issue9081", 2 } };

    for( const std::pair<wxString, int>& entry : tests )
    {
        KI_TEST::LoadBoard( m_settingsManager, entry.first, m_board );
        KI_TEST::FillZones( m_board.get(), 6 );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                {
                    violations.push_back( *aItem );
                } );

        bds.m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

        if( violations.size() == entry.second )
        {
            BOOST_CHECK_EQUAL( 1, 1 );  // quiet "did not check any assertions" warning
            BOOST_TEST_MESSAGE( wxString::Format( "DRC regression: %s, passed", entry.first ) );
        }
        else
        {
            BOOST_CHECK_EQUAL( violations.size(), entry.second );

            std::map<KIID, EDA_ITEM*> itemMap;
            m_board->FillItemMap( itemMap );

            for( const DRC_ITEM& item : violations )
            {
                BOOST_TEST_MESSAGE( item.ShowReport( EDA_UNITS::INCHES, RPT_SEVERITY_ERROR,
                                                     itemMap ) );
            }

            BOOST_ERROR( wxString::Format( "DRC regression: %s, failed", entry.first ) );
        }
    }
}
