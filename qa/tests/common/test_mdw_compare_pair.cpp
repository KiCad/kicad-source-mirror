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

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <dialogs/git/workspace_item_files.h>
#include <mdw/client/mdw_compare_doc.h>

#include <filesystem>
#include <fstream>
#include <sstream>


BOOST_AUTO_TEST_SUITE( MdwComparePairs )


// A file with one version, or one row picked, still has a pair to compare
BOOST_AUTO_TEST_CASE( OneSelectedVersionComparesWithItsPredecessorOrWorkingCopy )
{
    using PAIR = std::pair<long, long>;

    BOOST_CHECK( MdwComparePair( 3, { 1 }, false ) == PAIR( 2, 1 ) );

    // Row 0 is the working copy, so the one version is row 1 and the oldest
    BOOST_CHECK( MdwComparePair( 1, { 1 }, true ) == PAIR( 1, 0 ) );
    BOOST_CHECK( MdwComparePair( 3, { 3 }, true ) == PAIR( 3, 0 ) );
    BOOST_CHECK( MdwComparePair( 3, { 0 }, true ) == PAIR( 1, 0 ) );
    BOOST_CHECK( MdwComparePair( 3, { 1 }, true ) == PAIR( 2, 1 ) );

    BOOST_CHECK( MdwComparePair( 3, { 0, 2 }, false ) == PAIR( 2, 0 ) );
    BOOST_CHECK( MdwComparePair( 3, { 2, 0 }, false ) == PAIR( 2, 0 ) );

    // No predecessor, no working copy, a stray row or a selection that is not one or two rows
    BOOST_CHECK( !MdwComparePair( 3, { 2 }, false ) );
    BOOST_CHECK( !MdwComparePair( 0, { 0 }, true ) );
    BOOST_CHECK( !MdwComparePair( 0, {}, false ) );
    BOOST_CHECK( !MdwComparePair( 3, { 5 }, false ) );
    BOOST_CHECK( !MdwComparePair( 3, { -1 }, false ) );
    BOOST_CHECK( !MdwComparePair( 3, { 0, 5 }, false ) );
    BOOST_CHECK( !MdwComparePair( 3, { 0, 1, 2 }, false ) );
}


// A source that cannot list local changes compares the checkout with the newest release's file
BOOST_AUTO_TEST_CASE( FilesDifferOnlyWhenBothExistWithOtherBytes )
{
    const std::filesystem::path dir =
            std::filesystem::path( KI_TEST::GetTestDataRootDir() ) / "common" / "git" / "mdw_ctrl_board";
    const wxString sch = ( dir / "ctrl-board.kicad_sch" ).string();
    const wxString pro = ( dir / "ctrl-board.kicad_pro" ).string();

    BOOST_CHECK( !MdwFilesDiffer( sch, sch ) );
    BOOST_CHECK( MdwFilesDiffer( sch, pro ) );
    BOOST_CHECK( !MdwFilesDiffer( sch, ( dir / "missing.kicad_sch" ).string() ) );
    BOOST_CHECK( !MdwFilesDiffer( ( dir / "missing.kicad_sch" ).string(), pro ) );
}


// A library member's id is no file name, so its kind comes from the part's file
BOOST_AUTO_TEST_CASE( MemberComparesAsItsLibraryKind )
{
    MDW_COMPARE_DOC symbol =
            MdwCompareDocument( wxS( "passives.kicad_sym#C" ), wxS( "/lib/acme-library/passives.kicad_sym" ) );

    BOOST_CHECK( symbol.kind == KICAD_DIFF::DOC_KIND::SYM_LIB );
    BOOST_CHECK_EQUAL( symbol.fileName, wxS( "passives.kicad_sym" ) );

    MDW_COMPARE_DOC footprint = MdwCompareDocument( wxS( "fp_misc.pretty#TO-92" ),
                                                    wxS( "/lib/acme-library/fp_misc.pretty/TO-92.kicad_mod" ) );

    BOOST_CHECK( footprint.kind == KICAD_DIFF::DOC_KIND::FOOTPRINT );
    BOOST_CHECK_EQUAL( footprint.fileName, wxS( "TO-92.kicad_mod" ) );

    MDW_COMPARE_DOC board = MdwCompareDocument( wxS( "ctrl-board.kicad_pcb" ), wxString() );

    BOOST_CHECK( board.kind == KICAD_DIFF::DOC_KIND::PCB );
    BOOST_CHECK_EQUAL( board.fileName, wxS( "ctrl-board.kicad_pcb" ) );

    // The id alone is no file, so nothing is compared
    BOOST_CHECK( MdwCompareDocument( wxS( "passives.kicad_sym#C" ), wxString() ).kind
                 == KICAD_DIFF::DOC_KIND::UNKNOWN );
}


// A compare of one symbol must not show every other symbol of its library as added
BOOST_AUTO_TEST_CASE( WorkingCopyOfSymbolMemberHoldsOnlyThatSymbol )
{
    const std::filesystem::path file = std::filesystem::path( KI_TEST::GetTestDataRootDir() ) / "common" / "git"
                                       / "acme-library" / "passives.kicad_sym";
    std::ifstream     in( file );
    std::stringstream text;

    text << in.rdbuf();

    BOOST_REQUIRE( !text.str().empty() );

    const std::string member = MdwSymbolMemberText( text.str(), wxS( "C" ) );

    BOOST_CHECK( member.starts_with( "(kicad_symbol_lib" ) );
    BOOST_CHECK( member.find( "(symbol \"C\"" ) != std::string::npos );
    BOOST_CHECK( member.find( "(symbol \"L\"" ) == std::string::npos );
    BOOST_CHECK( member.ends_with( ")\n" ) );
    BOOST_CHECK( MdwSymbolMemberText( text.str(), wxS( "Missing" ) ).empty() );
}


BOOST_AUTO_TEST_SUITE_END()
