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
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <git/text_diff.h>

using KIGIT::TEXT_DIFF_KIND;
using KIGIT::TEXT_DIFF_LINE;

BOOST_AUTO_TEST_SUITE( GitTextDiff )


static wxString render( const std::vector<TEXT_DIFF_LINE>& aLines )
{
    wxString out;

    for( const TEXT_DIFF_LINE& line : aLines )
    {
        switch( line.kind )
        {
        case TEXT_DIFF_KIND::CONTEXT: out << wxS( " " ); break;
        case TEXT_DIFF_KIND::ADDED: out << wxS( "+" ); break;
        case TEXT_DIFF_KIND::REMOVED: out << wxS( "-" ); break;
        }

        out << line.text << wxS( "|" );
    }

    return out;
}


BOOST_AUTO_TEST_CASE( IdenticalFilesAreAllContext )
{
    std::vector<wxString> lines = { wxT( "a" ), wxT( "b" ) };

    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( lines, lines ) ), wxString( wxT( " a| b|" ) ) );
}


BOOST_AUTO_TEST_CASE( ChangedLineIsRemovedThenAdded )
{
    std::vector<wxString> oldLines = { wxT( "a" ), wxT( "old" ), wxT( "c" ) };
    std::vector<wxString> newLines = { wxT( "a" ), wxT( "new" ), wxT( "c" ) };

    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( oldLines, newLines ) ), wxString( wxT( " a|-old|+new| c|" ) ) );
}


BOOST_AUTO_TEST_CASE( InsertionAndDeletionKeepSurroundingContext )
{
    std::vector<wxString> oldLines = { wxT( "a" ), wxT( "b" ), wxT( "c" ) };
    std::vector<wxString> inserted = { wxT( "a" ), wxT( "b" ), wxT( "x" ), wxT( "c" ) };
    std::vector<wxString> removed = { wxT( "a" ), wxT( "c" ) };

    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( oldLines, inserted ) ), wxString( wxT( " a| b|+x| c|" ) ) );
    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( oldLines, removed ) ), wxString( wxT( " a|-b| c|" ) ) );
}


BOOST_AUTO_TEST_CASE( EmptySidesShowWholeFile )
{
    std::vector<wxString> lines = { wxT( "a" ), wxT( "b" ) };

    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( {}, lines ) ), wxString( wxT( "+a|+b|" ) ) );
    BOOST_CHECK_EQUAL( render( KIGIT::BuildTextDiff( lines, {} ) ), wxString( wxT( "-a|-b|" ) ) );
    BOOST_CHECK( KIGIT::BuildTextDiff( {}, {} ).empty() );
}


BOOST_AUTO_TEST_CASE( SplitHandlesLineEndings )
{
    std::vector<wxString> unix = KIGIT::SplitTextLines( "a\nb\n" );
    std::vector<wxString> dos = KIGIT::SplitTextLines( "a\r\nb\r\n" );
    std::vector<wxString> noFinalNewline = KIGIT::SplitTextLines( "a\nb" );

    BOOST_REQUIRE_EQUAL( unix.size(), 2 );
    BOOST_CHECK_EQUAL( unix[0], wxT( "a" ) );
    BOOST_CHECK_EQUAL( unix[1], wxT( "b" ) );

    BOOST_REQUIRE_EQUAL( dos.size(), 2 );
    BOOST_CHECK_EQUAL( dos[0], wxT( "a" ) );
    BOOST_CHECK_EQUAL( dos[1], wxT( "b" ) );

    BOOST_REQUIRE_EQUAL( noFinalNewline.size(), 2 );
    BOOST_CHECK_EQUAL( noFinalNewline[0], wxT( "a" ) );
    BOOST_CHECK_EQUAL( noFinalNewline[1], wxT( "b" ) );

    BOOST_CHECK( KIGIT::SplitTextLines( "" ).empty() );
}


// A tall narrow pair exercises the row-major indexing of the table. A line shared between the
// two sides must be found whichever side is the tall one.
BOOST_AUTO_TEST_CASE( TallNarrowTableFindsSharedLine )
{
    std::vector<wxString> many;
    std::vector<wxString> one = { wxT( "only" ) };

    for( int ii = 0; ii < 500; ii++ )
        many.push_back( ii == 250 ? wxString( wxT( "only" ) ) : wxString::Format( wxT( "line %d" ), ii ) );

    std::vector<TEXT_DIFF_LINE> removed = KIGIT::BuildTextDiff( many, one );
    std::vector<TEXT_DIFF_LINE> added = KIGIT::BuildTextDiff( one, many );

    for( const std::vector<TEXT_DIFF_LINE>& diff : { removed, added } )
    {
        int context = 0;

        for( const TEXT_DIFF_LINE& line : diff )
        {
            if( line.kind == TEXT_DIFF_KIND::CONTEXT )
            {
                context++;
                BOOST_CHECK_EQUAL( line.text, wxString( wxT( "only" ) ) );
            }
        }

        BOOST_CHECK_EQUAL( diff.size(), many.size() );
        BOOST_CHECK_EQUAL( context, 1 );
    }
}


// A shared line in the middle of an otherwise different pair is kept as context.
BOOST_AUTO_TEST_CASE( SharedLineInTheMiddleIsContext )
{
    std::vector<wxString> left = { wxT( "a" ), wxT( "shared" ), wxT( "b" ) };
    std::vector<wxString> right = { wxT( "c" ), wxT( "shared" ), wxT( "d" ) };

    int context = 0;

    for( const TEXT_DIFF_LINE& line : KIGIT::BuildTextDiff( left, right ) )
    {
        if( line.kind == TEXT_DIFF_KIND::CONTEXT )
        {
            context++;
            BOOST_CHECK_EQUAL( line.text, wxString( wxT( "shared" ) ) );
        }
    }

    BOOST_CHECK_EQUAL( context, 1 );
}


BOOST_AUTO_TEST_CASE( BinarySniffFindsNulByte )
{
    BOOST_CHECK( KIGIT::TextLooksBinary( std::string( "PK\x03\x04\0\x06", 6 ) ) );
    BOOST_CHECK( !KIGIT::TextLooksBinary( "plain text\nwith lines\n" ) );
    BOOST_CHECK( !KIGIT::TextLooksBinary( "" ) );

    // Only the head is read, so a NUL past the probe does not make the file binary.
    BOOST_CHECK( !KIGIT::TextLooksBinary( std::string( 9000, 'x' ) + std::string( 1, '\0' ) ) );
}


// Past the cell cap the diff stops looking for shared lines and lists one side then the other.
// 2001 against 2001 is the smallest square pair that exceeds it. The shared line in the middle
// is what tells the two paths apart: the table would report it as context, the fallback cannot.
BOOST_AUTO_TEST_CASE( OversizedPairFallsBackToWholeFileDiff )
{
    std::vector<wxString> oldLines;
    std::vector<wxString> newLines;

    for( int ii = 0; ii < 2001; ii++ )
    {
        oldLines.push_back( ii == 1000 ? wxString( wxT( "shared" ) ) : wxString::Format( wxT( "old %d" ), ii ) );
        newLines.push_back( ii == 1000 ? wxString( wxT( "shared" ) ) : wxString::Format( wxT( "new %d" ), ii ) );
    }

    std::vector<TEXT_DIFF_LINE> diff = KIGIT::BuildTextDiff( oldLines, newLines );

    BOOST_REQUIRE_EQUAL( diff.size(), oldLines.size() + newLines.size() );

    for( size_t ii = 0; ii < oldLines.size(); ii++ )
        BOOST_REQUIRE( diff[ii].kind == TEXT_DIFF_KIND::REMOVED );

    for( size_t ii = oldLines.size(); ii < diff.size(); ii++ )
        BOOST_REQUIRE( diff[ii].kind == TEXT_DIFF_KIND::ADDED );
}


BOOST_AUTO_TEST_SUITE_END()
