/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Tomasz Włostowski <tomasz.wlostowski@cern.ch>
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

#include <cstdint>
#include <thread>
#include <mutex>

#include <class_board.h>
#include <class_zone.h>
#include <class_module.h>
#include <connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>

#include "zone_filler.h"

#ifdef USE_OPENMP
#include <omp.h>
#endif /* USE_OPENMP */


ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
    m_commit( aCommit ),
    m_board( aBoard ),
    m_progressReporter( nullptr )
{
}


ZONE_FILLER::~ZONE_FILLER()
{
}


void ZONE_FILLER::SetProgressReporter( PROGRESS_REPORTER* aReporter )
{
    m_progressReporter = aReporter;
}


void ZONE_FILLER::Fill( std::vector<ZONE_CONTAINER*> aZones )
{
    std::vector<CN_ZONE_ISOLATED_ISLAND_LIST> toFill;
    auto connectivity = m_board->GetConnectivity();

    // Remove segment zones
    m_board->m_Zone.DeleteAll();

    int ii;

    for( auto zone : aZones )
    {
        // Keepout zones are not filled
        if( zone->GetIsKeepout() )
            continue;

        CN_ZONE_ISOLATED_ISLAND_LIST l;
        l.m_zone = zone;
        toFill.push_back( l );
    }

    int zoneCount = m_board->GetAreaCount();

    for( int i = 0; i < toFill.size(); i++ )
    {
        if (m_commit)
        {
            m_commit->Modify( toFill[i].m_zone );
        }
    }

    if( m_progressReporter )
    {
        m_progressReporter->Report( _( "Calculating zone fills..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }

    #ifdef USE_OPENMP
        #pragma omp parallel for schedule(dynamic)
    #endif
    for( int i = 0; i < toFill.size(); i++ )
    {
        toFill[i].m_zone->BuildFilledSolidAreasPolygons( m_board );

        if( m_progressReporter )
        {
            m_progressReporter->AdvanceProgress();
        }
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing insulated copper islands..." ) );
    }

    connectivity->SetProgressReporter( m_progressReporter );
    connectivity->FindIsolatedCopperIslands( toFill );

    for( auto& zone : toFill )
    {
        std::sort( zone.m_islands.begin(), zone.m_islands.end(), std::greater<int>() );
        SHAPE_POLY_SET poly = zone.m_zone->GetFilledPolysList();

        for( auto idx : zone.m_islands )
        {
            poly.DeletePolygon( idx );
        }

        zone.m_zone->AddFilledPolysList( poly );
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Caching polygon triangulations..." ) );
        m_progressReporter->SetMaxProgress( toFill.size() );
    }

    #ifdef USE_OPENMP
        #pragma omp parallel for schedule(dynamic)
    #endif

    for( int i = 0; i < toFill.size(); i++ )
    {
        if( m_progressReporter )
        {
            m_progressReporter->AdvanceProgress();
        }
        toFill[i].m_zone->CacheTriangulation();
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Committing changes..." ) );
    }

    connectivity->SetProgressReporter( nullptr );

    if( m_commit )
    {
        m_commit->Push( _( "Fill Zone(s)" ), false );
    } else {
        for( int i = 0; i < toFill.size(); i++ )
        {
            connectivity->Update( toFill[i].m_zone );
        }
    }
}
