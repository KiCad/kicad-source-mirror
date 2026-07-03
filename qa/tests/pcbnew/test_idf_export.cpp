/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
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
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

/*
 * Verifies that Edge_Cuts graphics living inside a footprint are exported as board cutouts in the
 * IDFv3 .emn output. issue5854.kicad_pcb is a DIP-14 footprint carrying an fp_rect on Edge.Cuts (a
 * rectangular board cavity) inside a rectangular board outline; the exporter must emit the board
 * outline as loop 0 and the footprint cutout as loop 1.
 */

#include <algorithm>
#include <cctype>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <pcbnew_utils/board_file_utils.h>
#include <exporters/export_idf.h>
#include <base_units.h>
#include <board.h>
#include <filename_resolver.h>
#include <footprint.h>
#include <geometry/eda_angle.h>
#include <pcb_shape.h>

#include <wx/string.h>


namespace
{

/// Return the BOARD_OUTLINE loops found in an IDFv3 .emn file, keyed by loop index with their
/// point coordinates in IDF units (mm). Loop 0 is the board perimeter; any index greater than 0
/// is a cutout.
std::map<int, std::vector<VECTOR2D>> ReadBoardOutlineLoops( const std::filesystem::path& aEmnPath )
{
    std::ifstream in( aEmnPath );
    BOOST_REQUIRE_MESSAGE( in, "Cannot open exported .emn: " << aEmnPath.string() );

    std::map<int, std::vector<VECTOR2D>> loops;
    std::string                          line;
    bool                                 inSection = false;

    while( std::getline( in, line ) )
    {
        if( line.find( ".BOARD_OUTLINE" ) != std::string::npos )
        {
            inSection = true;
            continue;
        }

        if( line.find( ".END_BOARD_OUTLINE" ) != std::string::npos )
            break;

        if( !inSection )
            continue;

        std::istringstream       ss( line );
        std::vector<std::string> tokens;

        for( std::string tok; ss >> tok; )
            tokens.push_back( tok );

        // A point record is "<loopIndex> <x> <y> <angle>"; skip the owner line and the single-value
        // thickness header, which would otherwise be miscounted as a loop index.
        if( tokens.size() < 4 )
            continue;

        const std::string& first = tokens.front();
        const bool         isLoopIndex =
                std::all_of( first.begin(), first.end(), []( unsigned char c ) { return std::isdigit( c ); } );

        if( isLoopIndex )
        {
            loops[std::stoi( first )].emplace_back( std::stod( tokens[1] ),
                                                    std::stod( tokens[2] ) );
        }
    }

    return loops;
}


/// Check that each expected point (board internal units) appears in @p aLoopPoints (IDF mm,
/// Y mirrored). Coordinate checks make a mirrored or displaced cutout fail, not just a
/// missing one.
void CheckLoopContainsPoints( const std::vector<VECTOR2D>& aLoopPoints,
                              const std::vector<VECTOR2I>& aExpectedIU )
{
    for( const VECTOR2I& expected : aExpectedIU )
    {
        const double expX = expected.x * pcbIUScale.MM_PER_IU;
        const double expY = -expected.y * pcbIUScale.MM_PER_IU;

        const bool found = std::any_of( aLoopPoints.begin(), aLoopPoints.end(),
                                        [&]( const VECTOR2D& pt )
                                        {
                                            return std::abs( pt.x - expX ) < 1e-3
                                                    && std::abs( pt.y - expY ) < 1e-3;
                                        } );

        BOOST_CHECK_MESSAGE( found, "Cutout corner (" << expX << ", " << expY
                                    << ") missing from exported loop" );
    }
}


/// Return the footprint's Edge_Cuts cutout shape, requiring exactly one.
PCB_SHAPE* GetCutoutShape( FOOTPRINT* aFootprint )
{
    PCB_SHAPE* cutout = nullptr;

    for( BOARD_ITEM* item : aFootprint->GraphicalItems() )
    {
        if( item->Type() != PCB_SHAPE_T || item->GetLayer() != Edge_Cuts )
            continue;

        BOOST_REQUIRE( !cutout );
        cutout = static_cast<PCB_SHAPE*>( item );
    }

    BOOST_REQUIRE( cutout );
    return cutout;
}

} // namespace


BOOST_AUTO_TEST_SUITE( IdfExport )


BOOST_AUTO_TEST_CASE( FootprintCutoutIsExported )
{
    const std::string sourceBoard = KI_TEST::GetPcbnewTestDataDir() + "issue5854.kicad_pcb";
    BOOST_REQUIRE_MESSAGE( std::filesystem::exists( sourceBoard ), "Missing test board " << sourceBoard );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( sourceBoard );
    BOOST_REQUIRE( board );

    const std::filesystem::path outDir =
            std::filesystem::temp_directory_path() / "kicad_idf_cutout_test";
    std::filesystem::create_directories( outDir );

    const std::filesystem::path emnPath = outDir / "cutout.emn";
    std::filesystem::remove( emnPath );

    FILENAME_RESOLVER resolver;
    wxString          errorMsg;

    const bool ok = ExportBoardToIDF3( board.get(), wxString::FromUTF8( emnPath.string().c_str() ),
                                       false, 0.0, 0.0, true, true, &resolver, &errorMsg );

    BOOST_REQUIRE_MESSAGE( ok, "IDF export failed: " << errorMsg.ToStdString() );
    BOOST_REQUIRE( std::filesystem::exists( emnPath ) );

    const std::map<int, std::vector<VECTOR2D>> loops = ReadBoardOutlineLoops( emnPath );

    // The board perimeter is loop 0; the footprint cutout must add at least one further loop.
    BOOST_CHECK_MESSAGE( loops.count( 0 ) == 1, "Board outline (loop 0) missing from .emn" );
    BOOST_REQUIRE_MESSAGE( loops.count( 1 ) == 1, "Footprint cutout (loop 1) missing from .emn" );

    // The cutout rect's corners must land where the board says they are (mm, Y mirrored),
    // so a reflected or displaced cutout fails even though a loop 1 exists.
    PCB_SHAPE* cutout = GetCutoutShape( board->Footprints().front() );
    BOOST_REQUIRE( cutout->GetShape() == SHAPE_T::RECTANGLE );

    const VECTOR2I start = cutout->GetStart();
    const VECTOR2I end = cutout->GetEnd();

    CheckLoopContainsPoints( loops.at( 1 ),
                             { start, VECTOR2I( end.x, start.y ), end,
                               VECTOR2I( start.x, end.y ) } );

    std::filesystem::remove_all( outDir );
}


// Rotating the footprint by a non-cardinal angle converts its fp_rect cutout to SHAPE_T::POLY
// (EDA_SHAPE::rotate), exercising the polygon branch of the cutout emitter.
BOOST_AUTO_TEST_CASE( RotatedPolygonCutoutIsExported )
{
    const std::string sourceBoard = KI_TEST::GetPcbnewTestDataDir() + "issue5854.kicad_pcb";
    BOOST_REQUIRE_MESSAGE( std::filesystem::exists( sourceBoard ), "Missing test board " << sourceBoard );

    std::unique_ptr<BOARD> board = KI_TEST::ReadBoardFromFileOrStream( sourceBoard );
    BOOST_REQUIRE( board );
    BOOST_REQUIRE( !board->Footprints().empty() );

    FOOTPRINT* footprint = board->Footprints().front();
    footprint->Rotate( footprint->GetPosition(), EDA_ANGLE( 45.0, DEGREES_T ) );

    const std::filesystem::path outDir =
            std::filesystem::temp_directory_path() / "kicad_idf_poly_cutout_test";
    std::filesystem::create_directories( outDir );

    const std::filesystem::path emnPath = outDir / "poly_cutout.emn";
    std::filesystem::remove( emnPath );

    FILENAME_RESOLVER resolver;
    wxString          errorMsg;

    const bool ok = ExportBoardToIDF3( board.get(), wxString::FromUTF8( emnPath.string().c_str() ),
                                       false, 0.0, 0.0, true, true, &resolver, &errorMsg );

    BOOST_REQUIRE_MESSAGE( ok, "IDF export failed: " << errorMsg.ToStdString() );
    BOOST_REQUIRE( std::filesystem::exists( emnPath ) );

    const std::map<int, std::vector<VECTOR2D>> loops = ReadBoardOutlineLoops( emnPath );

    BOOST_CHECK_MESSAGE( loops.count( 0 ) == 1, "Board outline (loop 0) missing from .emn" );
    BOOST_REQUIRE_MESSAGE( loops.count( 1 ) == 1,
                           "Rotated polygon cutout (loop 1) missing from .emn" );

    // The rotated cutout is now a POLY; its outline corners must appear in the exported loop
    PCB_SHAPE* cutout = GetCutoutShape( footprint );
    BOOST_REQUIRE( cutout->GetShape() == SHAPE_T::POLY );

    std::vector<VECTOR2I> corners;
    const SHAPE_LINE_CHAIN& chain = cutout->GetPolyShape().COutline( 0 );

    for( int ii = 0; ii < chain.PointCount(); ++ii )
        corners.push_back( chain.CPoint( ii ) );

    CheckLoopContainsPoints( loops.at( 1 ), corners );

    std::filesystem::remove_all( outDir );
}


BOOST_AUTO_TEST_SUITE_END()
