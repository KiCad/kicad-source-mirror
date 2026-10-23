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
#include <qa_utils/file_utils.h>

#include <git/text_diff.h>

#include <filesystem>
#include <fstream>

#include <wx/filename.h>

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


// The preview loads both sides on the UI thread, so two sides under the limit can still be too much together
BOOST_AUTO_TEST_CASE( DrawingPreviewLimitCountsBothSides )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-drawing-preview" ) );
    const wxString           before = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "before.kicad_pcb" );
    const wxString           after = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "after.kicad_pcb" );
    const std::string        side( KIGIT::DRAWING_PREVIEW_LIMIT * 6 / 10, 'x' );

    std::ofstream( before.fn_str(), std::ios::binary ) << side;
    std::ofstream( after.fn_str(), std::ios::binary ) << side;

    BOOST_CHECK( !KIGIT::DrawingTooLargeToPreview( before, wxEmptyString, false ) );
    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( before, after, false ) );
}

// A schematic's preview walks its whole hierarchy, so every sheet file it reaches counts, each only once
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitCountsReachableSheetsOnce )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );
    const std::string        sheets = "(kicad_sch\n"
                                      "  (sheet (property \"Sheetfile\" \"big.kicad_sch\"))\n"
                                      "  (sheet (property \"Sheetfile\" \"big.kicad_sch\"))\n"
                                      "  (sheet (property \"Sheetfile\" \"sub/half.kicad_sch\"))\n"
                                      ")\n";

    BOOST_REQUIRE( wxFileName::Mkdir( dir.PathStr() + sep + wxT( "sub" ) ) );
    std::ofstream( root.fn_str(), std::ios::binary ) << sheets;

    std::ofstream( ( dir.PathStr() + sep + wxT( "big.kicad_sch" ) ).fn_str(), std::ios::binary )
            << std::string( KIGIT::DRAWING_PREVIEW_LIMIT * 6 / 10, 'x' );

    std::ofstream( ( dir.PathStr() + sep + wxT( "sub" ) + sep + wxT( "half.kicad_sch" ) ).fn_str(), std::ios::binary )
            << std::string( KIGIT::DRAWING_PREVIEW_LIMIT * 3 / 10, 'x' );

    BOOST_CHECK( !KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, false ) );
    BOOST_CHECK( !KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


namespace
{

void writeFile( const wxString& aPath, const std::string& aText )
{
    std::ofstream( aPath.fn_str(), std::ios::binary ) << aText;
}


std::string sixTenths()
{
    return std::string( KIGIT::DRAWING_PREVIEW_LIMIT * 6 / 10, 'x' );
}

} // namespace


BOOST_AUTO_TEST_CASE( SchematicPreviewLimitReadsTheOlderSheetFileName )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheet file\" \"big.kicad_sch\"))\n)\n" );
    writeFile( dir.PathStr() + sep + wxT( "big.kicad_sch" ), sixTenths() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


// The loader follows a reference out of the side, so the cap cannot tell what it costs
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsAReferenceOutsideItsSideAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "side" ) + sep + wxT( "root.kicad_sch" );

    BOOST_REQUIRE( wxFileName::Mkdir( dir.PathStr() + sep + wxT( "side" ) ) );
    writeFile( dir.PathStr() + sep + wxT( "small.kicad_sch" ), "(kicad_sch)\n" );
    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"../small.kicad_sch\"))\n)\n" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsAnUnresolvableRootAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( dir.PathStr() + wxFileName::GetPathSeparator()
                                                          + wxT( "missing.kicad_sch" ),
                                                  wxEmptyString, true ) );
}


// A file that is not there is not loaded either, as when a version is previewed without its subsheets
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitCountsAMissingSheetAsNothing )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           root = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"absent.kicad_sch\"))\n)\n" );

    BOOST_CHECK( !KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


// The loader collapses ".." before it opens a sheet, so a folder git did not write does not hide the file
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitCountsASheetReachedThroughAnUnwrittenFolder )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n"
                     "  (sheet (property \"Sheetfile\" \"power.kicad_sch\"))\n"
                     "  (sheet (property \"Sheetfile\" \"unused/../power.kicad_sch\"))\n"
                     ")\n" );
    writeFile( dir.PathStr() + sep + wxT( "power.kicad_sch" ), sixTenths() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


// The loader expands the name from project variables the cap cannot see
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsAVariableSheetNameAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           root = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"${SHARED}/power.kicad_sch\"))\n)\n" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


// The scan reads a line at a time, so a name split from its property is doubt
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsASplitSheetNameAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           root = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\"\n    \"${SHARED}/power.kicad_sch\"))\n)\n" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


// The loader reads an unquoted file name, which the scan cannot tell from the next property's value
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsABareSheetNameAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           root = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "root.kicad_sch" );

    writeFile( root,
               "(kicad_sch\n  (sheet (property \"Sheetfile\" big.kicad_sch) (property \"Notes\" \"x\"))\n)\n" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


// A sheet may write its name and file on one line
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitReadsASheetFileAfterAnotherPropertyOnItsLine )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetname\" \"Power\") (property \"Sheetfile\" "
                     "\"big.kicad_sch\"))\n)\n" );
    writeFile( dir.PathStr() + sep + wxT( "big.kicad_sch" ), sixTenths() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


// Only a property's name is a sheet file key, so ordinary text of that spelling is no doubt
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitReadsTextNamedLikeASheetFile )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           root = dir.PathStr() + wxFileName::GetPathSeparator() + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch\n  (text \"Sheetfile\"\n    (at 0 0 0))\n)\n" );

    BOOST_CHECK( !KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


// The loader collapses ".." in a declared top-level sheet even when its path is absolute
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitCountsAnAbsoluteTopLevelSheetThroughAnUnwrittenFolder )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );
    wxString                 other = dir.PathStr() + wxT( "/unused/../other.kicad_sch" );

    other.Replace( wxT( "\\" ), wxT( "/" ) );
    writeFile( root, "(kicad_sch)\n" );
    writeFile( dir.PathStr() + sep + wxT( "other.kicad_sch" ), sixTenths() );
    writeFile( dir.PathStr() + sep + wxT( "root.kicad_pro" ),
               R"({ "schematic": { "top_level_sheets": [ { "filename": "root.kicad_sch" }, { "filename": ")"
                       + other.utf8_string() + R"(" } ] } })" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


BOOST_AUTO_TEST_CASE( SchematicPreviewLimitStartsFromEveryTopLevelSheet )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch)\n" );
    writeFile( dir.PathStr() + sep + wxT( "other.kicad_sch" ), sixTenths() );
    writeFile( dir.PathStr() + sep + wxT( "root.kicad_pro" ),
               R"({ "schematic": { "top_level_sheets": [ { "filename": "root.kicad_sch" },
                                                        { "filename": "other.kicad_sch" } ] } })" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}

BOOST_AUTO_TEST_CASE( SchematicPreviewLimitFollowsParentReferencesInsideTheSideRoot )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "sub" ) + sep + wxT( "root.kicad_sch" );

    BOOST_REQUIRE( wxFileName::Mkdir( dir.PathStr() + sep + wxT( "sub" ) ) );
    BOOST_REQUIRE( wxFileName::Mkdir( dir.PathStr() + sep + wxT( "shared" ) ) );
    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"../shared/big.kicad_sch\"))\n)\n" );
    writeFile( dir.PathStr() + sep + wxT( "shared" ) + sep + wxT( "big.kicad_sch" ), sixTenths() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true, dir.PathStr(), dir.PathStr() ) );
}


BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsAnOversizedProjectAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch)\n" );
    writeFile( dir.PathStr() + sep + wxT( "root.kicad_pro" ),
               std::string( KIGIT::PROJECT_PREVIEW_LIMIT + 1, ' ' ) );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, wxEmptyString, true ) );
}


BOOST_AUTO_TEST_CASE( SchematicPreviewLimitReadsACommentedProject )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           root = dir.PathStr() + sep + wxT( "root.kicad_sch" );

    writeFile( root, "(kicad_sch)\n" );
    writeFile( dir.PathStr() + sep + wxT( "other.kicad_sch" ), sixTenths() );
    writeFile( dir.PathStr() + sep + wxT( "root.kicad_pro" ),
               "// a note\n{ \"schematic\": { \"top_level_sheets\": [ { \"filename\": \"root.kicad_sch\" },\n"
               "                                                    { \"filename\": \"other.kicad_sch\" } ] } }\n" );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true ) );
}


#ifndef _WIN32
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitTreatsALinkOutOfTheSideAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           side = dir.PathStr() + sep + wxT( "side" );
    const wxString           root = side + sep + wxT( "root.kicad_sch" );
    const wxString           small = dir.PathStr() + sep + wxT( "small.kicad_sch" );

    BOOST_REQUIRE( wxFileName::Mkdir( side ) );
    writeFile( small, "(kicad_sch)\n" );
    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"link.kicad_sch\"))\n)\n" );
    std::filesystem::create_symlink( small.utf8_string(), ( side + sep + wxT( "link.kicad_sch" ) ).utf8_string() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true, side, side ) );
}


// The loader looks for a linked sheet's children beside the link, not beside its target
BOOST_AUTO_TEST_CASE( SchematicPreviewLimitFindsALinkedSheetsChildrenBesideTheLink )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-schematic-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           board = dir.PathStr() + sep + wxT( "board" );
    const wxString           shared = dir.PathStr() + sep + wxT( "shared" );
    const wxString           root = board + sep + wxT( "root.kicad_sch" );

    BOOST_REQUIRE( wxFileName::Mkdir( board ) );
    BOOST_REQUIRE( wxFileName::Mkdir( shared ) );
    writeFile( root, "(kicad_sch\n  (sheet (property \"Sheetfile\" \"module.kicad_sch\"))\n)\n" );
    writeFile( shared + sep + wxT( "module.kicad_sch" ),
               "(kicad_sch\n  (sheet (property \"Sheetfile\" \"detail.kicad_sch\"))\n)\n" );
    writeFile( board + sep + wxT( "detail.kicad_sch" ), sixTenths() );
    std::filesystem::create_symlink( ( shared + sep + wxT( "module.kicad_sch" ) ).utf8_string(),
                                     ( board + sep + wxT( "module.kicad_sch" ) ).utf8_string() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( root, root, true, dir.PathStr(), dir.PathStr() ) );
}


BOOST_AUTO_TEST_CASE( DrawingPreviewLimitTreatsABoardLinkedOutOfTheSideAsTooLarge )
{
    KI_TEST::SCOPED_TEMP_DIR dir( wxT( "kicad-drawing-preview" ) );
    const wxString           sep = wxFileName::GetPathSeparator();
    const wxString           side = dir.PathStr() + sep + wxT( "side" );
    const wxString           small = dir.PathStr() + sep + wxT( "small.kicad_pcb" );
    const wxString           link = side + sep + wxT( "link.kicad_pcb" );

    BOOST_REQUIRE( wxFileName::Mkdir( side ) );
    writeFile( small, "(kicad_pcb)\n" );
    std::filesystem::create_symlink( small.utf8_string(), link.utf8_string() );

    BOOST_CHECK( KIGIT::DrawingTooLargeToPreview( link, wxEmptyString, false, side ) );
}
#endif

BOOST_AUTO_TEST_SUITE_END()
