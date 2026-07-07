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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

/**
 * @file test_group_save_perf.cpp
 *
 * Regression performance test for
 * https://gitlab.com/kicad/code/kicad/-/issues/24251
 *
 * PCB_IO_KICAD_SEXPR::format(PCB_GROUP*) previously rebuilt a local
 * std::unordered_set<EDA_ITEM*> from the board's full item-by-id cache once per
 * group.  With G groups and N board items the save was O(G * N).  This test
 * builds boards with the same item count but increasing group counts and
 * verifies the save cost scales roughly linearly with the group count rather
 * than quadratically.
 */

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <memory>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <lset.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>


namespace
{

/**
 * Build a board with a fixed number of PCB_SHAPE objects and a configurable
 * number of groups, each of which owns a small slice of those shapes.
 *
 * Total board item count stays the same across runs so any scaling we see
 * comes from the group count, not from item count.
 */
std::unique_ptr<BOARD> buildBoardWithGroups( std::size_t aGroupCount, std::size_t aTotalShapes )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    std::vector<PCB_SHAPE*> shapes;
    shapes.reserve( aTotalShapes );

    for( std::size_t i = 0; i < aTotalShapes; ++i )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
        int        x = static_cast<int>( i );
        shape->SetStart( VECTOR2I( x, 0 ) );
        shape->SetEnd( VECTOR2I( x + pcbIUScale.mmToIU( 1 ), 0 ) );
        shape->SetLayer( F_SilkS );
        shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
        board->Add( shape );
        shapes.push_back( shape );
    }

    if( aGroupCount == 0 )
        return board;

    std::size_t perGroup = std::max<std::size_t>( 1, aTotalShapes / aGroupCount );

    for( std::size_t g = 0; g < aGroupCount; ++g )
    {
        PCB_GROUP* group = new PCB_GROUP( board.get() );
        group->SetName( wxString::Format( wxT( "G%zu" ), g ) );

        std::size_t first = ( g * perGroup ) % aTotalShapes;

        for( std::size_t k = 0; k < perGroup; ++k )
            group->AddItem( shapes[( first + k ) % aTotalShapes] );

        board->Add( group );
    }

    return board;
}


double saveBoardSeconds( BOARD& aBoard )
{
    namespace fs = std::filesystem;
    const fs::path savePath = fs::temp_directory_path() / "qa_group_save_perf.kicad_pcb";

    auto start = std::chrono::steady_clock::now();
    KI_TEST::DumpBoardToFile( aBoard, savePath.string() );
    auto end = std::chrono::steady_clock::now();

    std::error_code ec;
    fs::remove( savePath, ec );

    return std::chrono::duration<double>( end - start ).count();
}

} // namespace


BOOST_AUTO_TEST_SUITE( GroupSavePerformance )


/**
 * Save boards with the same total item count but very different group counts.
 *
 * Pre-fix, the format() of each group rebuilt a pointer set scanning the full
 * item-by-id cache, so doubling the group count more than doubled the save
 * time even though the underlying serialised output grew only linearly.
 *
 * We check that scaling is sub-quadratic with a generous margin so the test is
 * robust against debug builds, CI noise and slow VMs while still failing if
 * the O(N^2) regression returns.
 */
BOOST_AUTO_TEST_CASE( SaveScalesLinearlyWithGroupCount )
{
    constexpr std::size_t kTotalShapes = 4000;

    // Warm up the allocator / disk cache so the first measurement is not
    // dominated by one-shot setup costs.
    {
        auto warm = buildBoardWithGroups( 50, kTotalShapes );
        (void) saveBoardSeconds( *warm );
    }

    auto board100 = buildBoardWithGroups( 100, kTotalShapes );
    auto board1000 = buildBoardWithGroups( 1000, kTotalShapes );
    auto board5000 = buildBoardWithGroups( 5000, kTotalShapes );

    double t100 = saveBoardSeconds( *board100 );
    double t1000 = saveBoardSeconds( *board1000 );
    double t5000 = saveBoardSeconds( *board5000 );

    BOOST_TEST_MESSAGE( "Save time (s)  100 groups : " << t100 );
    BOOST_TEST_MESSAGE( "Save time (s) 1000 groups : " << t1000 );
    BOOST_TEST_MESSAGE( "Save time (s) 5000 groups : " << t5000 );

    // Use a 5 ms floor so very fast runs do not produce huge ratios from
    // measurement jitter.
    const double floorSec = 0.005;
    double       baseline = std::max( t100, floorSec );

    double ratio1000 = t1000 / baseline;
    double ratio5000 = t5000 / baseline;

    BOOST_TEST_MESSAGE( "Ratio (1000 / 100): " << ratio1000 );
    BOOST_TEST_MESSAGE( "Ratio (5000 / 100): " << ratio5000 );

    // Quadratic scaling between 100 and 5000 groups would be ~2500x.  Linear
    // would be ~50x.  Empirically, the unfixed code on a debug build hits ~145x
    // and the fixed code stays under ~5x.  Allow up to 60x to absorb CI noise
    // while still catching the regression with a comfortable margin.
    BOOST_CHECK_LT( ratio5000, 60.0 );

    // 1000 / 100 should be ~10x linearly; allow up to 30x.  Unfixed code hits
    // ~8x here too, but the absolute time difference (60ms vs 1.5s) is large.
    BOOST_CHECK_LT( ratio1000, 30.0 );
}


BOOST_AUTO_TEST_SUITE_END()
