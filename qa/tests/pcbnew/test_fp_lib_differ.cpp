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

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <pcbnew_utils/board_test_utils.h>
#include <pcbnew_utils/board_file_utils.h>

#include <diff_merge/fp_lib_differ.h>
#include <diff_merge/kicad_diff_types.h>
#include <diff_merge/pcb_geometry_extractor.h>

#include <footprint.h>

#include <nlohmann/json.hpp>

#include <wx/filename.h>


using namespace KICAD_DIFF;


BOOST_AUTO_TEST_SUITE( FpLibDiffer )


static wxString getFixturePath()
{
    // qa/data/pcbnew/.. -> qa/data/libraries/Resistor_SMD.pretty
    wxFileName fn( KI_TEST::GetPcbnewTestDataDir(), wxEmptyString );
    fn.RemoveLastDir();
    fn.AppendDir( wxS( "libraries" ) );
    fn.AppendDir( wxS( "Resistor_SMD.pretty" ) );
    return fn.GetPath();
}


BOOST_AUTO_TEST_CASE( IdenticalLibrariesDiffEmpty )
{
    auto [ownersA, mapA] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !mapA.empty() );

    FP_LIB_DIFFER differ( mapA, mapB, wxS( "Resistor_SMD.pretty" ) );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_CHECK( result.Empty() );
    BOOST_CHECK_EQUAL( result.docType.ToStdString(), "pretty" );
}


BOOST_AUTO_TEST_CASE( FootprintRemovedShowsAsRemoved )
{
    auto [ownersA, mapA] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !mapB.empty() );
    mapB.erase( mapB.begin() );

    FP_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::REMOVED );
}


BOOST_AUTO_TEST_CASE( FootprintChangesCarryDrawableBBoxes )
{
    auto [ownersA, mapA] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !ownersB.empty() );
    FOOTPRINT* subject = ownersB.front().get();
    subject->Move( VECTOR2I( 1000, 0 ) );

    FP_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_REQUIRE_EQUAL( result.changes.size(), 1u );
    BOOST_CHECK( result.changes[0].kind == CHANGE_KIND::MODIFIED );
    BOOST_CHECK_GT( result.changes[0].bbox.GetWidth(), 0 );
    BOOST_CHECK_GT( result.changes[0].bbox.GetHeight(), 0 );
}


BOOST_AUTO_TEST_CASE( ExtractFootprintGeometryProducesLayeredContext )
{
    auto [owners, map] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE( !owners.empty() );

    DOCUMENT_GEOMETRY geometry = ExtractFootprintGeometry( *owners.front(), KIGFX::COLOR4D( 0.38, 0.38, 0.38, 0.55 ) );

    BOOST_CHECK( !geometry.Empty() );
    BOOST_CHECK( GeometryLayerSet( geometry ).any() );
}


BOOST_AUTO_TEST_CASE( DiffJsonRoundTrip )
{
    auto [ownersA, mapA] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    if( !mapB.empty() )
        mapB.erase( mapB.begin() );

    FP_LIB_DIFFER differ( mapA, mapB, wxS( "lib.pretty" ) );
    DOCUMENT_DIFF result = differ.Diff();

    nlohmann::json j = result.ToJson();
    DOCUMENT_DIFF  back = DOCUMENT_DIFF::FromJson( j );
    BOOST_CHECK_EQUAL( back.changes.size(), result.changes.size() );
}


// LibraryItemKiidPath must be deterministic for the same footprint name
// across calls -- differ + applier must agree on the synthetic UUID or
// per-item resolution lookup silently misses.
BOOST_AUTO_TEST_CASE( LibraryItemKiidPathIsDeterministicForSameName )
{
    KIID_PATH a = LibraryItemKiidPath( wxS( "R_0402_1005Metric" ) );
    KIID_PATH b = LibraryItemKiidPath( wxS( "R_0402_1005Metric" ) );

    BOOST_REQUIRE_EQUAL( a.size(), 1u );
    BOOST_REQUIRE_EQUAL( b.size(), 1u );
    BOOST_CHECK( a == b );
}


BOOST_AUTO_TEST_CASE( LibraryItemKiidPathDiffersForDifferentNames )
{
    KIID_PATH a = LibraryItemKiidPath( wxS( "R_0402_1005Metric" ) );
    KIID_PATH b = LibraryItemKiidPath( wxS( "R_0603_1608Metric" ) );

    BOOST_CHECK( !( a == b ) );
}


// Multiple removals: each footprint must surface as its own change record
// rather than being merged.
BOOST_AUTO_TEST_CASE( MultipleFootprintRemovalsAllEmitChanges )
{
    auto [ownersA, mapA] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB, mapB] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE_GE( mapB.size(), 3u );

    std::vector<wxString> victims;

    for( int i = 0; i < 3 && !mapB.empty(); ++i )
    {
        victims.push_back( mapB.begin()->first );
        mapB.erase( mapB.begin() );
    }

    FP_LIB_DIFFER differ( mapA, mapB );
    DOCUMENT_DIFF result = differ.Diff();

    BOOST_CHECK_EQUAL( result.changes.size(), victims.size() );

    for( const ITEM_CHANGE& c : result.changes )
    {
        BOOST_CHECK( c.kind == CHANGE_KIND::REMOVED );
    }
}


// Output ordering must be deterministic for the same input pair so JSON
// diffs are byte-stable across runs.
BOOST_AUTO_TEST_CASE( OutputOrderingIsDeterministicAcrossRuns )
{
    auto [ownersA1, mapA1] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB1, mapB1] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersA2, mapA2] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );
    auto [ownersB2, mapB2] = FP_LIB_DIFFER::LoadLibrary( getFixturePath() );

    BOOST_REQUIRE_GE( mapB1.size(), 2u );

    mapB1.erase( mapB1.begin() );
    mapB2.erase( mapB2.begin() );

    FP_LIB_DIFFER differ1( mapA1, mapB1 );
    FP_LIB_DIFFER differ2( mapA2, mapB2 );

    DOCUMENT_DIFF r1 = differ1.Diff();
    DOCUMENT_DIFF r2 = differ2.Diff();

    BOOST_CHECK_EQUAL( r1.ToJson().dump(), r2.ToJson().dump() );
}


BOOST_AUTO_TEST_SUITE_END()
