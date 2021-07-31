/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 201 KiCad Developers, see AUTHORS.txt for contributors.
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

#include <string>

#include <wx/toplevel.h>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>
#include <board.h>
#include <board_design_settings.h>
#include <pad.h>
#include <pcb_track.h>
#include <footprint.h>
#include <drc/drc_item.h>
#include <drc/drc_engine.h>
#include <zone_filler.h>
#include <board_commit.h>
#include <tool/tool_manager.h>
#include <zone_filler_tool.h>
#include <settings/settings_manager.h>

struct DRC_REGRESSION_TEST_FIXTURE
{
    DRC_REGRESSION_TEST_FIXTURE() :
            m_settingsManager( true /* headless */ )
    {
    }

    void loadBoard( const wxString& relPath )
    {
        if( m_board )
        {
            m_board->SetProject( nullptr );
            m_board = nullptr;
        }

        std::string absPath = KI_TEST::GetPcbnewTestDataDir() + relPath.ToStdString();
        wxFileName  projectFile( absPath + ".kicad_pro" );
        std::string boardPath = absPath + ".kicad_pcb";
        wxFileName  rulesFile( absPath + ".kicad_dru" );

        if( projectFile.Exists() )
            m_settingsManager.LoadProject( projectFile.GetFullPath() );

        m_board = KI_TEST::ReadBoardFromFileOrStream( boardPath );

        if( projectFile.Exists() )
            m_board->SetProject( &m_settingsManager.Prj() );

        m_DRCEngine = std::make_shared<DRC_ENGINE>( m_board.get(), &m_board->GetDesignSettings() );

        if( rulesFile.Exists() )
            m_DRCEngine->InitEngine( rulesFile );
        else
            m_DRCEngine->InitEngine( wxFileName() );

        m_board->GetDesignSettings().m_DRCEngine = m_DRCEngine;

        m_toolMgr = std::make_unique<TOOL_MANAGER>();
        m_toolMgr->SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );
    }

    void fillZones( int aFillVersion )
    {
        BOARD_COMMIT       commit( m_toolMgr.get() );
        ZONE_FILLER        filler( m_board.get(), &commit );
        std::vector<ZONE*> toFill;

        m_board->GetDesignSettings().m_ZoneFillVersion = aFillVersion;

        for( ZONE* zone : m_board->Zones() )
            toFill.push_back( zone );

        if( filler.Fill( toFill, false, nullptr ) )
            commit.Push( _( "Fill Zone(s)" ), false, false );
    }

    SETTINGS_MANAGER              m_settingsManager;

    std::unique_ptr<BOARD>        m_board;
    std::unique_ptr<TOOL_MANAGER> m_toolMgr;
    std::shared_ptr<DRC_ENGINE>   m_DRCEngine;
};



constexpr int delta = KiROUND( 0.006 * IU_PER_MM );


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
        loadBoard( relPath );
        fillZones( 6 );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        bds.m_DRCSeverities[ DRCE_INVALID_OUTLINE ] = SEVERITY::RPT_SEVERITY_IGNORE;
        bds.m_DRCSeverities[ DRCE_UNCONNECTED_ITEMS ] = SEVERITY::RPT_SEVERITY_IGNORE;

        m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                {
                    if( bds.GetSeverity( aItem->GetErrorCode() ) == SEVERITY::RPT_SEVERITY_ERROR )
                        violations.push_back( *aItem );
                } );

        m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

        if( violations.empty() )
        {
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
                                                      { "issue7325", 21 },
                                                      { "issue8003", 2 } };

    for( const std::pair<wxString, int>& entry : tests )
    {
        loadBoard( entry.first );
        fillZones( 6 );

        std::vector<DRC_ITEM>  violations;
        BOARD_DESIGN_SETTINGS& bds = m_board->GetDesignSettings();

        m_DRCEngine->SetViolationHandler(
                [&]( const std::shared_ptr<DRC_ITEM>& aItem, wxPoint aPos )
                {
                    violations.push_back( *aItem );
                } );

        m_DRCEngine->RunTests( EDA_UNITS::MILLIMETRES, true, false );

        if( violations.size() == entry.second )
        {
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
