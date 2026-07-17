/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 3
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/gpl-3.0.html
 * or you may search the http://www.gnu.org website for the version 3 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <board.h>
#include <footprint.h>
#include <lib_id.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


namespace
{

LIB_ID makeFpid( const wxString& aFull )
{
    LIB_ID id;
    BOOST_REQUIRE_EQUAL( id.Parse( aFull ), -1 );
    return id;
}

KIID_PATH pathOf( const KIID& aUuid )
{
    KIID_PATH path;
    path.push_back( aUuid );
    return path;
}

struct UPDATER_FIXTURE
{
    UPDATER_FIXTURE()
    {
        m_settingsManager.LoadProject( "" );
        m_board = std::make_unique<BOARD>();
        m_board->SetProject( &m_settingsManager.Prj() );

        m_toolMgr.SetEnvironment( m_board.get(), nullptr, nullptr, nullptr, nullptr );
        m_toolMgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );
    }

    FOOTPRINT* addBoardFootprint( const wxString& aRef, const wxString& aFpid )
    {
        FOOTPRINT* fp = new FOOTPRINT( m_board.get() );
        fp->SetReference( aRef );
        fp->SetFPID( makeFpid( aFpid ) );
        m_board->Add( fp );
        return fp;
    }

    SETTINGS_MANAGER       m_settingsManager;
    std::unique_ptr<BOARD> m_board;
    TOOL_MANAGER           m_toolMgr;
};

} // namespace


BOOST_AUTO_TEST_SUITE( BoardNetlistUpdaterFpidChange )


// https://gitlab.com/kicad/code/kicad/-/issues/24960
// The schematic assigned a different footprint to R1. With replace footprints off the
// updater must keep the matched board footprint instead of adding a second R1.
BOOST_FIXTURE_TEST_CASE( RelinkKeepsFootprintOnFpidChange, UPDATER_FIXTURE )
{
    FOOTPRINT* fp = addBoardFootprint( wxS( "R1" ), wxS( "TestLib:R_0603" ) );

    KIID    kiid;
    NETLIST netlist;
    netlist.AddComponent( new COMPONENT( makeFpid( wxS( "TestLib:R_0805" ) ), wxS( "R1" ), wxS( "R1" ), KIID_PATH(),
                                         std::vector<KIID>{ kiid } ) );

    BOARD_NETLIST_UPDATER updater( &m_toolMgr, m_board.get() );
    updater.SetLookupByTimestamp( false );
    updater.SetReplaceFootprints( false );
    updater.SetDeleteUnusedFootprints( false );

    BOOST_REQUIRE( updater.UpdateNetlist( netlist ) );

    BOOST_CHECK_EQUAL( updater.GetErrorCount(), 0 );
    BOOST_CHECK_EQUAL( updater.GetNewFootprintCount(), 0 );
    BOOST_REQUIRE_EQUAL( m_board->Footprints().size(), 1 );

    FOOTPRINT* survivor = m_board->Footprints().front();
    BOOST_CHECK( survivor == fp );
    BOOST_CHECK( survivor->GetFPID() == makeFpid( wxS( "TestLib:R_0603" ) ) );

    // matched footprints get relinked to the symbol
    BOOST_CHECK( survivor->GetPath() == pathOf( kiid ) );
}


// same thing with UUID matching
BOOST_FIXTURE_TEST_CASE( UuidMatchKeepsFootprintOnFpidChange, UPDATER_FIXTURE )
{
    KIID kiid;

    FOOTPRINT* fp = addBoardFootprint( wxS( "R1" ), wxS( "TestLib:R_0603" ) );
    fp->SetPath( pathOf( kiid ) );

    NETLIST netlist;
    netlist.AddComponent( new COMPONENT( makeFpid( wxS( "TestLib:R_0805" ) ), wxS( "R1" ), wxS( "R1" ), KIID_PATH(),
                                         std::vector<KIID>{ kiid } ) );

    BOARD_NETLIST_UPDATER updater( &m_toolMgr, m_board.get() );
    updater.SetLookupByTimestamp( true );
    updater.SetReplaceFootprints( false );
    updater.SetDeleteUnusedFootprints( false );

    BOOST_REQUIRE( updater.UpdateNetlist( netlist ) );

    BOOST_CHECK_EQUAL( updater.GetErrorCount(), 0 );
    BOOST_CHECK_EQUAL( updater.GetNewFootprintCount(), 0 );
    BOOST_REQUIRE_EQUAL( m_board->Footprints().size(), 1 );
    BOOST_CHECK( m_board->Footprints().front() == fp );
    BOOST_CHECK( m_board->Footprints().front()->GetFPID() == makeFpid( wxS( "TestLib:R_0603" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
