/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file test_issue14447_arc_connection.cpp
 *
 * Regression coverage for https://gitlab.com/kicad/code/kicad/-/issues/14447.
 *
 * A CADSTAR schematic CONN PATH may contain ACWARC/CWARC arc vertices. The importer parsed the
 * path with ParseAllChildPoints, which only accepts PT nodes and aborted import with
 * "Unknown node 'ACWARC' in 'PATH'". The connection path is now parsed as vertices so curved
 * connections survive the import. The CONN/PATH fragment exercised here is taken verbatim from
 * the file attached to the issue.
 */

#include <boost/test/unit_test.hpp>
#include <qa_utils/wx_utils/unit_test_utils.h>

#include <memory>

#include <sch_io/cadstar/cadstar_sch_archive_parser.h>


struct ARC_CONNECTION_FIXTURE
{
    std::string GetTestFile()
    {
        return KI_TEST::GetEeschemaTestDataDir() + "io/cadstar/issue14447_arc_connection.csa";
    }
};


BOOST_FIXTURE_TEST_SUITE( CadstarSchArcConnection, ARC_CONNECTION_FIXTURE )


BOOST_AUTO_TEST_CASE( ParseConnectionWithArc )
{
    using PARSER = CADSTAR_SCH_ARCHIVE_PARSER;

    std::unique_ptr<XNODE> root( PARSER::LoadArchiveFile( GetTestFile(), wxT( "CADSTARSCM" ) ) );
    BOOST_REQUIRE( root );

    // Walk down to the single CONN node the fixture file holds.
    XNODE* connNode = nullptr;

    for( XNODE* schematic = root->GetChildren(); schematic && !connNode;
         schematic = schematic->GetNext() )
    {
        if( schematic->GetName() != wxT( "SCHEMATIC" ) )
            continue;

        for( XNODE* net = schematic->GetChildren(); net && !connNode; net = net->GetNext() )
        {
            if( net->GetName() != wxT( "NET" ) )
                continue;

            for( XNODE* child = net->GetChildren(); child; child = child->GetNext() )
            {
                if( child->GetName() == wxT( "CONN" ) )
                {
                    connNode = child;
                    break;
                }
            }
        }
    }

    BOOST_REQUIRE( connNode );

    PARSER::PARSER_CONTEXT context;
    PARSER::NET_SCH::CONNECTION_SCH conn;

    // The whole bug is here. With the path parsed as plain points the ACWARC child made parsing
    // abort with IO_ERROR( "Unknown node 'ACWARC' in 'PATH'" ). It must now parse cleanly.
    BOOST_REQUIRE_NO_THROW( conn.Parse( connNode, &context ) );

    // Two straight points, the arc, and the closing straight point must all be retained.
    BOOST_REQUIRE_EQUAL( conn.Path.size(), 4 );

    const CADSTAR_ARCHIVE_PARSER::VERTEX& arc = conn.Path[2];

    BOOST_CHECK( arc.Type == CADSTAR_ARCHIVE_PARSER::VERTEX_TYPE::ANTICLOCKWISE_ARC );

    // CADSTAR emits (ACWARC (PT centre) (PT end)); the parser keeps the first as the arc centre
    // and the second as its end point.
    BOOST_CHECK_EQUAL( arc.Center.x, 10033000 );
    BOOST_CHECK_EQUAL( arc.Center.y, 8763000 );
    BOOST_CHECK_EQUAL( arc.End.x, 11430000 );
    BOOST_CHECK_EQUAL( arc.End.y, 8763000 );

    // The path must terminate at the final straight point of the connection.
    BOOST_CHECK_EQUAL( conn.Path[3].End.x, 11430000 );
    BOOST_CHECK_EQUAL( conn.Path[3].End.y, 10160000 );
}


BOOST_AUTO_TEST_SUITE_END()
