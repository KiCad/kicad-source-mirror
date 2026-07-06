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

#include <filesystem>
#include <fstream>
#include <memory>

#include <qa_utils/wx_utils/unit_test_utils.h>
#include <boost/test/unit_test.hpp>

#include <base_units.h>
#include <board.h>
#include <footprint.h>
#include <kiid.h>
#include <pcb_shape.h>
#include <pcbnew/pcb_io/odbpp/pcb_io_odbpp.h>
#include <settings/settings_manager.h>
#include <core/utf8.h>

namespace fs = std::filesystem;

/**
 * Count surface ("S ") feature records in a single ODB++ "features" file.
 *
 * A filled polygon reaches the feature stream as a surface record, so a non-zero count on a
 * solder-mask layer proves the mask opening was emitted.
 */
static int countOdbSurfaceRecords( const fs::path& aFeaturesFile )
{
    int           surfaceCount = 0;
    std::ifstream stream( aFeaturesFile );
    std::string   line;

    while( std::getline( stream, line ) )
    {
        if( line.rfind( "S ", 0 ) == 0 )
            surfaceCount++;
    }

    return surfaceCount;
}


/**
 * Sum surface ("S ") feature records across every solder-mask "features" file in an ODB++ tree.
 *
 * Both front and back mask layers produce a directory whose name contains "mask", so a search that
 * stops at the first match would depend on unspecified std::filesystem iteration order.  The shape
 * under test sits on the front mask, so picking the empty back-mask file first would spuriously
 * report zero.  Aggregating across all mask layers keeps the check order-independent while matching
 * by substring to stay robust against ODB++ naming changes.
 *
 * aFoundFile is set when at least one mask features file exists, so the caller can still assert the
 * export produced a solder-mask layer at all.
 */
static int countMaskSurfaceRecords( const fs::path& aRoot, bool& aFoundFile )
{
    int total = 0;
    aFoundFile = false;

    for( const fs::directory_entry& entry : fs::recursive_directory_iterator( aRoot ) )
    {
        if( !entry.is_regular_file() || entry.path().filename() != "features" )
            continue;

        const std::string layerDir = entry.path().parent_path().filename().string();

        if( layerDir.find( "mask" ) == std::string::npos )
            continue;

        aFoundFile = true;
        total += countOdbSurfaceRecords( entry.path() );
    }

    return total;
}


/**
 * Regression guard for GitLab #24690: a footprint polygon on a copper layer with the "Solder mask"
 * flag set must knock a mask opening out of the ODB++ export, matching the Gerber output.  The
 * mask-layer assignment loops in ODB_STEP_ENTITY::MakeLayerEntity() previously bucketed footprint
 * graphics and board drawings by their single GetLayer(), so a mask-flagged shape never reached the
 * F_Mask layer and the opening was silently dropped.
 */
BOOST_AUTO_TEST_CASE( OdbSolderMaskShapeExport )
{
    SETTINGS_MANAGER       settingsManager;
    std::unique_ptr<BOARD> board = std::make_unique<BOARD>();

    const int side = pcbIUScale.mmToIU( 20 );

    // Give the board a square Edge.Cuts outline so the export produces a valid board profile.  Edge
    // cuts land in their own layer feature file, not the mask one we inspect, so they cannot mask a
    // dropped shape.
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

    // The item under test: a filled polygon on the front copper layer of a footprint, flagged to
    // open the solder mask over itself.  SetHasSolderMask() is what puts the shape on F_Mask via
    // PCB_SHAPE::GetLayerSet(), the layer the export must honor.
    FOOTPRINT* fp = new FOOTPRINT( board.get() );
    fp->SetReference( wxT( "U1" ) );
    fp->SetPosition( { pcbIUScale.mmToIU( 10 ), pcbIUScale.mmToIU( 10 ) } );

    PCB_SHAPE* poly = new PCB_SHAPE( fp, SHAPE_T::POLY );
    poly->SetLayer( F_Cu );
    poly->SetFilled( true );
    poly->SetPolyPoints( { { pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 5 ) },
                           { pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 5 ) },
                           { pcbIUScale.mmToIU( 15 ), pcbIUScale.mmToIU( 15 ) },
                           { pcbIUScale.mmToIU( 5 ), pcbIUScale.mmToIU( 15 ) } } );
    poly->SetHasSolderMask( true );

    BOOST_REQUIRE_MESSAGE( poly->IsOnLayer( F_Mask ),
                           "Test shape is not reported on F_Mask; SetHasSolderMask had no effect" );

    fp->Add( poly );
    board->Add( fp );

    const fs::path outDir =
            fs::temp_directory_path() / ( "kicad_qa_odb_soldermask_24690_" + KIID().AsString().ToStdString() );

    BOOST_REQUIRE( fs::create_directory( outDir ) );

    PCB_IO_ODBPP                odbExporter;
    std::map<std::string, UTF8> props;
    props["units"] = "mm";
    props["sigfig"] = "6";

    BOOST_REQUIRE_NO_THROW( odbExporter.SaveBoard( outDir.string(), board.get(), &props ) );

    BOOST_REQUIRE_MESSAGE( fs::exists( outDir ), "ODB++ export produced no output tree" );

    bool foundMask = false;
    int  surfaceCount = countMaskSurfaceRecords( outDir, foundMask );

    BOOST_REQUIRE_MESSAGE( foundMask, "ODB++ export produced no solder-mask features file" );

    BOOST_CHECK_MESSAGE( surfaceCount > 0, "Mask-flagged shape was dropped from the ODB++ export "
                                           "(no surface features on the solder-mask layer)" );

    fs::remove_all( outDir );
}
