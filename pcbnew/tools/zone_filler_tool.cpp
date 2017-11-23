/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2014-2017 CERN
 * Copyright (C) 2014-2017 KiCad Developers, see AUTHORS.txt for contributors.
 * @author Maciej Suminski <maciej.suminski@cern.ch>
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

#include <painter.h>
#include <project.h>
#include <pcbnew_id.h>
#include <wxPcbStruct.h>
#include <class_board.h>
#include <class_zone.h>
#include <pcb_draw_panel_gal.h>
#include <class_module.h>
#include <connectivity_data.h>
#include <board_commit.h>

#include <widgets/progress_reporter.h>
#include <tool/tool_manager.h>
#include <bitmaps.h>
#include <hotkeys.h>

#include "pcb_actions.h"
#include "selection_tool.h"
#include "zone_filler_tool.h"

#ifdef USE_OPENMP
#include <omp.h>
#endif /* USE_OPENMP */

// Zone actions
TOOL_ACTION PCB_ACTIONS::zoneFill( "pcbnew.ZoneFiller.zoneFill",
        AS_GLOBAL, 0,
        _( "Fill" ), _( "Fill zone(s)" ), fill_zone_xpm );

TOOL_ACTION PCB_ACTIONS::zoneFillAll( "pcbnew.ZoneFiller.zoneFillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_FILL_OR_REFILL ),
        _( "Fill All" ), _( "Fill all zones" ) );

TOOL_ACTION PCB_ACTIONS::zoneUnfill( "pcbnew.ZoneFiller.zoneUnfill",
        AS_GLOBAL, 0,
        _( "Unfill" ), _( "Unfill zone(s)" ), zone_unfill_xpm );

TOOL_ACTION PCB_ACTIONS::zoneUnfillAll( "pcbnew.ZoneFiller.zoneUnfillAll",
        AS_GLOBAL, TOOL_ACTION::LegacyHotKey( HK_ZONE_REMOVE_FILLED ),
        _( "Unfill All" ), _( "Unfill all zones" ) );


class ZONE_FILLER
{
public:
    ZONE_FILLER( BOARD* aBoard, COMMIT* aCommit );
    ~ZONE_FILLER();

    void    SetProgressReporter( PROGRESS_REPORTER* aReporter );
    void    Fill( std::vector<ZONE_CONTAINER*> aZones );
    void    Unfill( std::vector<ZONE_CONTAINER*> aZones );

private:
    COMMIT* m_commit;
    PROGRESS_REPORTER* m_progressReporter;
    BOARD* m_board;
};

ZONE_FILLER::ZONE_FILLER(  BOARD* aBoard, COMMIT* aCommit ) :
    m_commit( aCommit ),
    m_board( aBoard )
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

    assert( m_commit );

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
        m_commit->Modify( toFill[i].m_zone );
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

        m_progressReporter->AdvanceProgress();
    }

    if( m_progressReporter )
    {
        m_progressReporter->AdvancePhase();
        m_progressReporter->Report( _( "Removing insulated copper islands..." ) );
    }

    m_board->GetConnectivity()->SetProgressReporter( m_progressReporter );
    m_board->GetConnectivity()->FindIsolatedCopperIslands( toFill );

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
        m_progressReporter->AdvanceProgress();
        toFill[i].m_zone->CacheTriangulation();
    }

    m_progressReporter->AdvancePhase();
    m_progressReporter->Report( _( "Committing changes..." ) );

    m_commit->Push( _( "Fill Zones" ), false );
}




ZONE_FILLER_TOOL::ZONE_FILLER_TOOL() :
    PCB_TOOL( "pcbnew.ZoneFiller" )
{
}


ZONE_FILLER_TOOL::~ZONE_FILLER_TOOL()
{
}


void ZONE_FILLER_TOOL::Reset( RESET_REASON aReason )
{
}

// Zone actions
int ZONE_FILLER_TOOL::ZoneFill( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto item : selection )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*> ( item );

        toFill.push_back(zone);
    }

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Fill Zones" ), 3 )
            );

    ZONE_FILLER filler( board(), &commit );
    filler.SetProgressReporter( progressReporter.get() );
    filler.Fill( toFill );

    return 0;
}


int ZONE_FILLER_TOOL::ZoneFillAll( const TOOL_EVENT& aEvent )
{
    std::vector<ZONE_CONTAINER*> toFill;

    BOARD_COMMIT commit( this );

    for( auto zone : board()->Zones() )
    {
        toFill.push_back(zone);
    }

    std::unique_ptr<WX_PROGRESS_REPORTER> progressReporter(
            new WX_PROGRESS_REPORTER( frame(), _( "Fill All Zones" ), 3 )
            );

    ZONE_FILLER filler( board(), &commit );
    filler.SetProgressReporter( progressReporter.get() );
    filler.Fill( toFill );

    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfill( const TOOL_EVENT& aEvent )
{
    auto selTool = m_toolMgr->GetTool<SELECTION_TOOL>();
    const auto& selection = selTool->GetSelection();
    auto connectivity = getModel<BOARD>()->GetConnectivity();

    BOARD_COMMIT commit( this );

    for( auto item : selection )
    {
        assert( item->Type() == PCB_ZONE_AREA_T );

        ZONE_CONTAINER* zone = static_cast<ZONE_CONTAINER*>( item );

        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
    }

    commit.Push( _( "Unfill Zone" ) );

    connectivity->RecalculateRatsnest();

    return 0;
}


int ZONE_FILLER_TOOL::ZoneUnfillAll( const TOOL_EVENT& aEvent )
{
    BOARD*  board = getModel<BOARD>();
    auto    connectivity = getModel<BOARD>()->GetConnectivity();

    BOARD_COMMIT commit( this );

    for( int i = 0; i < board->GetAreaCount(); ++i )
    {
        ZONE_CONTAINER* zone = board->GetArea( i );

        commit.Modify( zone );

        zone->SetIsFilled( false );
        zone->ClearFilledPolysList();
    }

    commit.Push( _( "Unfill All Zones" ) );

    connectivity->RecalculateRatsnest();

    return 0;
}


void ZONE_FILLER_TOOL::setTransitions()
{
    // Zone actions
    Go( &ZONE_FILLER_TOOL::ZoneFill, PCB_ACTIONS::zoneFill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneFillAll, PCB_ACTIONS::zoneFillAll.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfill, PCB_ACTIONS::zoneUnfill.MakeEvent() );
    Go( &ZONE_FILLER_TOOL::ZoneUnfillAll, PCB_ACTIONS::zoneUnfillAll.MakeEvent() );
}
