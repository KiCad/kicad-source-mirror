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
 * Group members in the netlist are instance paths. Two instances of the same sheet file
 * contain the same symbol uuids, only the path tells them apart, so path matching must
 * assign each instance to its own group.
 */

#include <netlist_reader/pcb_netlist.h>
#include <netlist_reader/netlist_reader.h>
#include <richio.h>
#include <lib_id.h>
#include <qa_utils/wx_utils/unit_test_utils.h>


namespace
{

// Two sheet instances of one file, one symbol, one group per instance
const char* TWO_INSTANCE_NETLIST =
        "(export (version \"E\")\n"
        "  (components\n"
        "    (comp (ref \"Q1\")\n"
        "      (value \"BC817\")\n"
        "      (footprint \"TestLib:SOT-23\")\n"
        "      (sheetpath (names \"/Amp 1/\") (tstamps \"/aaaaaaaa-1111-4aaa-8aaa-aaaaaaaaaaaa/\"))\n"
        "      (tstamps \"55555555-5555-4555-8555-555555555555\"))\n"
        "    (comp (ref \"Q2\")\n"
        "      (value \"BC817\")\n"
        "      (footprint \"TestLib:SOT-23\")\n"
        "      (sheetpath (names \"/Amp 2/\") (tstamps \"/bbbbbbbb-2222-4bbb-8bbb-bbbbbbbbbbbb/\"))\n"
        "      (tstamps \"55555555-5555-4555-8555-555555555555\")))\n"
        "  (groups\n"
        "    (group (name \"Amp (Amp 1)\")\n"
        "      (uuid \"/aaaaaaaa-1111-4aaa-8aaa-aaaaaaaaaaaa/99999999-9999-4999-8999-999999999999\")\n"
        "      (lib_id \"MyBlocks:Amp\")\n"
        "      (members\n"
        "        (member (uuid \"/aaaaaaaa-1111-4aaa-8aaa-aaaaaaaaaaaa/55555555-5555-4555-8555-555555555555\"))))\n"
        "    (group (name \"Amp (Amp 2)\")\n"
        "      (uuid \"/bbbbbbbb-2222-4bbb-8bbb-bbbbbbbbbbbb/99999999-9999-4999-8999-999999999999\")\n"
        "      (lib_id \"MyBlocks:Amp\")\n"
        "      (members\n"
        "        (member (uuid \"/bbbbbbbb-2222-4bbb-8bbb-bbbbbbbbbbbb/55555555-5555-4555-8555-555555555555\"))))))\n";


void loadNetlist( NETLIST& aNetlist, const char* aText )
{
    try
    {
        // The netlist reader owns and deletes the line reader
        KICAD_NETLIST_READER netlistReader( new STRING_LINE_READER( aText, wxS( "test netlist" ) ), &aNetlist );
        netlistReader.LoadNetlist();
    }
    catch( const IO_ERROR& ioe )
    {
        BOOST_FAIL( "netlist parse failed: " + ioe.What().ToStdString() );
    }
}

} // namespace


BOOST_AUTO_TEST_SUITE( NetlistGroupInstances )


BOOST_AUTO_TEST_CASE( InstancePathMembersResolvePerInstance )
{
    NETLIST netlist;
    loadNetlist( netlist, TWO_INSTANCE_NETLIST );

    BOOST_REQUIRE_EQUAL( netlist.GetCount(), 2 );

    COMPONENT* q1 = netlist.GetComponentByReference( wxS( "Q1" ) );
    COMPONENT* q2 = netlist.GetComponentByReference( wxS( "Q2" ) );

    BOOST_REQUIRE( q1 && q2 );
    BOOST_REQUIRE_MESSAGE( q1->GetGroup(), "Q1 was not assigned to a group" );
    BOOST_REQUIRE_MESSAGE( q2->GetGroup(), "Q2 was not assigned to a group" );

    // Each instance must land in its own group, not both in the last one parsed
    BOOST_CHECK_EQUAL( q1->GetGroup()->name, wxString( wxS( "Amp (Amp 1)" ) ) );
    BOOST_CHECK_EQUAL( q2->GetGroup()->name, wxString( wxS( "Amp (Amp 2)" ) ) );
    BOOST_CHECK( q1->GetGroup() != q2->GetGroup() );
    BOOST_CHECK( q1->GetGroup()->uuid != q2->GetGroup()->uuid );
}


BOOST_AUTO_TEST_CASE( GroupUuidsAreDeterministic )
{
    NETLIST netlistA;
    NETLIST netlistB;
    loadNetlist( netlistA, TWO_INSTANCE_NETLIST );
    loadNetlist( netlistB, TWO_INSTANCE_NETLIST );

    COMPONENT* a1 = netlistA.GetComponentByReference( wxS( "Q1" ) );
    COMPONENT* b1 = netlistB.GetComponentByReference( wxS( "Q1" ) );

    BOOST_REQUIRE( a1 && b1 && a1->GetGroup() && b1->GetGroup() );

    // Update PCB must find the same board group on every run
    BOOST_CHECK( a1->GetGroup()->uuid == b1->GetGroup()->uuid );
}


BOOST_AUTO_TEST_CASE( BareUuidMembersStillResolve )
{
    NETLIST netlist;

    LIB_ID fpid;
    fpid.Parse( wxS( "TestLib:SOT-23" ) );

    KIID symbolUuid;
    netlist.AddComponent(
            new COMPONENT( fpid, wxS( "Q1" ), wxS( "BC817" ), KIID_PATH(), std::vector<KIID>{ symbolUuid } ) );

    KIID_PATH member;
    member.push_back( symbolUuid );

    KIID groupUuid;
    netlist.AddGroup( new NETLIST_GROUP{ wxS( "Amp" ), groupUuid, LIB_ID(), std::vector<KIID_PATH>{ member } } );

    netlist.ApplyGroupMembership();

    COMPONENT* q1 = netlist.GetComponentByReference( wxS( "Q1" ) );

    BOOST_REQUIRE( q1 );
    BOOST_REQUIRE_MESSAGE( q1->GetGroup(), "bare uuid member was not resolved" );
    BOOST_CHECK_EQUAL( q1->GetGroup()->name, wxString( wxS( "Amp" ) ) );
}


BOOST_AUTO_TEST_SUITE_END()
