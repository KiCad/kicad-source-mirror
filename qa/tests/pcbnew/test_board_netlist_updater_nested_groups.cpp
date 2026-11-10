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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Regression test for https://gitlab.com/kicad/code/kicad/-/work_items/24766
 *
 * A nested schematic group must stay nested on the board after Update PCB from Schematic.
 *
 * nested_groups.net was exported by KiCad from qa/data/eeschema/nested_groups.kicad_sch:
 *     kicad-cli sch export netlist --format kicadsexpr
 */

#include <board.h>
#include <footprint.h>
#include <pcb_group.h>
#include <netlist_reader/board_netlist_updater.h>
#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <lib_id.h>
#include <project.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


namespace
{

FOOTPRINT* addFootprint( BOARD* aBoard, const wxString& aRef, const LIB_ID& aFpid )
{
    FOOTPRINT* fp = new FOOTPRINT( aBoard );
    fp->SetReference( aRef );
    fp->SetFPID( aFpid );
    aBoard->Add( fp );
    return fp;
}


KIID addComponent( NETLIST& aNetlist, const wxString& aRef, const LIB_ID& aFpid )
{
    KIID kiid;
    aNetlist.AddComponent( new COMPONENT( aFpid, aRef, aRef, KIID_PATH(), std::vector<KIID>{ kiid } ) );
    return kiid;
}


PCB_GROUP* findGroup( BOARD* aBoard, const KIID& aUuid )
{
    for( PCB_GROUP* group : aBoard->Groups() )
    {
        if( group->m_Uuid == aUuid )
            return group;
    }

    return nullptr;
}


KIID_PATH pathOf( const KIID& aUuid )
{
    KIID_PATH path;
    path.push_back( aUuid );
    return path;
}


PCB_GROUP* findGroupByName( BOARD* aBoard, const wxString& aName )
{
    for( PCB_GROUP* group : aBoard->Groups() )
    {
        if( group->GetName() == aName )
            return group;
    }

    return nullptr;
}


void loadNetlist( NETLIST& aNetlist, const wxString& aFileName )
{
    try
    {
        std::unique_ptr<NETLIST_READER> netlistReader(
                NETLIST_READER::GetNetlistReader( &aNetlist, aFileName, wxEmptyString ) );

        BOOST_REQUIRE_MESSAGE( netlistReader, "no netlist reader for " + aFileName );
        netlistReader->LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        BOOST_FAIL( "netlist load failed: " + ioe.What().ToStdString() );
    }
}

} // namespace


BOOST_AUTO_TEST_SUITE( BoardNetlistUpdaterNestedGroups )


BOOST_AUTO_TEST_CASE( NestedSchematicGroupNestsOnBoard )
{
    // The updater dereferences the board's project, so the board needs one.
    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( "" );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetProject( &settingsManager.Prj() );

    LIB_ID fpid;
    BOOST_REQUIRE_EQUAL( fpid.Parse( wxS( "TestLib:R" ) ), -1 );

    addFootprint( board.get(), wxS( "R1" ), fpid );
    addFootprint( board.get(), wxS( "R2" ), fpid );
    addFootprint( board.get(), wxS( "X3" ), fpid );
    addFootprint( board.get(), wxS( "X4" ), fpid );
    addFootprint( board.get(), wxS( "X5" ), fpid );

    NETLIST netlist;

    KIID r1 = addComponent( netlist, wxS( "R1" ), fpid );
    KIID r2 = addComponent( netlist, wxS( "R2" ), fpid );
    KIID x3 = addComponent( netlist, wxS( "X3" ), fpid );
    KIID x4 = addComponent( netlist, wxS( "X4" ), fpid );
    KIID x5 = addComponent( netlist, wxS( "X5" ), fpid );

    KIID childUuid;
    KIID parentUuid;

    netlist.AddGroup( new NETLIST_GROUP( wxS( "RPI" ), childUuid.AsString(), LIB_ID(),
                                         std::vector<KIID_PATH>{ pathOf( r1 ), pathOf( r2 ) } ) );

    // RPI1 nests RPI: its members include the path RPI is referred to by.
    netlist.AddGroup( new NETLIST_GROUP(
            wxS( "RPI1" ), parentUuid.AsString(), LIB_ID(),
            std::vector<KIID_PATH>{ pathOf( x3 ), pathOf( x4 ), pathOf( x5 ), pathOf( childUuid ) } ) );

    netlist.ApplyGroupMembership();

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( board.get(), nullptr, nullptr, nullptr, nullptr );
    toolMgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    BOARD_NETLIST_UPDATER updater( &toolMgr, board.get() );
    updater.SetIsDryRun( false );
    updater.SetWillDrag( false );
    updater.SetTransferGroups( true );
    updater.SetReplaceFootprints( false );
    updater.SetDeleteUnusedFootprints( false );

    BOOST_REQUIRE( updater.UpdateNetlist( netlist ) );

    PCB_GROUP* parentGroup = findGroup( board.get(), parentUuid );
    PCB_GROUP* childGroup = findGroup( board.get(), childUuid );

    BOOST_REQUIRE_MESSAGE( parentGroup, "parent group RPI1 was not created on the board" );
    BOOST_REQUIRE_MESSAGE( childGroup, "child group RPI was not created on the board" );

    BOOST_CHECK_MESSAGE( childGroup->GetParentGroup() == static_cast<EDA_GROUP*>( parentGroup ),
                         "child group RPI is not nested inside parent group RPI1 "
                         "(nested group membership was lost on update)" );

    BOOST_CHECK_MESSAGE( parentGroup->GetItems().count( childGroup ) == 1,
                         "parent group RPI1 does not list child group RPI as a member" );
}


BOOST_AUTO_TEST_CASE( RootSheetNestedGroupNestsOnBoard )
{
    SETTINGS_MANAGER settingsManager;
    settingsManager.LoadProject( "" );

    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();
    board->SetProject( &settingsManager.Prj() );

    LIB_ID fpid;
    BOOST_REQUIRE_EQUAL( fpid.Parse( wxS( "Resistor_SMD:R_0805_2012Metric" ) ), -1 );

    addFootprint( board.get(), wxS( "R1" ), fpid );
    addFootprint( board.get(), wxS( "R2" ), fpid );
    addFootprint( board.get(), wxS( "R3" ), fpid );

    NETLIST netlist;
    loadNetlist( netlist, wxString::FromUTF8( KI_TEST::GetPcbnewTestDataDir() ) + wxS( "netlists/nested_groups.net" ) );

    BOOST_REQUIRE_EQUAL( netlist.GetCount(), 3 );

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( board.get(), nullptr, nullptr, nullptr, nullptr );
    toolMgr.RegisterTool( new KI_TEST::DUMMY_TOOL() );

    BOARD_NETLIST_UPDATER updater( &toolMgr, board.get() );
    updater.SetIsDryRun( false );
    updater.SetWillDrag( false );
    updater.SetTransferGroups( true );
    updater.SetReplaceFootprints( false );
    updater.SetDeleteUnusedFootprints( false );

    BOOST_REQUIRE( updater.UpdateNetlist( netlist ) );

    BOOST_CHECK_EQUAL( board->Groups().size(), 2u );

    PCB_GROUP* outer = findGroupByName( board.get(), wxS( "OUTER" ) );
    PCB_GROUP* inner = findGroupByName( board.get(), wxS( "INNER" ) );

    BOOST_REQUIRE_MESSAGE( outer, "group OUTER was not created on the board" );
    BOOST_REQUIRE_MESSAGE( inner, "group INNER was not created on the board" );

    BOOST_CHECK_MESSAGE( inner->GetParentGroup() == static_cast<EDA_GROUP*>( outer ),
                         "group INNER is not nested inside group OUTER" );

    BOOST_CHECK_MESSAGE( outer->GetItems().count( inner ) == 1, "group OUTER does not list group INNER as a member" );
}


BOOST_AUTO_TEST_SUITE_END()
