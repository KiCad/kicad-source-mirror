/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright The KiCad Developers, see AUTHORS.TXT for contributors.
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

#include <qa_utils/utility_registry.h>

#include <chrono>
#include <iostream>
#include <vector>

#include <wx/filename.h>

#include <board.h>
#include <board_commit.h>
#include <board_design_settings.h>
#include <connectivity/connectivity_data.h>
#include <core/profile.h>
#include <drc/drc_engine.h>
#include <mmh3_hash.h>
#include <pcb_io/pcb_io_mgr.h>
#include <settings/settings_manager.h>
#include <tool/tool_manager.h>
#include <zone.h>
#include <zone_filler.h>

#include <pcbnew_utils/board_test_utils.h>


/**
 * Headless zone-fill benchmark; times a full refill over N iterations.
 */
static int fill_bench_main_func( int argc, char** argv )
{
    if( argc < 2 )
    {
        std::cerr << "Usage: fill_bench <board.kicad_pcb> [iterations]" << std::endl;
        return KI_TEST::RET_CODES::BAD_CMDLINE;
    }

    wxString boardPath = wxString::FromUTF8( argv[1] );
    int      iterations = argc > 2 ? std::atoi( argv[2] ) : 1;

    std::unique_ptr<BOARD> board;

    {
        PROF_TIMER loadTimer;
        board.reset( PCB_IO_MGR::Load( PCB_IO_MGR::KICAD_SEXP, boardPath, nullptr, {}, nullptr, nullptr ) );
        std::cout << "Load: " << loadTimer.SinceStart<std::chrono::milliseconds>().count() << " ms"
                  << std::endl;
    }

    if( !board )
    {
        std::cerr << "Failed to load board" << std::endl;
        return KI_TEST::RET_CODES::TOOL_SPECIFIC;
    }

    BOARD_DESIGN_SETTINGS& bds = board->GetDesignSettings();

    auto drcEngine = std::make_shared<DRC_ENGINE>( board.get(), &bds );
    drcEngine->InitEngine( wxFileName() );
    bds.m_DRCEngine = drcEngine;

    board->BuildListOfNets();
    board->BuildConnectivity();

    std::cout << "Zones: " << board->Zones().size() << ", nets: " << board->GetNetCount() << std::endl;

    TOOL_MANAGER toolMgr;
    toolMgr.SetEnvironment( board.get(), nullptr, nullptr, nullptr, nullptr );

    KI_TEST::DUMMY_TOOL* dummyTool = new KI_TEST::DUMMY_TOOL();
    toolMgr.RegisterTool( dummyTool );

    std::vector<ZONE*> toFill;

    for( ZONE* zone : board->Zones() )
        toFill.push_back( zone );

    for( int i = 0; i < iterations; ++i )
    {
        BOARD_COMMIT commit( dummyTool );
        ZONE_FILLER  filler( board.get(), &commit );

        PROF_TIMER fillTimer;
        bool       ok = filler.Fill( toFill, false, nullptr );
        double     ms = static_cast<double>( fillTimer.SinceStart<std::chrono::milliseconds>().count() );

        std::cout << "Fill #" << i << ": " << ms << " ms" << ( ok ? "" : " (FAILED)" )
                  << std::endl;
    }

    // Order-independent digest, so two configs can be checked for identical output.
    double             totalArea = 0.0;
    unsigned long long outlineCount = 0;
    unsigned long long vertexCount = 0;

    // Order-sensitive so two runs match only when the fill is byte-identical, not merely
    // equal in area.
    MMH3_HASH streamHash( 0x5A4F4E45 );

    for( ZONE* zone : board->Zones() )
    {
        for( PCB_LAYER_ID layer : zone->GetLayerSet().Seq() )
        {
            if( !zone->HasFilledPolysForLayer( layer ) )
                continue;

            std::shared_ptr<SHAPE_POLY_SET> polys = zone->GetFilledPolysList( layer );
            totalArea += polys->Area();
            outlineCount += (unsigned long long) polys->OutlineCount();
            vertexCount += (unsigned long long) polys->FullPointCount();

            for( auto it = polys->CIterateWithHoles(); it; ++it )
            {
                streamHash.add( it->x );
                streamHash.add( it->y );
            }
        }
    }

    HASH_128 digest = streamHash.digest();

    std::cout << "FillSummary area=" << std::fixed << totalArea << " outlines=" << outlineCount
              << " vertices=" << vertexCount << " streamhash=" << digest.ToString() << std::endl;

    return KI_TEST::RET_CODES::OK;
}


static bool registered = UTILITY_REGISTRY::Register( { "fill_bench",
                                                       "Benchmark zone filling on a board",
                                                       fill_bench_main_func } );
