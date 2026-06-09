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

#include <filesystem>
#include <fstream>
#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <base_units.h>
#include <board.h>
#include <kiid.h>
#include <pcb_dimension.h>
#include <pcb_shape.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <settings/settings_manager.h>
#include <core/utf8.h>

namespace fs = std::filesystem;

/**
 * Count line ("L ") feature records in a single ODB++ "features" file.
 *
 * A dimension's extension lines, crossbar and arrowheads are emitted as line records, so a
 * non-zero count in the silkscreen layer proves the dimension reached the feature stream
 * (regression guard for GitLab #20249, where the PCB_DIM_*_T cases were stubbed out of the
 * ODB++ feature generator).
 */
static int countOdbLineRecords( const fs::path& aFeaturesFile )
{
    int           lineCount = 0;
    std::ifstream stream( aFeaturesFile );
    std::string   line;

    while( std::getline( stream, line ) )
    {
        if( line.rfind( "L ", 0 ) == 0 )
            lineCount++;
    }

    return lineCount;
}


/**
 * Sum line ("L ") feature records across every silkscreen "features" file in an ODB++ export tree.
 *
 * Both front and back silk layers produce a directory whose name contains "silk", so a search that
 * stops at the first match would depend on std::filesystem directory-iteration order, which is
 * unspecified.  The dimension under test sits on the front silk, so picking the empty back-silk file
 * first would spuriously report zero.  Aggregating across all silk layers keeps the check
 * order-independent while still matching by substring to stay robust against ODB++ naming changes.
 *
 * aFoundFile is set when at least one silk features file exists, so the caller can still assert the
 * export produced a silkscreen layer at all.
 */
static int countSilkLineRecords( const fs::path& aRoot, bool& aFoundFile )
{
    int total = 0;
    aFoundFile = false;

    for( const fs::directory_entry& entry : fs::recursive_directory_iterator( aRoot ) )
    {
        if( !entry.is_regular_file() || entry.path().filename() != "features" )
            continue;

        const std::string layerDir = entry.path().parent_path().filename().string();

        if( layerDir.find( "silk" ) == std::string::npos )
            continue;

        aFoundFile = true;
        total += countOdbLineRecords( entry.path() );
    }

    return total;
}


BOOST_AUTO_TEST_CASE( OdbDimensionExport )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    const int side = pcbIUScale.mmToIU( 20 );

    // Give the board a square Edge.Cuts outline so the export produces a valid board profile.  Edge
    // cuts land in their own layer feature file, not the silkscreen one we inspect, so they cannot
    // mask a dropped dimension.
    auto addEdge = [&]( const VECTOR2I& aStart, const VECTOR2I& aEnd )
    {
        PCB_SHAPE* edge = new PCB_SHAPE( board.get(), SHAPE_T::SEGMENT );
        edge->SetLayer( Edge_Cuts );
        edge->SetStart( aStart );
        edge->SetEnd( aEnd );
        edge->SetWidth( pcbIUScale.mmToIU( 0.1 ) );
        board->Add( edge );
    };

    addEdge( { 0, 0 }, { side, 0 } );
    addEdge( { side, 0 }, { side, side } );
    addEdge( { side, side }, { 0, side } );
    addEdge( { 0, side }, { 0, 0 } );

    // The item under test: an aligned dimension on the front silkscreen.
    PCB_DIM_ALIGNED* dimension = new PCB_DIM_ALIGNED( board.get(), PCB_DIM_ALIGNED_T );
    dimension->SetLayer( F_SilkS );
    dimension->SetStart( { 0, 0 } );
    dimension->SetEnd( { side, 0 } );
    dimension->SetHeight( pcbIUScale.mmToIU( 3 ) );
    dimension->SetLineThickness( pcbIUScale.mmToIU( 0.15 ) );

    // Update() must run after the feature points are set to populate the geometry returned by
    // GetShapes(); otherwise the dimension has no shapes to plot.
    dimension->Update();

    BOOST_REQUIRE_MESSAGE( !dimension->GetShapes().empty(),
                           "Dimension produced no geometry to export" );

    board->Add( dimension );

    const fs::path outDir = fs::temp_directory_path()
                            / ( "kicad_qa_odb_dimension_20249_" + KIID().AsString().ToStdString() );

    BOOST_REQUIRE( fs::create_directory( outDir ) );

    PCB_IO_ODBPP                odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "6";

    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( outDir.string(), board.get(), &props ) );

    BOOST_REQUIRE_MESSAGE( fs::exists( outDir ), "ODB++ export produced no output tree" );

    bool foundSilk = false;
    int  lineCount = countSilkLineRecords( outDir, foundSilk );

    BOOST_REQUIRE_MESSAGE( foundSilk, "ODB++ export produced no silkscreen features file" );

    BOOST_CHECK_MESSAGE( lineCount > 0,
                         "Dimensions were dropped from the ODB++ export (no line features emitted)" );

    fs::remove_all( outDir );
}
