/*
 * This program is part of KiCad, a free EDA CAD application.
 * Copyright The KiCad Developers, see AUTHORS.txt for contributors.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include <geometry/shape_poly_set.h>

#include <pcbnew_utils/board_file_utils.h>
#include <qa_utils/utility_registry.h>

#include <board.h>
#include <zone.h>
#include <layer_ids.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace fs = std::filesystem;


static std::string SerializePolySet( const SHAPE_POLY_SET& aPolySet )
{
    std::ostringstream ss;

    ss << "polyset " << aPolySet.OutlineCount() << "\n";

    for( int i = 0; i < aPolySet.OutlineCount(); i++ )
    {
        const SHAPE_POLY_SET::POLYGON& poly = aPolySet.CPolygon( i );

        ss << "poly " << poly.size() << "\n";

        for( const SHAPE_LINE_CHAIN& chain : poly )
        {
            ss << chain.PointCount() << "\n";

            for( int k = 0; k < chain.PointCount(); k++ )
            {
                const VECTOR2I& pt = chain.CPoint( k );
                ss << pt.x << " " << pt.y << "\n";
            }
        }
    }

    return ss.str();
}


static void CollectBoardFiles( const fs::path& aPath, std::vector<fs::path>& aOut )
{
    if( fs::is_regular_file( aPath ) && aPath.extension() == ".kicad_pcb" )
    {
        aOut.push_back( aPath );
    }
    else if( fs::is_directory( aPath ) )
    {
        for( const auto& entry : fs::recursive_directory_iterator( aPath ) )
        {
            if( entry.is_regular_file() && entry.path().extension() == ".kicad_pcb" )
                aOut.push_back( entry.path() );
        }
    }
}


static int extract_zone_fills_main( int argc, char* argv[] )
{
    std::string outputDir = ".";
    std::vector<std::string> inputs;

    for( int i = 1; i < argc; i++ )
    {
        std::string arg = argv[i];

        if( arg == "--output-dir" && i + 1 < argc )
        {
            outputDir = argv[++i];
        }
        else
        {
            inputs.push_back( arg );
        }
    }

    if( inputs.empty() )
    {
        std::cerr << "Usage: qa_pcbnew_tools extract_zone_fills [--output-dir <dir>] "
                     "<board_or_dir> [board_or_dir ...]\n";
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    fs::create_directories( outputDir );

    std::vector<fs::path> boardFiles;

    for( const auto& input : inputs )
        CollectBoardFiles( input, boardFiles );

    std::cout << "Found " << boardFiles.size() << " board files\n";

    int totalZones = 0;
    int boardsWithData = 0;

    for( const auto& boardPath : boardFiles )
    {
        std::unique_ptr<BOARD> board;

        try
        {
            board = KI_TEST::ReadBoardFromFileOrStream( boardPath.string() );
        }
        catch( const std::exception& e )
        {
            std::cerr << "Error loading " << boardPath << ": " << e.what() << "\n";
            continue;
        }

        if( !board )
        {
            std::cerr << "Failed to load: " << boardPath << "\n";
            continue;
        }

        std::ostringstream fileContent;
        int zonesInBoard = 0;
        std::string boardName = boardPath.stem().string();

        fileContent << "(zone_fills\n";
        fileContent << "  (source \"" << boardPath.filename().string() << "\")\n";

        for( ZONE* zone : board->Zones() )
        {
            if( zone->GetIsRuleArea() )
                continue;

            for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
            {
                auto poly = zone->GetFilledPolysList( layer );

                if( !poly || poly->OutlineCount() == 0 )
                    continue;

                fileContent << "  (zone (layer \"" << LayerName( layer ).ToStdString() << "\")";
                fileContent << " (net \"" << zone->GetNetname().ToStdString() << "\")";
                fileContent << " (outline_count " << poly->OutlineCount() << ")";

                int vertexCount = 0;

                for( int i = 0; i < poly->OutlineCount(); i++ )
                {
                    const auto& polygon = poly->CPolygon( i );

                    for( const SHAPE_LINE_CHAIN& chain : polygon )
                        vertexCount += chain.PointCount();
                }

                fileContent << " (vertex_count " << vertexCount << ")\n";
                fileContent << "    (polyset\n";
                fileContent << SerializePolySet( *poly );
                fileContent << "    )\n";
                fileContent << "  )\n";

                zonesInBoard++;
            }
        }

        fileContent << ")\n";

        if( zonesInBoard > 0 )
        {
            fs::path outPath = fs::path( outputDir ) / ( boardName + ".kicad_polys" );
            std::ofstream out( outPath );
            out << fileContent.str();
            out.close();

            std::cout << boardPath.filename() << " -> " << zonesInBoard << " zones\n";
            totalZones += zonesInBoard;
            boardsWithData++;
        }
    }

    std::cout << "\nExtracted " << totalZones << " zones from "
              << boardsWithData << " boards\n";

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( {
        "extract_zone_fills",
        "Extract zone fill polygons from boards for triangulation benchmarking",
        extract_zone_fills_main,
} );
