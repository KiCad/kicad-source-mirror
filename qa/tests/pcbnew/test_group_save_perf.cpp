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
 * saves boards with the same shape count but very different group counts and
 * verifies the save cost scales roughly linearly with the group count rather
 * than quadratically.
 */

#include <algorithm>
#include <cstddef>
#include <filesystem>
#include <limits>
#include <memory>
#include <string>
#include <vector>

#include <qa_utils/wx_utils/unit_test_utils.h>

#include <board.h>
#include <core/profile.h>
#include <lset.h>
#include <pcb_group.h>
#include <pcb_shape.h>
#include <pcbnew_utils/board_file_utils.h>
#include <pcbnew_utils/board_test_utils.h>


namespace
{

/**
 * Build a board with a fixed number of PCB_SHAPE objects and a configurable
 * number of groups.
 *
 * Shapes are assigned to groups round-robin so every group owns at least one
 * shape when aGroupCount <= aTotalShapes.  An item can only belong to one
 * group at a time, so a wrapping slice scheme would re-parent shapes and
 * silently empty out earlier groups.
 */
std::unique_ptr<BOARD> buildBoardWithGroups( std::size_t aGroupCount, std::size_t aTotalShapes )
{
    auto board = std::make_unique<BOARD>();
    board->SetEnabledLayers( LSET::AllCuMask() | LSET::AllTechMask() );

    std::vector<PCB_GROUP*> groups;
    groups.reserve( aGroupCount );

    for( std::size_t g = 0; g < aGroupCount; ++g )
    {
        PCB_GROUP* group = new PCB_GROUP( board.get() );
        group->SetName( wxString::Format( wxT( "G%zu" ), g ) );
        board->Add( group );
        groups.push_back( group );
    }

    for( std::size_t i = 0; i < aTotalShapes; ++i )
    {
        PCB_SHAPE* shape = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
        int        x = static_cast<int>( i );
        shape->SetStart( VECTOR2I( x, 0 ) );
        shape->SetEnd( VECTOR2I( x + pcbIUScale.mmToIU( 1 ), 0 ) );
        shape->SetLayer( F_SilkS );
        shape->SetStroke( STROKE_PARAMS( pcbIUScale.mmToIU( 0.1 ), LINE_STYLE::SOLID ) );
        board->Add( shape );

        if( !groups.empty() )
            groups[i % groups.size()]->AddItem( shape );
    }

    return board;
}


/**
 * Build a board, save it once and return the save time in seconds.
 *
 * The board is built and destroyed inside the call so only one board is alive
 * per measurement and heap state stays comparable between measurements.
 */
double saveBoardSeconds( const std::filesystem::path& aDir, std::size_t aGroupCount,
                         std::size_t aTotalShapes )
{
    auto board = buildBoardWithGroups( aGroupCount, aTotalShapes );

    const std::filesystem::path savePath =
            aDir / ( "groups" + std::to_string( aGroupCount ) + ".kicad_pcb" );

    PROF_TIMER timer;
    KI_TEST::DumpBoardToFile( *board, savePath.string() );
    timer.Stop();

    return timer.msecs() / 1000.0;
}

} // namespace


BOOST_AUTO_TEST_SUITE( GroupSavePerformance )


/**
 * Save boards with the same shape count but very different group counts.
 *
 * Pre-fix, the format() of each group rebuilt a pointer set scanning the full
 * item-by-id cache, so the per-group cost grew with the board's item count and
 * the total cost grew quadratically in the group count.
 *
 * We check that scaling is sub-quadratic with a generous margin so the test is
 * robust against debug builds, CI noise and slow VMs while still failing if
 * the O(N^2) regression returns.
 */
BOOST_AUTO_TEST_CASE( SaveScalesLinearlyWithGroupCount )
{
    constexpr std::size_t kTotalShapes = 5000;

    KI_TEST::TEMPORARY_DIRECTORY tempDir( "group_save_perf", "" );

    // Warm up the save path (allocators, formatter, disk cache) so the first
    // measurement is not dominated by one-shot setup costs.
    (void) saveBoardSeconds( tempDir.GetPath(), 10, 100 );

    // Take the best of two runs per configuration so a scheduler stall during
    // a single save cannot fail the test on a loaded CI machine.
    auto bestSave =
            [&]( std::size_t aGroupCount )
            {
                double best = std::numeric_limits<double>::infinity();

                for( int run = 0; run < 2; ++run )
                {
                    best = std::min( best, saveBoardSeconds( tempDir.GetPath(), aGroupCount,
                                                             kTotalShapes ) );
                }

                return best;
            };

    double t100 = bestSave( 100 );
    double t5000 = bestSave( 5000 );

    BOOST_TEST_MESSAGE( "Save time (s)  100 groups : " << t100 );
    BOOST_TEST_MESSAGE( "Save time (s) 5000 groups : " << t5000 );

    // Use a 5 ms floor so very fast runs do not produce huge ratios from
    // measurement jitter.
    const double floorSec = 0.005;
    double       ratio = t5000 / std::max( t100, floorSec );

    BOOST_TEST_MESSAGE( "Ratio (5000 / 100): " << ratio );

    // Linear scaling in the group count gives a small single-digit ratio here
    // because the serialised output is dominated by the shared 5000 shapes.
    // The unfixed O(Groups * BoardItems) code rescans the full item cache for
    // each of the 5000 groups and lands far above this bound.
    BOOST_CHECK_LT( ratio, 20.0 );
}


BOOST_AUTO_TEST_SUITE_END()
