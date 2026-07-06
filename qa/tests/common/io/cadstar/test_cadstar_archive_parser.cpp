/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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
 * @file test_cadstar_archive_parser.cpp
 * Test suite for #CADSTAR_ARCHIVE_PARSER
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <common/io/cadstar/cadstar_archive_parser.h>

#include <qa_utils/geometry/geometry.h> // For KI_TEST::IsVecWithinTol
#include <geometry/shape_line_chain.h>
#include <geometry/shape_arc.h> // For SHAPE_ARC::DefaultAccuracyForPCB()
#include <wx/log.h>


BOOST_AUTO_TEST_SUITE( CadstartArchiveParser )


struct VERTEX_APPEND_CASE
{
    std::string                              m_CaseName;
    CADSTAR_ARCHIVE_PARSER::VERTEX           m_VertexToAppend;
    BOX2I                                    m_ExpBBox;
    int                                      m_ExpBBoxError;
};

    std::function<wxPoint( const wxPoint& )> m_CadstarToKicadPointCallback;

using vt=CADSTAR_ARCHIVE_PARSER::VERTEX_TYPE;


static const std::vector<VERTEX_APPEND_CASE> appendToChainCases
{
    {
        "Append a point on x",
        { vt::VT_POINT, { 500000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size:*/ { 500000, 0 } },
        0
    },
    {
        "Append a point on y",
        { vt::VT_POINT, { 0, 500000 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size:*/ { 0, 500000 } },
        0
    },
    {
        "Append a Semicircle (clockwise)",
        { vt::CLOCKWISE_SEMICIRCLE, { 500000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size: */ { 500000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a Semicircle (anticlockwise)",
        { vt::ANTICLOCKWISE_SEMICIRCLE, { 500000, 0 } },
        { /* BBOX Position: */ { 0, -250000 }, /* Size: */ { 500000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a 90 degree Arc (clockwise)",
        { vt::CLOCKWISE_ARC, { 250000, 250000 }, { 250000, 0 } },
        { /* BBOX Position: */ { 0, 0 }, /* Size: */ { 250000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
    {
        "Append a 90 degree Arc (anticlockwise)",
        { vt::ANTICLOCKWISE_ARC, { 250000, -250000 }, { 250000, 0 } },
        { /* BBOX Position: */ { 0, -250000 }, /* Size: */ { 250000, 250000 } },
        int( SHAPE_ARC::DefaultAccuracyForPCB() ) // acceptable error when converting to line segments
    },
};



BOOST_AUTO_TEST_CASE( AppendToChain )
{
    static const std::vector<VECTOR2D> coordinateMultipliers =
    {
      { 0.1,  0.1 },
      { 0.1, -0.1 }, // y inversion
      {  1,    1  },
      {  1,   -1  }, // y inversion
      { 10,   10  },
      { 10,  -10  }  // y inversion
    };


    for( const auto& c : appendToChainCases )
    {
        BOOST_TEST_INFO_SCOPE( c.m_CaseName );

        for( const VECTOR2D& mult : coordinateMultipliers )
        {
            BOOST_TEST_INFO_SCOPE( "Applied scaling x=" << mult.x << " y=" << mult.y );

            SHAPE_LINE_CHAIN chain( { 0, 0 } ); // starting chain contains a point at 0,0

            auto transformCoord =
                [&]( const VECTOR2I& aPt ) -> VECTOR2I
                {
                    int x = double( aPt.x ) * mult.x;
                    int y = double( aPt.y ) * mult.y;
                    return { x, y };
                };

            c.m_VertexToAppend.AppendToChain( &chain, transformCoord, SCH_IU_PER_MM * 0.01 );

            BOX2I expBoxTransformed;
            expBoxTransformed.SetOrigin( transformCoord( c.m_ExpBBox.GetPosition() ) );
            expBoxTransformed.SetSize( transformCoord( c.m_ExpBBox.GetSize() ) );
            expBoxTransformed.Normalize();

            BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                    ( chain.BBox().GetPosition() )( expBoxTransformed.GetPosition() ) ( c.m_ExpBBoxError ) );

            BOOST_CHECK_PREDICATE(
                KI_TEST::IsVecWithinTol<VECTOR2I>,
                    ( chain.BBox().GetSize() )( expBoxTransformed.GetSize() ) ( c.m_ExpBBoxError ) );
        }
    }

}


/**
 * Helper to add "attr0", "attr1", ... attributes to an XNODE,
 * matching the format used by the CADSTAR S-expression parser.
 */
static void AddCadstarAttributes( XNODE* aNode, const std::vector<wxString>& aValues )
{
    for( size_t i = 0; i < aValues.size(); ++i )
    {
        wxString attrName = wxString::Format( wxT( "attr%zu" ), i );
        aNode->AddAttribute( attrName, aValues[i] );
    }

    aNode->AddAttribute( wxT( "numAttributes" ),
                         wxString::Format( wxT( "%zu" ), aValues.size() ) );
}


/**
 * Build a minimal HEADER node tree with a known child node and an unknown child node.
 * Verifies that the unknown node is gracefully skipped (no exception thrown).
 */
BOOST_AUTO_TEST_CASE( UnknownNodeInHeaderDoesNotThrow )
{
    wxLogNull suppress; // Suppress wxLogWarning messages during this test

    XNODE headerNode( wxXML_ELEMENT_NODE, wxT( "HEADER" ) );

    XNODE* formatNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "FORMAT" ) );
    AddCadstarAttributes( formatNode, { wxT( "REGULAR" ), wxT( "1" ), wxT( "21" ) } );
    headerNode.AddChild( formatNode );

    XNODE* unknownNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "FUTURE_FEATURE" ) );
    headerNode.AddChild( unknownNode );

    CADSTAR_ARCHIVE_PARSER::HEADER header;
    CADSTAR_ARCHIVE_PARSER::PARSER_CONTEXT ctx;

    BOOST_CHECK_NO_THROW( header.Parse( &headerNode, &ctx ) );
    BOOST_CHECK_EQUAL( header.Format.Type, wxT( "REGULAR" ) );
}


/**
 * Build a minimal ROUTECODE node tree with an unknown child node.
 * Verifies that the unknown node is skipped and known children still parse correctly.
 */
BOOST_AUTO_TEST_CASE( UnknownNodeInRouteCodeDoesNotThrow )
{
    wxLogNull suppress;

    XNODE routeCodeNode( wxXML_ELEMENT_NODE, wxT( "ROUTECODE" ) );
    AddCadstarAttributes( &routeCodeNode, { wxT( "RC1" ), wxT( "Default" ), wxT( "1000" ) } );

    XNODE* neckNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "NECKWIDTH" ) );
    AddCadstarAttributes( neckNode, { wxT( "500" ) } );
    routeCodeNode.AddChild( neckNode );

    XNODE* unknownNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "NEWFEATURE_V25" ) );
    routeCodeNode.AddChild( unknownNode );

    CADSTAR_ARCHIVE_PARSER::ROUTECODE routecode;
    CADSTAR_ARCHIVE_PARSER::PARSER_CONTEXT ctx;

    BOOST_CHECK_NO_THROW( routecode.Parse( &routeCodeNode, &ctx ) );
    BOOST_CHECK_EQUAL( routecode.ID, wxT( "RC1" ) );
    BOOST_CHECK_EQUAL( routecode.NeckedWidth, 500 );
}


/**
 * Verify that unknown parameters in enum-like parsers (e.g. ParseAlignment)
 * return a sensible default rather than throwing.
 */
BOOST_AUTO_TEST_CASE( UnknownAlignmentReturnsDefault )
{
    wxLogNull suppress;

    XNODE alignNode( wxXML_ELEMENT_NODE, wxT( "ALIGN" ) );
    AddCadstarAttributes( &alignNode, { wxT( "FUTURE_ALIGNMENT" ) } );

    CADSTAR_ARCHIVE_PARSER::ALIGNMENT result;

    BOOST_CHECK_NO_THROW( result = CADSTAR_ARCHIVE_PARSER::ParseAlignment( &alignNode ) );
    BOOST_CHECK_EQUAL( static_cast<int>( result ),
                       static_cast<int>( CADSTAR_ARCHIVE_PARSER::ALIGNMENT::NO_ALIGNMENT ) );
}


/**
 * Verify that unknown readability parameter returns a sensible default.
 */
BOOST_AUTO_TEST_CASE( UnknownReadabilityReturnsDefault )
{
    wxLogNull suppress;

    XNODE readNode( wxXML_ELEMENT_NODE, wxT( "READABILITY" ) );
    AddCadstarAttributes( &readNode, { wxT( "LEFT_TO_RIGHT" ) } );

    CADSTAR_ARCHIVE_PARSER::READABILITY result;

    BOOST_CHECK_NO_THROW( result = CADSTAR_ARCHIVE_PARSER::ParseReadability( &readNode ) );
    BOOST_CHECK_EQUAL( static_cast<int>( result ),
                       static_cast<int>( CADSTAR_ARCHIVE_PARSER::READABILITY::BOTTOM_TO_TOP ) );
}


/**
 * Verify that CheckNoChildNodes and CheckNoNextNodes warn rather than throw
 * when unexpected nodes are present.
 */
BOOST_AUTO_TEST_CASE( CheckNoChildNodesWarnsInsteadOfThrowing )
{
    wxLogNull suppress;

    XNODE parentNode( wxXML_ELEMENT_NODE, wxT( "PARENT" ) );
    XNODE* childNode = new XNODE( wxXML_ELEMENT_NODE, wxT( "UNEXPECTED" ) );
    parentNode.AddChild( childNode );

    BOOST_CHECK_NO_THROW( CADSTAR_ARCHIVE_PARSER::CheckNoChildNodes( &parentNode ) );
}


BOOST_AUTO_TEST_CASE( CheckNoNextNodesWarnsInsteadOfThrowing )
{
    wxLogNull suppress;

    XNODE parentNode( wxXML_ELEMENT_NODE, wxT( "PARENT" ) );
    XNODE* child1 = new XNODE( wxXML_ELEMENT_NODE, wxT( "CHILD1" ) );
    XNODE* child2 = new XNODE( wxXML_ELEMENT_NODE, wxT( "CHILD2" ) );
    parentNode.AddChild( child1 );
    parentNode.AddChild( child2 );

    BOOST_CHECK_NO_THROW( CADSTAR_ARCHIVE_PARSER::CheckNoNextNodes( child1 ) );
}


/**
 * Verify that unknown units parameter returns DESIGN as a default.
 */
BOOST_AUTO_TEST_CASE( UnknownUnitsReturnsDefault )
{
    wxLogNull suppress;

    XNODE unitsNode( wxXML_ELEMENT_NODE, wxT( "UNITS" ) );
    AddCadstarAttributes( &unitsNode, { wxT( "FUTURISTIC_UNIT" ) } );

    CADSTAR_ARCHIVE_PARSER::UNITS result;

    BOOST_CHECK_NO_THROW( result = CADSTAR_ARCHIVE_PARSER::ParseUnits( &unitsNode ) );
    BOOST_CHECK_EQUAL( static_cast<int>( result ),
                       static_cast<int>( CADSTAR_ARCHIVE_PARSER::UNITS::DESIGN ) );
}


/**
 * Verify that unknown pin type returns UNCOMMITTED as a default.
 */
BOOST_AUTO_TEST_CASE( UnknownPinTypeReturnsDefault )
{
    wxLogNull suppress;

    XNODE pinTypeNode( wxXML_ELEMENT_NODE, wxT( "PINTYPE" ) );
    AddCadstarAttributes( &pinTypeNode, { wxT( "FUTURE_PIN_TYPE" ) } );

    CADSTAR_PIN_TYPE result;

    BOOST_CHECK_NO_THROW( result = CADSTAR_ARCHIVE_PARSER::PART::GetPinType( &pinTypeNode ) );
    BOOST_CHECK_EQUAL( static_cast<int>( result ),
                       static_cast<int>( CADSTAR_PIN_TYPE::UNCOMMITTED ) );
}


BOOST_AUTO_TEST_SUITE_END()

